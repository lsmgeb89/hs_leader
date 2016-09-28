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

struct Message {
  bool is_null_;
  std::size_t uid_;
  Flag flag_;
  std::size_t hop_count_;
  std::string ToString(void) {
    std::stringstream out;
    if (is_null_) {
      out << "null message";
    } else {
      out << "uid = " << uid_;
      if (flag_ == In) {
        out << " in-bound";
      } else {
        out << " out-bound";
      }
      out << " hop = " << hop_count_;
    }
    return out.str();
  }
};

} // namespace utils

#endif // HS_LEADER_MESSAGE_H_
