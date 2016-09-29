#include <iostream>
#include "HSAlgo.h"
#include "log_util.h"

namespace HS {

void HSAlgo::Run(void) {
  master_thread_ = std::thread([this] { this->Master(); });
  master_thread_.join();
}

void HSAlgo::Process(const std::size_t& id,
                     utils::MessageChannel& send_to_pred_ch,
                     utils::MessageChannel& recv_from_pred_ch,
                     utils::MessageChannel& send_to_succ_ch,
                     utils::MessageChannel& recv_from_succ_ch) {
  std::size_t round_to_exit(2);
  std::size_t succ_to_exit(2);
  std::ptrdiff_t round(0);
  bool pred_exit(false);

  // states
  const std::size_t uid(id);
  utils::Message msg_to_succ{utils::SelectLeader, uid, utils::Out, 1};
  utils::Message msg_to_pred{utils::SelectLeader, uid, utils::Out, 1};
  Status status(Unknown);
  std::size_t phrase(0);

  while (round_to_exit) {
    // round begin
    {
      std::unique_lock<std::mutex> lock(mutex_round_);
      round_begin_.wait(lock, [this, round] { return (round == round_); });
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round << " begins" << std::endl;
    }

    // message-generation
    std::unique_ptr<utils::Message> p_msg_to_succ(nullptr);
    std::unique_ptr<utils::Message> p_msg_to_pred(nullptr);
    if (!pred_exit) {
      p_msg_to_pred.reset(new utils::Message(msg_to_pred));
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
                << " sends to predecessor: " << p_msg_to_pred->ToString() << std::endl;
      send_to_pred_ch.Send(std::move(p_msg_to_pred));
    }

    if (succ_to_exit) {
      p_msg_to_succ.reset(new utils::Message(msg_to_succ));
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
                << " sends to successor: " << p_msg_to_succ->ToString() << std::endl;
      send_to_succ_ch.Send(std::move(p_msg_to_succ));
    }

    // transition
    msg_to_succ = {utils::NullType, 0, utils::Null, 0};
    msg_to_pred = {utils::NullType, 0, utils::Null, 0};
    std::unique_ptr<utils::Message> p_msg_from_succ(nullptr);
    std::unique_ptr<utils::Message> p_msg_from_pred(nullptr);

    if (!pred_exit) {
      recv_from_pred_ch.Receive(p_msg_from_pred);
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
                << " receives from predecessor: " << p_msg_from_pred->ToString() << std::endl;
    }

    if (succ_to_exit) {
      recv_from_succ_ch.Receive(p_msg_from_succ);
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
                << " receives from successor: " << p_msg_from_succ->ToString() << std::endl;
    }

    // dealing with exits
    if (round_to_exit == 1) {
      round_to_exit--;
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
                << " exits" << std::endl;
    }

    if (succ_to_exit == 1) {
      succ_to_exit--;
    }

    // relay predecessor's leader notification to successor
    if (p_msg_from_pred &&
        p_msg_from_pred->type_ == utils::NotifyLeader &&
        p_msg_from_pred->flag_ == utils::Out) {
      pred_exit = true;
      if (status == Leader) {
        succ_to_exit = 0;
        round_to_exit = 0;
      } else {
        msg_to_succ = *p_msg_from_pred;
        round_to_exit--;
      }

      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
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
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " relay predecessor's out-bound message "
                  << p_msg_from_pred->uid_ << std::endl;
      } else if (p_msg_from_pred->uid_ > uid &&
                 p_msg_from_pred->hop_count_ == 1) {
        msg_to_pred = {utils::SelectLeader,
                       p_msg_from_pred->uid_,
                       utils::In,
                       1};
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " turn around predecessor's out-bound message "
                  << p_msg_from_pred->uid_ << std::endl;
      } else if (p_msg_from_pred->uid_ == uid) {
        status = Leader;
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " I'm the leader. In next round, notify successor." << std::endl;
        msg_to_succ = {utils::NotifyLeader, uid, utils::Out, 0};
        succ_to_exit--;
      } else if (p_msg_from_pred->uid_ < uid) {
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
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
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " relay successor's out-bound message "
                  << p_msg_from_succ->uid_ << std::endl;
      } else if (p_msg_from_succ->uid_ > uid &&
                 p_msg_from_succ->hop_count_ == 1) {
        msg_to_succ = {utils::SelectLeader, p_msg_from_succ->uid_, utils::In, 1};
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " turn around successor's out-bound message "
                  << p_msg_from_succ->uid_ << std::endl;
      } else if (p_msg_from_succ->uid_ == uid) {
        status = Leader;
      } else if (p_msg_from_succ->uid_ < uid) {
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
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
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
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
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
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
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
                << " advance to next phrase "
                << phrase << std::endl;
    }

    // round end
    {
      std::unique_lock<std::mutex> lock(mutex_round_end_counter_);
      if (!round_to_exit) {
        process_num_--;
      } else {
        round_end_counter_++;
      }
      round_end_.notify_one();
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round << " ends" << std::endl;
    }

    round++;
  }
}

void HSAlgo::Master(void) {
  // spawn threads
  for (size_t i = 0; i < process_num_; i++) {
    process_threads_.emplace_back([this, i] { this->Process(process_ids_.at(i),
                                                            recv_channels_.at((i + process_num_ - 1) % process_num_),
                                                            send_channels_.at((i + process_num_ - 1) % process_num_),
                                                            send_channels_.at(i),
                                                            recv_channels_.at(i)); });
  }

  while (process_num_) {
    {
      std::unique_lock<std::mutex> lock(mutex_round_);
      round_++;
      round_end_counter_ = 0;
      std::cout << lock_with(mutex_log_)
                << "[m] round " << round_ << " begins" << std::endl;
      round_begin_.notify_all();
    }

    {
      std::unique_lock<std::mutex> lock(mutex_round_end_counter_);
      round_end_.wait(lock, [this] { return (round_end_counter_ == process_num_); });
      std::cout << lock_with(mutex_log_)
                << "[m] round " << round_ << " ends" << std::endl;
    }
  }

  // wait them exit
  for (auto& thread : process_threads_) {
    thread.join();
  }
}

} // namespace HS
