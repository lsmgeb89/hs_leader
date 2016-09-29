#ifndef HS_LEADER_MESSAGE_H_
#define HS_LEADER_MESSAGE_H_

#include <string>
#include <sstream>

namespace utils {

enum Flag {
  Null = 0,
  In = 1,
  Out = 2
};

enum MessageType {
  NullType = 0,
  SelectLeader = 1,
  NotifyLeader = 2
};

struct Message {
  MessageType type_;
  std::size_t uid_;
  Flag flag_;
  std::size_t hop_count_;

  std::string ToString(void) {
    std::stringstream out;
    if (NullType == type_) {
      out << "null message";
    } else if (SelectLeader == type_) {
      out << "uid = " << uid_;
      if (flag_ == In) {
        out << " in-bound";
      } else {
        out << " out-bound";
      }
      out << " hop = " << hop_count_;
    } else {
      out << "notify leader: " << uid_;
    }
    return out.str();
  }
};

} // namespace utils

#endif // HS_LEADER_MESSAGE_H_
