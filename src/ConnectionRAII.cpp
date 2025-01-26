#include "ConnectionRAII.hpp"

ConnectionRAII::ConnectionRAII(std::atomic<int> &connections)
    : _connections_(connections) {
  ++_connections_;
}

ConnectionRAII::~ConnectionRAII() { --_connections_; }
