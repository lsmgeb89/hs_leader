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
  bool done(false);
  std::size_t round(0);

  // states
  const std::size_t uid(id);
  utils::Message msg_to_succ{false, uid, utils::Out, 1};
  utils::Message msg_to_pred{false, uid, utils::Out, 1};
  Status status(Unknown);
  std::size_t phrase(0);

  while (!done) {
    // round begin
    {
      std::unique_lock<std::mutex> lock(mutex_round_);
      round_begin_.wait(lock, [this, round] { return (round == round_); });
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round << " begins" << std::endl;
    }

    // message-generation
    std::unique_ptr<utils::Message> p_msg_to_succ(new utils::Message(msg_to_succ));
    std::unique_ptr<utils::Message> p_msg_to_pred(new utils::Message(msg_to_pred));
    std::cout << lock_with(mutex_log_)
              << "[" << id << "] round " << round
              << " sends to predecessor: " << p_msg_to_pred->ToString() << std::endl;
    std::cout << lock_with(mutex_log_)
              << "[" << id << "] round " << round
              << " sends to successor: " << p_msg_to_succ->ToString() << std::endl;
    send_to_succ_ch.Send(std::move(p_msg_to_succ));
    send_to_pred_ch.Send(std::move(p_msg_to_pred));

    // transition
    msg_to_succ = {true, 0, utils::Null, 0};
    msg_to_pred = {true, 0, utils::Null, 0};
    std::unique_ptr<utils::Message> p_msg_from_succ(nullptr);
    std::unique_ptr<utils::Message> p_msg_from_pred(nullptr);
    recv_from_pred_ch.Receive(p_msg_from_pred);
    recv_from_succ_ch.Receive(p_msg_from_succ);
    std::cout << lock_with(mutex_log_)
              << "[" << id << "] round " << round
              << " receives from predecessor: " << p_msg_from_pred->ToString() << std::endl;
    std::cout << lock_with(mutex_log_)
              << "[" << id << "] round " << round
              << " receives from successor: " << p_msg_from_succ->ToString() << std::endl;

    // relay predecessor's out-bound message
    if (!p_msg_from_pred->is_null_ &&
        p_msg_from_pred->flag_ == utils::Out) {
      if (p_msg_from_pred->uid_ > uid &&
          p_msg_from_pred->hop_count_ > 1) {
        msg_to_succ = {false,
                       p_msg_from_pred->uid_,
                       utils::Out,
                       p_msg_from_pred->hop_count_ - 1};
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " relay predecessor's out-bound message "
                  << p_msg_from_pred->uid_ << std::endl;
      } else if (p_msg_from_pred->uid_ > uid &&
                 p_msg_from_pred->hop_count_ == 1) {
        msg_to_pred = {false, p_msg_from_pred->uid_, utils::In, 1};
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " turn around predecessor's out-bound message "
                  << p_msg_from_pred->uid_ << std::endl;
      } else if (p_msg_from_pred->uid_ == uid) {
        status = Leader;
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " I'm the leader" << std::endl;
      } else if (p_msg_from_pred->uid_ < uid) {
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " discards predecessor's out-bound message "
                  << p_msg_from_pred->uid_ << std::endl;
      }
    }

    // relay successor's out-bound message
    if (!p_msg_from_succ->is_null_ &&
        p_msg_from_succ->flag_ == utils::Out) {
      if (p_msg_from_succ->uid_ > uid &&
          p_msg_from_succ->hop_count_ > 1) {
        msg_to_pred = {false,
                       p_msg_from_succ->uid_,
                       utils::Out,
                       p_msg_from_succ->hop_count_ - 1};
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " relay successor's out-bound message "
                  << p_msg_from_succ->uid_ << std::endl;
      } else if (p_msg_from_succ->uid_ > uid &&
                 p_msg_from_succ->hop_count_ == 1) {
        msg_to_succ = {false, p_msg_from_succ->uid_, utils::In, 1};
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " turn around successor's out-bound message "
                  << p_msg_from_succ->uid_ << std::endl;
      } else if (p_msg_from_succ->uid_ == uid) {
        status = Leader;
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " I'm the leader" << std::endl;
      } else if (p_msg_from_succ->uid_ < uid) {
        std::cout << lock_with(mutex_log_)
                  << "[" << id << "] round " << round
                  << " discards successor's out-bound message "
                  << p_msg_from_succ->uid_ << std::endl;
      }
    }

    // relay predecessor's in-bound message
    if (!p_msg_from_pred->is_null_ &&
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
    if (!p_msg_from_succ->is_null_ &&
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
    if (!p_msg_from_pred->is_null_ &&
        p_msg_from_pred->uid_ == uid &&
        p_msg_from_pred->flag_ == utils::In &&
        p_msg_from_pred->hop_count_ == 1 &&
        !p_msg_from_succ->is_null_ &&
        p_msg_from_succ->uid_ == uid &&
        p_msg_from_succ->flag_ == utils::In &&
        p_msg_from_succ->hop_count_ == 1) {
      phrase++;
      msg_to_pred = {false, uid, utils::Out, (2U << (phrase - 1))};
      msg_to_succ = {false, uid, utils::Out, (2U << (phrase - 1))};
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round
                << " advance to next phrase "
                << phrase << std::endl;
    }

    // round end
    {
      std::unique_lock<std::mutex> lock(mutex_round_end_counter_);
      round_end_counter_++;
      round_end_.notify_one();
      std::cout << lock_with(mutex_log_)
                << "[" << id << "] round " << round << " ends" << std::endl;
    }

    round++;
  }
}

void HSAlgo::Master(void) {
  bool done(false);

  // spawn threads
  for (size_t i = 0; i < process_num_; i++) {
    process_threads_.emplace_back([this, i] { this->Process(process_ids_.at(i),
                                                            recv_channels_.at((i + process_num_ - 1) % process_num_),
                                                            send_channels_.at((i + process_num_ - 1) % process_num_),
                                                            send_channels_.at(i),
                                                            recv_channels_.at(i)); });
  }

  while (!done) {
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

    // TODO: exit condition
    if (round_ == 50) {
      done = true;
    }
  }

  // wait them exit
  for (auto& thread : process_threads_) {
    thread.join();
  }
}

} // namespace HS
