#include  <iostream>
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/ssl.hpp>
#include <string>
#include <memory>
#include <fstream>
#include <thread>
#include <map>
#include <functional>
#include <sstream>
#include <ctime>
#include <mutex>
#include <atomic>
#include <iomanip>

using boost::asio::ip::tcp;

struct ServerStats {
	std::atomic<int> request_count{ 0 };
	std::atomic<int> active_threads{ 0 };
	std::time_t strat_time = std::time(nullptr);
};

class ConnectionRAII {
public:
	ConnectionRAII(std::atomic<int>& connections) : _connections_(connections) {
		++_connections_;
	}

	~ConnectionRAII() {
		--_connections_;
	}

private:
	std::atomic<int>& _connections_;
};

class ReQuestHandler {
public:
	ReQuestHandler(const std::string& static_file_dir)
	: _static_file_dir_( static_file_dir), _server_stats_() {
		if (!boost::filesystem::exists(_static_file_dir_)) 
		{
			throw std::runtime_error("Static file directory does not exist: " + _static_file_dir_);
		}
	}

	std::string HandleRequest(tcp::socket& socket) {
		try
		{
			char buffer[1024];
			boost::system::error_code error;
			size_t length = socket.read_some(boost::asio::buffer(buffer), error);

			if (error && error != boost::asio::error::eof){
				throw boost::system::system_error(error);
			}
			
			std::string request(buffer, length);
			LogRequest(request,socket.remote_endpoint());
			_server_stats_.request_count++;
			
			std::map<std::string, std::string> headers = HeadersParse(request);
			std::string url = URL_DECODE_CLEAN(ParseURL(request));
			std::string body = GetRequestBody(request);
			std::string method = Parse(request);
			
			if (_routes_.count(url))
			{
				return _routes_[url](body, headers, method);
			} 
			else
			{
				std::string static_file_response = ServerStaticFile(url);
				if (!static_file_response.empty())
				{
					return static_file_response;
				}
				else
				{
					return GenerateResponce(404, "404 Not Found", "Static file not found");
				}
			}
		}
		catch (const std::exception&ex)
		{
			std::cerr << "Error in request handler: " << ex.what()<<std::endl;
			return GenerateResponce(500, "500 Internal Server Error", "Server file not found");
		}
	}

	void RegisterRoute(const std::string& path,
		const std::function<std::string(const std::string&,
			const std::map<std::string, std::string>&,
			const std::string&)>& handler) {
		_routes_[path] = handler;
	}
private:
	void LogRequest(const std::string& request, const tcp::endpoint& endpoint) {
		std::lock_guard<std::mutex> locak(_log_mutex_);
		std::string _log_file_name_ = "server_log.txt";
		std::ofstream file_logg(_log_file_name_, std::ios_base::app);
		auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		file_logg << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:$M:$S")
		<<"] Client: " << endpoint.address().to_string() << "\n";
		file_logg << "Request: " << request << "\n\n";
	}
	
	std::string Parse(const std::string& request) {
		std::istringstream stream(request);
		std::string way;
		stream >> way;
		return way;
	}

	std::map<std::string, std::string> HeadersParse(const std::string& request) {
		std::map<std::string, std::string>headers;
		std::istringstream stream(request);
		std::string line;
		while (std::getline(stream,line) && line != "\r")
		{
			auto pos = line.find(": ");
			if (pos !=std::string::npos)
			{
				headers[line.substr(0, pos)] = line.substr(pos + 2);
			}
		}
		return headers;
	}

	std::string ParseURL (const std::string& request) {
		std::istringstream stream(request);
		std::string way;
		std::string url;
		stream >> way >> url;
		return url;
	}

	std::string URL_DECODE_CLEAN(const std::string& url) {
		std::string decoded;
		char hexagon[3] = { 0 };
		for (size_t i = 0; i < url.size(); ++i)
		{
			if (url[i] == '%' && i + 2 < url.size())
			{
				hexagon[0] = url[i + 1];
				hexagon[1] = url[i + 2];
				decoded += static_cast<char>(std::strtol(hexagon, nullptr, 16));
				i += 2;
			}
			else if (url[i] == '+')
			{
				decoded += ' ';
			}
			else 
			{
				decoded += url[i];
			}
		}
		return decoded;
	}

