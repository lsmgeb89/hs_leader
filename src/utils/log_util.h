#ifndef HS_LEADER_LOG_UTIL_H_
#define HS_LEADER_LOG_UTIL_H_

#include <algorithm>
#include <iomanip>
#include <mutex>

#ifndef NDEBUG
#define debug_clog std::clog
#else
class NullBuffer : public std::streambuf {
public:
  int overflow(int c) { return c; }
};

class NullStream : public std::ostream {
  public:
    NullStream() : std::ostream(&m_sb) {}
  private:
    NullBuffer m_sb;
};
static NullStream null_stream;

#define debug_clog null_stream
#endif

inline std::ostream& operator<<(std::ostream& out,
                                const std::lock_guard<std::mutex> &) {
  return out;
}

template <typename T> inline std::lock_guard<T> lock_with(T &mutex) {
  mutex.lock();
  return { mutex, std::adopt_lock };
}

inline bool IsNumber(const std::string& str) {
  return (!str.empty() && std::find_if(str.begin(), str.end(),
          [](char c) { return !std::isdigit(c); }) == str.end());
}

#endif // HS_LEADER_LOG_UTIL_H_
