#ifndef HS_LEADER_HSALGO_H_
#define HS_LEADER_HSALGO_H_

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include "message_channel.h"

namespace HS {

enum ThreadState {
  RoundBegin = 0, // Master notifies all processes that this round begins
  RoundEnd = 1,   // A process has done all the things in this round and wants to go to next round
  Exited = 2      // A process has done all the things in this round and exits in this round
};

enum Status {
  Unknown = 0,
  Leader = 1
};

class HSAlgo {
 public:
  HSAlgo(const std::vector<std::size_t>& process_ids)
    : round_(-1),
      thread_states_(process_ids.size(), RoundEnd),
      thread_num_(process_ids.size()),
      process_ids_(process_ids),
      send_channels_(thread_num_),
      recv_channels_(thread_num_) {
  }

  void Run(void);

 private:
  void Process(const std::size_t& id,
               ThreadState* const state,
               utils::MessageChannel& send_to_pred_ch,
               utils::MessageChannel& recv_from_pred_ch,
               utils::MessageChannel& send_to_succ_ch,
               utils::MessageChannel& recv_from_succ_ch);

  void Master(void);

 private:
  // round begin
  mutable std::mutex mutex_round_begin_;
  std::ptrdiff_t round_;
  std::condition_variable round_begin_;

  // round end
  mutable std::mutex mutex_round_end_;
  std::vector<ThreadState> thread_states_;
  std::condition_variable round_end_;

  // threads
  std::size_t thread_num_;
  const std::vector<std::size_t> process_ids_;
  std::thread master_thread_;
  std::vector<std::thread> process_threads_;

  // message channel
  std::vector<utils::MessageChannel> send_channels_;
  std::vector<utils::MessageChannel> recv_channels_;

  // log
  mutable std::mutex mutex_log_;
};

} // namespace HS

#endif // HS_LEADER_HSALGO_H_
