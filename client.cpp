#include <iostream>
#include <chrono>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>

namespace bpo = boost::program_options;
namespace chron = std::chrono;
using boost::asio::ip::tcp;

#define SECONDS 5

boost::asio::io_context io_context;
tcp::resolver resolver(io_context);
tcp::socket tcp_socket(io_context);

std::array<char, 4096> bytes;

void read_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) {
    if (!ec) {
        std::cout.write(bytes.data(), bytes_transferred);
    }
}

void connect_handler(const boost::system::error_code &ec) {
    if (!ec) {
        /* send the message */    
        uint32_t request_id = 1223;
        uint64_t unix_timestamp = chron::duration_cast<chron::seconds>(chron::system_clock::now().time_since_epoch()).count() + SECONDS;
        std::string cookie_data = "Wake up lovely service";
        uint32_t cookie_size = cookie_data.length();

        uint32_t low = (uint32_t)(unix_timestamp >> 32);
        uint32_t high = (uint32_t)unix_timestamp;

        std::cout << "Timestamp: " << unix_timestamp << std::endl;

        std::vector<boost::asio::const_buffer> header;
        header.push_back(boost::asio::buffer(&request_id, sizeof(uint32_t))); 
        header.push_back(boost::asio::buffer(&low, sizeof(uint32_t))); 
        header.push_back(boost::asio::buffer(&high, sizeof(uint32_t))); 
        header.push_back(boost::asio::buffer(&cookie_size, sizeof(uint32_t))); 

        /* Write header */
        boost::asio::write(tcp_socket, header);
        boost::asio::write(tcp_socket, boost::asio::buffer(cookie_data));

        tcp_socket.async_read_some(boost::asio::buffer(bytes), read_handler);
    }
}

void resolve_handler(const boost::system::error_code &ec, tcp::resolver::iterator it) {
    if (!ec) {
        tcp_socket.async_connect(*it, connect_handler);
    }
}

int main(int argc, const char *argv[]) {
    
    try {

        /* reading cli arguments */
        bpo::options_description description("List of options");
        description.add_options()
            ("help,h", "prints help message")
            ("server,s", bpo::value<std::string>(), "adress of the timer server")        
            ("port,p", bpo::value<std::string>(), "port the server is listeing on")
            ("messages,m", bpo::value<int>(), "number of messages to send async")
        ;

        bpo::variables_map vm;
        bpo::store(bpo::parse_command_line(argc, argv, description), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            std::cout << description << std::endl;
            return 0;
        }

        if (!vm.count("server") || !vm.count("port")) {
            std::cout << "Please provide a server and a port!" << std::endl;
            std::cout << description << std::endl;
            return 1;
        }

        /* start the io context and send the messages */ 
        std::cout << "Connecting to Server: " << vm["server"].as<std::string>() << ":" << vm["port"].as<std::string>() << std::endl;

        tcp::resolver::query q{vm["server"].as<std::string>(), vm["port"].as<std::string>()};
        resolver.async_resolve(q, resolve_handler);
        io_context.run();

    } catch (std::exception &e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }
    
    return 0;
}
