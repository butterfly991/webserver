#ifndef SERVERSTATS_H
#define SERVERSTATS_H

#include <atomic>
#include <ctime>

struct ServerStats {
  std::atomic<int> request_count{0};
  std::atomic<int> active_threads{0};
  std::time_t start_time = std::time(nullptr);
};

#endif // SERVERSTATS_H
