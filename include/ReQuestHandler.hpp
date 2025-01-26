#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <atomic>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <string>

class ReQuestHandler {
public:
  ReQuestHandler(const std::string &static_file_dir);

  std::string HandleRequest(boost::asio::ip::tcp::socket &socket);

  void RegisterRoute(
      const std::string &path,
      const std::function<std::string(
          const std::string &, const std::map<std::string, std::string> &,
          const std::string &)> &handler);

private:
  void LogRequest(const std::string &request,
                  const boost::asio::ip::tcp::endpoint &endpoint);
  std::string Parse(const std::string &request);
  std::map<std::string, std::string> HeadersParse(const std::string &request);
  std::string ParseURL(const std::string &request);
  std::string URL_DECODE_CLEAN(const std::string &url);
  std::string GetRequestBody(const std::string &request);
  std::string GenerateResponse(int status_code,
                               const std::string &status_message,
                               const std::string &body);
  std::string ServeStaticFile(const std::string &url);

  std::string _static_file_dir_;
  std::mutex _log_mutex_;
  std::map<std::string,
           std::function<std::string(const std::string &,
                                     const std::map<std::string, std::string> &,
                                     const std::string &)>>
      _routes_;
  struct ServerStats {
    std::atomic<int> request_count{0};
    std::atomic<int> active_threads{0};
    std::time_t start_time = std::time(nullptr);
  };
  ServerStats _server_stats_;
};

#endif // REQUEST_HANDLER_HPP
