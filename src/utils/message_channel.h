#ifndef HS_LEADER_MESSAGE_CHANNEL_H_
#define HS_LEADER_MESSAGE_CHANNEL_H_

#include <condition_variable>
#include <mutex>
#include "message.h"

namespace utils {

class MessageChannel {
 public:
  MessageChannel(void)
    : msg_(nullptr) {}

  void Send(std::unique_ptr<Message> msg) {
    std::unique_lock<std::mutex> lock(mutex_msg_);
    msg_ = std::move(msg);
    msg_sent_.notify_one();
  }

  void Receive(std::unique_ptr<Message>& msg) {
    std::unique_lock<std::mutex> lock(mutex_msg_);
    msg_sent_.wait(lock, [this] { return (msg_.get()); });
    msg = std::move(msg_);
    msg_ = nullptr;
  }

 private:
  mutable std::mutex mutex_msg_;
  std::unique_ptr<Message> msg_;
  std::condition_variable msg_sent_;
};

} // namespace utils

#endif // HS_LEADER_MESSAGE_CHANNEL_H_
