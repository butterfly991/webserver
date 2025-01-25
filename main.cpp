#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

using boost::asio::ip::tcp;

struct ServerStats {
  std::atomic<int> request_count{0};
  std::atomic<int> active_threads{0};
  std::time_t start_time = std::time(nullptr);
};

class ConnectionRAII {
public:
  ConnectionRAII(std::atomic<int> &connections) : _connections_(connections) {
    ++_connections_;
  }

  ~ConnectionRAII() { --_connections_; }

private:
  std::atomic<int> &_connections_;
};

class ReQuestHandler {
public:
  ReQuestHandler(const std::string &static_file_dir)
      : _static_file_dir_(static_file_dir), _server_stats_() {
    if (!boost::filesystem::exists(_static_file_dir_)) {
      throw std::runtime_error("Static file directory does not exist: " +
                               _static_file_dir_);
    }
  }

  std::string HandleRequest(tcp::socket &socket) {
    try {
      char buffer[1024];
      boost::system::error_code error;
      size_t length = socket.read_some(boost::asio::buffer(buffer), error);

      if (error && error != boost::asio::error::eof) {
        throw boost::system::system_error(error);
      }

      std::string request(buffer, length);
      LogRequest(request, socket.remote_endpoint());
      _server_stats_.request_count++;

      std::map<std::string, std::string> headers = HeadersParse(request);
      std::string url = URL_DECODE_CLEAN(ParseURL(request));
      std::string body = GetRequestBody(request);
      std::string method = Parse(request);

      if (_routes_.count(url)) {
        return _routes_[url](body, headers, method);
      } else {
        std::string static_file_response = ServeStaticFile(url);
        if (!static_file_response.empty()) {
          return static_file_response;
        } else {
          return GenerateResponse(404, "404 Not Found",
                                  "Static file not found");
        }
      }
    } catch (const std::exception &ex) {
      std::cerr << "Error in request handler: " << ex.what() << std::endl;
      return GenerateResponse(500, "500 Internal Server Error", "Server error");
    }
  }

  void RegisterRoute(
      const std::string &path,
      const std::function<std::string(
          const std::string &, const std::map<std::string, std::string> &,
          const std::string &)> &handler) {
    _routes_[path] = handler;
  }

private:
  void LogRequest(const std::string &request, const tcp::endpoint &endpoint) {
    std::lock_guard<std::mutex> lock(_log_mutex_);
    std::string log_file_name = "server_log.txt";
    std::ofstream log_file(log_file_name, std::ios_base::app);
    auto time =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    log_file << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
             << "] Client: " << endpoint.address().to_string() << "\n";
    log_file << "Request: " << request << "\n\n";
  }

  std::string Parse(const std::string &request) {
    std::istringstream stream(request);
    std::string method;
    stream >> method;
    return method;
  }

  std::map<std::string, std::string> HeadersParse(const std::string &request) {
    std::map<std::string, std::string> headers;
    std::istringstream stream(request);
    std::string line;
    while (std::getline(stream, line) && line != "\r") {
      auto pos = line.find(": ");
      if (pos != std::string::npos) {
        headers[line.substr(0, pos)] = line.substr(pos + 2);
      }
    }
    return headers;
  }

  std::string ParseURL(const std::string &request) {
    std::istringstream stream(request);
    std::string method;
    std::string url;
    stream >> method >> url;
    return url;
  }

  std::string URL_DECODE_CLEAN(const std::string &url) {
    std::string decoded;
    char hexagon[3] = {0};
    for (size_t i = 0; i < url.size(); ++i) {
      if (url[i] == '%' && i + 2 < url.size()) {
        hexagon[0] = url[i + 1];
        hexagon[1] = url[i + 2];
        decoded += static_cast<char>(std::strtol(hexagon, nullptr, 16));
        i += 2;
      } else if (url[i] == '+') {
        decoded += ' ';
      } else {
        decoded += url[i];
      }
    }
    return decoded;
  }

  std::string GetRequestBody(const std::string &request) {
    auto pos = request.find("\r\n\r\n");
    if (pos != std::string::npos) {
      return request.substr(pos + 4);
    }
    return "";
  }

  std::string GenerateResponse(int status_code,
                               const std::string &status_message,
                               const std::string &body) {
    std::string response = "HTTP/1.1 " + std::to_string(status_code) + " " +
                           status_message + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    return response;
  }

  std::string ServeStaticFile(const std::string &url) {
    std::string file_path = _static_file_dir_ + url;

    if (file_path.back() == '/') {
      file_path += "index.html";
    }

    boost::filesystem::path path(file_path);
    if (boost::filesystem::exists(path) &&
        boost::filesystem::is_regular_file(path)) {
      std::ifstream file(file_path, std::ios::binary);
      std::ostringstream buffer;
      buffer << file.rdbuf();
      std::string content = buffer.str();

      return GenerateResponse(200, "OK", content);
    }

    std::cerr << "File not found: " << file_path << std::endl;
    return "";
  }

  std::string _static_file_dir_;
  std::mutex _log_mutex_;
  std::map<std::string,
           std::function<std::string(const std::string &,
                                     const std::map<std::string, std::string> &,
                                     const std::string &)>>
      _routes_;
  ServerStats _server_stats_;
};

class Server {
public:
  Server(boost::asio::io_context &io_context, short port, int thread_count,
         const std::string &static_file_dir)
      : _acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        _thread_pool_(thread_count), _request_handler_(static_file_dir),
        _current_connections_(0) {
    StartAccept(io_context);
  }

private:
  void StartAccept(boost::asio::io_context &io_context) {
    auto socket = std::make_shared<tcp::socket>(io_context);
    _acceptor_.async_accept(
        *socket,
        [this, socket, &io_context](const boost::system::error_code &error) {
          if (!error) {
            ConnectionRAII connection_guard(_current_connections_);
            HandleRequest(std::move(*socket));
          }
          StartAccept(io_context);
        });
  }

  void HandleRequest(tcp::socket socket) {
    boost::asio::post(_thread_pool_, [this,
                                      socket = std::move(socket)]() mutable {
      try {
        std::string response = _request_handler_.HandleRequest(socket);
        boost::asio::write(socket, boost::asio::buffer(response));
        socket.close();
      } catch (const std::exception &ex) {
        std::cerr << "Error in request handling: " << ex.what() << std::endl;
      }
    });
  }

  tcp::acceptor _acceptor_;
  boost::asio::thread_pool _thread_pool_;
  ReQuestHandler _request_handler_;
  std::atomic<int> _current_connections_;
};

int main(int argc, char *argv[]) {
  try {
    boost::program_options::options_description options("Allowed options");
    options.add_options()("help", "produce help message")(
        "port", boost::program_options::value<int>()->default_value(8080),
        "set port")("threads",
                    boost::program_options::value<int>()->default_value(4),
                    "set thread count")(
        "static",
        boost::program_options::value<std::string>()->default_value("static"),
        "set static file directory");

    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, options), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
      std::cout << options << std::endl;
      return 0;
    }

    int port = vm["port"].as<int>();
    int threads = vm["threads"].as<int>();
    std::string static_dir = vm["static"].as<std::string>();

    boost::asio::io_context io_context;
    Server server(io_context, port, threads, static_dir);
    std::cout << "Server started on http://localhost:" << port << std::endl;
    io_context.run();
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
  }

  return 0;
}
