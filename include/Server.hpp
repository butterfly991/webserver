#ifndef SERVER_H
#define SERVER_H

#include "ReQuestHandler.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <memory>

class Server {
public:
  Server(boost::asio::io_context &io_context, short port, int thread_count,
         const std::string &static_file_dir);

private:
  void StartAccept(boost::asio::io_context &io_context);
  void HandleRequest(boost::asio::ip::tcp::socket socket);

  boost::asio::ip::tcp::acceptor _acceptor_;
  boost::asio::thread_pool _thread_pool_;
  ReQuestHandler _request_handler_;
  std::atomic<int> _current_connections_;
};

#endif // SERVER_H
