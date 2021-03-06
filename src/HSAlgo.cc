#include <iostream>
#include "HSAlgo.h"
#include "log_util.h"

namespace HS {

void HSAlgo::Run(void) {
  master_thread_ = std::thread([this] { this->Master(); });
  master_thread_.join();
}

void HSAlgo::Process(const std::size_t& id,
                     ThreadState* state,
                     utils::MessageChannel& send_to_pred_ch,
                     utils::MessageChannel& recv_from_pred_ch,
                     utils::MessageChannel& send_to_succ_ch,
                     utils::MessageChannel& recv_from_succ_ch) {
  std::size_t round_to_exit(2); // how many rounds before I exit
  std::size_t succ_to_exit(2);  // how many rounds before my successor exits
  std::ptrdiff_t round(0);      // round counter
  bool pred_exit(false);        // a flag indicating whether predecessor exits or not

  // states
  const std::size_t uid(id);
  utils::Message msg_to_succ{utils::SelectLeader, uid, utils::Out, 1};
  utils::Message msg_to_pred{utils::SelectLeader, uid, utils::Out, 1};
  Status status(Unknown);
  std::size_t phrase(0);

  while (round_to_exit) {
    // wait master to notify that this round begins
    {
      std::unique_lock<std::mutex> lock(mutex_round_begin_);
      round_begin_.wait(lock, [state] { return (*state == RoundBegin); });
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round << " begins" << std::endl;
    }

    // message-generation function
    std::unique_ptr<utils::Message> p_msg_to_succ(nullptr);
    std::unique_ptr<utils::Message> p_msg_to_pred(nullptr);

    // send a message to predecessor if it has not exited
    if (!pred_exit) {
      p_msg_to_pred.reset(new utils::Message(msg_to_pred));
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " sends to predecessor: " << p_msg_to_pred->ToString() << std::endl;
      send_to_pred_ch.Send(std::move(p_msg_to_pred));
    }

    // send a message to successor if it has not exited
    if (succ_to_exit) {
      p_msg_to_succ.reset(new utils::Message(msg_to_succ));
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " sends to successor: " << p_msg_to_succ->ToString() << std::endl;
      send_to_succ_ch.Send(std::move(p_msg_to_succ));
    }

    // transition
    msg_to_succ = {utils::NullType, 0, utils::Null, 0};
    msg_to_pred = {utils::NullType, 0, utils::Null, 0};
    std::unique_ptr<utils::Message> p_msg_from_succ(nullptr);
    std::unique_ptr<utils::Message> p_msg_from_pred(nullptr);

    // receive a message from predecessor if it has not exited
    if (!pred_exit) {
      recv_from_pred_ch.Receive(p_msg_from_pred);
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " receives from predecessor: " << p_msg_from_pred->ToString() << std::endl;
    }

    // receive a message from successor if it has not exited
    if (succ_to_exit) {
      recv_from_succ_ch.Receive(p_msg_from_succ);
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " receives from successor: " << p_msg_from_succ->ToString() << std::endl;
    }

    // count down for me
    if (round_to_exit == 1) {
      round_to_exit--;
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " exits" << std::endl;
    }

    // count down for successor
    if (succ_to_exit == 1) {
      succ_to_exit--;
    }

    // relay predecessor's leader notification to successor
    if (p_msg_from_pred &&
        p_msg_from_pred->type_ == utils::NotifyLeader &&
        p_msg_from_pred->flag_ == utils::Out) {
      /*** exit protocol ***
      /* If I receive leader notification message,
      /* I assume that my predcessor will exit in this round
      /*           and my successor will exit in next two round.
      /* I will exit in next round because I have to relay leader notification to successor.
       */
      // So I set the flag. From next round, I will not send or receive any message to or from predcessor.
      pred_exit = true;
      // If leader receives leader notification message, it means all other processes has been exits.
      // Leader exits in this round
      if (status == Leader) {
        succ_to_exit = 0;
        round_to_exit = 0;
      } else {
        // relay predecessor's leader notification to successor
        msg_to_succ = *p_msg_from_pred;
        // I will exit in next round
        round_to_exit--;
        std::cout << lock_with(mutex_log_)
                  << "[process " << id << "] round " << round
                  << " I know process " << p_msg_from_pred->uid_
                  << " is the leader." << std::endl;
      }

      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " will exit in " << round_to_exit << " round(s)" << std::endl;
    }

    // relay predecessor's out-bound message
    if (p_msg_from_pred &&
        p_msg_from_pred->type_ == utils::SelectLeader &&
        p_msg_from_pred->flag_ == utils::Out) {
      if (p_msg_from_pred->uid_ > uid &&
          p_msg_from_pred->hop_count_ > 1) {
        msg_to_succ = {utils::SelectLeader,
                       p_msg_from_pred->uid_,
                       utils::Out,
                       p_msg_from_pred->hop_count_ - 1};
        debug_clog << lock_with(mutex_log_)
                   << "[process " << id << "] round " << round
                   << " relay predecessor's out-bound message "
                   << p_msg_from_pred->uid_ << std::endl;
      } else if (p_msg_from_pred->uid_ > uid &&
                 p_msg_from_pred->hop_count_ == 1) {
        msg_to_pred = {utils::SelectLeader,
                       p_msg_from_pred->uid_,
                       utils::In,
                       1};
        debug_clog << lock_with(mutex_log_)
                   << "[process " << id << "] round " << round
                   << " turn around predecessor's out-bound message "
                   << p_msg_from_pred->uid_ << std::endl;
      } else if (p_msg_from_pred->uid_ == uid) {
        status = Leader;
        std::cout << lock_with(mutex_log_)
                  << "[process " << id << "] round " << round
                  << " I'm the leader." << std::endl;
        // send leader notification to successor in next round
        msg_to_succ = {utils::NotifyLeader, uid, utils::Out, 0};
        // successor exiting begins counting down
        succ_to_exit--;
      } else if (p_msg_from_pred->uid_ < uid) {
        debug_clog << lock_with(mutex_log_)
                   << "[process " << id << "] round " << round
                   << " discards predecessor's out-bound message "
                   << p_msg_from_pred->uid_ << std::endl;
      }
    }

    // relay successor's out-bound message
    if (p_msg_from_succ &&
        p_msg_from_succ->type_ == utils::SelectLeader &&
        p_msg_from_succ->flag_ == utils::Out) {
      if (p_msg_from_succ->uid_ > uid &&
          p_msg_from_succ->hop_count_ > 1) {
        msg_to_pred = {utils::SelectLeader,
                       p_msg_from_succ->uid_,
                       utils::Out,
                       p_msg_from_succ->hop_count_ - 1};
        debug_clog << lock_with(mutex_log_)
                   << "[process " << id << "] round " << round
                   << " relay successor's out-bound message "
                   << p_msg_from_succ->uid_ << std::endl;
      } else if (p_msg_from_succ->uid_ > uid &&
                 p_msg_from_succ->hop_count_ == 1) {
        msg_to_succ = {utils::SelectLeader, p_msg_from_succ->uid_, utils::In, 1};
        debug_clog << lock_with(mutex_log_)
                   << "[process " << id << "] round " << round
                   << " turn around successor's out-bound message "
                   << p_msg_from_succ->uid_ << std::endl;
      } else if (p_msg_from_succ->uid_ == uid) {
        status = Leader;
      } else if (p_msg_from_succ->uid_ < uid) {
        debug_clog << lock_with(mutex_log_)
                   << "[process " << id << "] round " << round
                   << " discards successor's out-bound message "
                   << p_msg_from_succ->uid_ << std::endl;
      }
    }

    // relay predecessor's in-bound message
    if (p_msg_from_pred &&
        p_msg_from_pred->type_ == utils::SelectLeader &&
        p_msg_from_pred->uid_ != uid &&
        p_msg_from_pred->flag_ == utils::In &&
        p_msg_from_pred->hop_count_ == 1) {
      msg_to_succ = *p_msg_from_pred;
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " relay predecessor's in-bound message "
                 << p_msg_from_pred->uid_ << std::endl;
    }

    // relay successor's in-bound message
    if (p_msg_from_succ &&
        p_msg_from_succ->type_ == utils::SelectLeader &&
        p_msg_from_succ->uid_ != uid &&
        p_msg_from_succ->flag_ == utils::In &&
        p_msg_from_succ->hop_count_ == 1) {
      msg_to_pred = *p_msg_from_succ;
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " relay successor's in-bound message "
                 << p_msg_from_succ->uid_ << std::endl;
    }

    // advance to next phrase
    if (p_msg_from_pred &&
        p_msg_from_pred->type_ == utils::SelectLeader &&
        p_msg_from_pred->uid_ == uid &&
        p_msg_from_pred->flag_ == utils::In &&
        p_msg_from_pred->hop_count_ == 1 &&
        p_msg_from_succ &&
        p_msg_from_succ->type_ == utils::SelectLeader &&
        p_msg_from_succ->uid_ == uid &&
        p_msg_from_succ->flag_ == utils::In &&
        p_msg_from_succ->hop_count_ == 1) {
      phrase++;
      msg_to_pred = {utils::SelectLeader, uid, utils::Out, (2U << (phrase - 1))};
      msg_to_succ = {utils::SelectLeader, uid, utils::Out, (2U << (phrase - 1))};
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round
                 << " advance to next phrase "
                 << phrase << std::endl;
    }

    // notify master process that I'm done with this round or I exit
    {
      std::unique_lock<std::mutex> lock(mutex_round_end_);
      if (!round_to_exit) {
        *state = Exited;
      } else {
        *state = RoundEnd;
      }
      round_end_.notify_one();
      debug_clog << lock_with(mutex_log_)
                 << "[process " << id << "] round " << round << " ends" << std::endl;
    }

    round++;
  }
}

void HSAlgo::Master(void) {
  // spawn threads
  for (size_t i = 0; i < thread_num_; i++) {
    process_threads_.emplace_back([this, i] { this->Process(process_ids_.at(i),
                                                            &thread_states_.at(i),
                                                            recv_channels_.at((i + thread_num_ - 1) % thread_num_),
                                                            send_channels_.at((i + thread_num_ - 1) % thread_num_),
                                                            send_channels_.at(i),
                                                            recv_channels_.at(i)); });
  }

  // If there is any process wants to go to next round, we do this.
  // Otherwise, we exits.
  while (std::any_of(thread_states_.cbegin(),
                     thread_states_.cend(),
                     [](const ThreadState& state) { return (state == RoundEnd);})) {
    // signal all non exited processes that this round begins
    {
      std::unique_lock<std::mutex> lock(mutex_round_begin_);
      round_++;
      std::for_each(thread_states_.begin(), thread_states_.end(),
                    [](ThreadState& state) {
                      if (state == RoundEnd) { state = RoundBegin; }});
      debug_clog << lock_with(mutex_log_)
                 << "[master] round " << round_ << " begins" << std::endl;
      round_begin_.notify_all();
    }

    // wait all processes end or exit this round
    {
      std::unique_lock<std::mutex> lock(mutex_round_end_);
      round_end_.wait(lock, [this] {
        return (std::none_of(thread_states_.cbegin(), thread_states_.cend(),
                             [](const ThreadState& state) { return (state == RoundBegin); })); });
      debug_clog << lock_with(mutex_log_)
                 << "[master] round " << round_ << " ends" << std::endl;
    }
  }

  // wait all threads exit
  for (auto& thread : process_threads_) {
    thread.join();
  }
}

} // namespace HS