	std::string GetRequestBody(const std::string& request) {
		auto pos = request.find("\r\n\r\n");
		if (pos !=std::string::npos)
		{
			return  request.substr(pos + 4);
		}
		return "";
	}

	std::string GenerateResponce(int _status_code_, const std::string& body ,const std::string& _status_message_) {
		std::string response = "HTTP/1.1 " + std::to_string(_status_code_) + " " + _status_message_ + "\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
		response += "connection: close\r\n\r\n";
		response += body;
		return response;
	}

	std::string ServerStaticFile(const std::string& url) {
		if (_cache_file_.count(url))
		{
			return _cache_file_[url];
		}

		boost::filesystem::path _file_path_ = _static_file_dir_ + url;
		if (boost::filesystem::exists(_file_path_) && boost::filesystem::is_regular_file(_file_path_))
		{
			std::ifstream file(_file_path_.string(), std::ios::binary);
			std::ostringstream Qui;
			Qui << file.rdbuf();
			std::string content = Qui.str();

			std::string response = GenerateResponce(200, "OK", content);
			if (_cache_file_.size()<max_cache_size)
			{
				_cache_file_[url] = response;
			}
			return response;
		}
		return GenerateResponce(404, "404 Not Found", "Static file not found");
	}
	std::string _static_file_dir_;
	std::mutex _log_mutex_;
	std::map<std::string, std::function<std::string(const std::string&, const std::map<std::string, std::string>&, const std::string&)>> _routes_;//endpoints
	std::map<std::string, std::string> _cache_file_;
	const size_t max_cache_size = 50;
	ServerStats _server_stats_;
};

class Server{
public:
	Server(boost::asio::io_context& io_context, short port, int thread_count, const std::string& static_file_dir)
		: _acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), _thread_pool_(thread_count), _reQuest_handler_(static_file_dir), _current_connections_(0) {
		StartAccept(); 
	} 
private:
	void StartAccept() {
		auto socket = std::make_shared<tcp::socket>(_acceptor_.get_executor().context());
		_acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
			if (!error)
			{
				ConnectionRAII connection_protection(_current_connections_);
				HandleRequest(std::move(*socket));
			}
			StartAccept();
			});
	}

	void HandleRequest(tcp::socket socket) {
		boost::asio::post(_thread_pool_, [this, socket = std::move(socket)]() mutable {
			try
			{
				std::string response = _reQuest_handler_.HandleRequest(socket);
				boost::asio::write(socket, boost::asio::buffer(response));
				socket.shutdown(tcp::socket::shutdown_both);
				socket.close(); 
			}
			catch (const std::exception& ex)
			{
				std::cerr << "Error in request handling: " << ex.what() << std::endl; 
			}
			});;
	}

	tcp::acceptor _acceptor_;
	boost::asio::thread_pool _thread_pool_;
	ReQuestHandler _reQuest_handler_;
	std::atomic<int> _current_connections_;
};
int main(int argc,char* argv[]) {
	try
	{
		boost::program_options::options_description command_arguments ("Allowed options");
		command_arguments.add_options()
		("help", "prodoce help message")
		("port", boost::program_options::value<int>()->default_value(8080), "set port")
		("threads", boost::program_options::value<int>()->default_value(4), "set thread count")
		("static", boost::program_options::value<std::string>()->default_value("static"), "set static file directory");

		boost::program_options::variables_map QV;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, command_arguments), QV);
		boost::program_options::notify(QV);

		if (QV.count("help"))
		{
			std::cout << command_arguments << std::endl;
			return 0;
		}

		int port = QV["port"].as<int>();
		int threads = QV["thread"].as<int>();
		std::string staticDIR = QV["static"].as<std::string>();

		boost::asio::io_context io_context;
		Server server(io_context, port, threads, staticDIR);
		std::cout << "Enjoy your use!\n" << "(//>//<//)" << std::endl;
		std::cout << "Server started on http://localhost:" << port << std::endl;
		io_context.run();
	}
	catch (const std::exception&ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}
	return 0;
}




