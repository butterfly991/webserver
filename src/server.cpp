#include "Server.hpp"
#include "ConnectionRAII.hpp"
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

Server::Server(boost::asio::io_context &io_context, short port,
               int thread_count, const std::string &static_file_dir)
    : _acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      _thread_pool_(thread_count), _request_handler_(static_file_dir),
      _current_connections_(0) {
  StartAccept(io_context);
}

void Server::StartAccept(boost::asio::io_context &io_context) {
  auto socket = std::make_shared<tcp::socket>(io_context);
  _acceptor_.async_accept(*socket, [this, socket, &io_context](
                                       const boost::system::error_code &error) {
    if (!error) {
      ConnectionRAII connection_guard(_current_connections_);
      HandleRequest(std::move(*socket));
    }
    StartAccept(io_context);
  });
}

void Server::HandleRequest(tcp::socket socket) {
  boost::asio::post(
      _thread_pool_, [this, socket = std::move(socket)]() mutable {
        try {
          std::string response = _request_handler_.HandleRequest(socket);
          boost::asio::write(socket, boost::asio::buffer(response));
          socket.close();
        } catch (const std::exception &ex) {
          std::cerr << "Error in request handling: " << ex.what() << std::endl;
        }
      });
}
