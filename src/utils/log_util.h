#ifndef HS_LEADER_LOG_UTIL_H_
#define HS_LEADER_LOG_UTIL_H_

#include <algorithm>
#include <iomanip>
#include <mutex>

inline std::ostream& operator<<(std::ostream& out,
                                const std::lock_guard<std::mutex> &) {
  return out;
}

template <typename T> inline std::lock_guard<T> lock_with(T &mutex) {
  mutex.lock();
  return { mutex, std::adopt_lock };
}

#endif // HS_LEADER_LOG_UTIL_H_
