#include "ReQuestHandler.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

ReQuestHandler::ReQuestHandler(const std::string &static_file_dir)
    : _static_file_dir_(static_file_dir), _server_stats_() {
  if (!boost::filesystem::exists(_static_file_dir_)) {
    throw std::runtime_error("Static file directory does not exist: " +
                             _static_file_dir_);
  }
}

std::string
ReQuestHandler::HandleRequest(boost::asio::ip::tcp::socket &socket) {
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
        return GenerateResponse(404, "404 Not Found", "Static file not found");
      }
    }
  } catch (const std::exception &ex) {
    std::cerr << "Error in request handler: " << ex.what() << std::endl;
    return GenerateResponse(500, "500 Internal Server Error", "Server error");
  }
}

void ReQuestHandler::LogRequest(
    const std::string &request,
    const boost::asio::ip::tcp::endpoint &endpoint) {
  std::lock_guard<std::mutex> lock(_log_mutex_);
  std::string log_file_name = "server_log.txt";
  std::ofstream log_file(log_file_name, std::ios_base::app);
  auto time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  log_file << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
           << "] Client: " << endpoint.address().to_string() << "\n";
  log_file << "Request: " << request << "\n\n";
}

std::string ReQuestHandler::Parse(const std::string &request) {
  std::istringstream stream(request);
  std::string method;
  stream >> method;
  return method;
}

std::map<std::string, std::string>
ReQuestHandler::HeadersParse(const std::string &request) {
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

std::string ReQuestHandler::ParseURL(const std::string &request) {
  std::istringstream stream(request);
  std::string method;
  std::string url;
  stream >> method >> url;
  return url;
}

std::string ReQuestHandler::URL_DECODE_CLEAN(const std::string &url) {
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

std::string ReQuestHandler::GetRequestBody(const std::string &request) {
  auto pos = request.find("\r\n\r\n");
  if (pos != std::string::npos) {
    return request.substr(pos + 4);
  }
  return "";
}

std::string ReQuestHandler::GenerateResponse(int status_code,
                                             const std::string &status_message,
                                             const std::string &body) {
  std::string response =
      "HTTP/1.1 " + std::to_string(status_code) + " " + status_message + "\r\n";
  response += "Content-Type: text/html\r\n";
  response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
  response += "Connection: close\r\n\r\n";
  response += body;
  return response;
}

std::string ReQuestHandler::ServeStaticFile(const std::string &url) {
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

void ReQuestHandler::RegisterRoute(
    const std::string &path,
    const std::function<std::string(const std::string &,
                                    const std::map<std::string, std::string> &,
                                    const std::string &)> &handler) {
  _routes_[path] = handler;
}
