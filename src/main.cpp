#include "Server.hpp"
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>

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
