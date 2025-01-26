#ifndef CONNECTIONRAII_H
#define CONNECTIONRAII_H

#include <atomic>

class ConnectionRAII {
public:
  ConnectionRAII(std::atomic<int> &connections);
  ~ConnectionRAII();

private:
  std::atomic<int> &_connections_;
};

#endif // CONNECTIONRAII_H
