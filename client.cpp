#include <iostream>
#include <chrono>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#define SECONDS 5

using boost::asio::ip::tcp;
namespace bpo = boost::program_options;
namespace chron = std::chrono;

bpo::variables_map vm;

boost::asio::io_context io_context;
tcp::resolver resolver(io_context);
tcp::socket tcp_socket(io_context);

/* buffers */
std::vector<uint32_t> inbound_header{0,0};
std::string inbound_message;

void read_message(const boost::system::error_code &ec, std::size_t bytes_transferred) {
    if (!ec) {
        std::cout << "Response: " << inbound_message << std::endl;
    }
}

void read_header(const boost::system::error_code &ec, std::size_t bytes_transferred) {
    if (!ec) {
        /* read message */
        inbound_message.resize(inbound_header[1]);
        boost::asio::async_read(tcp_socket, boost::asio::buffer(inbound_message, inbound_header[1]), read_message);
    }
}

void connect_handler(const boost::system::error_code &ec) {
    if (!ec) {

        /* send the messages */    
        uint32_t request_id = 1223;
        uint64_t unix_timestamp = chron::duration_cast<chron::seconds>(chron::system_clock::now().time_since_epoch()).count() + SECONDS;
        std::string cookie_data = "Wake me up";
        uint32_t cookie_size = cookie_data.length();

        uint32_t low = (uint32_t)(unix_timestamp >> 32);
        uint32_t high = (uint32_t)unix_timestamp;

        std::vector<boost::asio::const_buffer> header;
        header.push_back(boost::asio::buffer(&request_id, sizeof(uint32_t))); 
        header.push_back(boost::asio::buffer(&low, sizeof(uint32_t))); 
        header.push_back(boost::asio::buffer(&high, sizeof(uint32_t))); 
        header.push_back(boost::asio::buffer(&cookie_size, sizeof(uint32_t))); 

        /* send header */
        boost::asio::write(tcp_socket, header);
        /* send message */
        boost::asio::write(tcp_socket, boost::asio::buffer(cookie_data));

        /* read response */
        boost::asio::async_read(tcp_socket, boost::asio::buffer(&inbound_header.front(), 8), read_header);

    }
}

void resolve_handler(const boost::system::error_code &ec, tcp::resolver::iterator it) {
    if (!ec) {
        tcp_socket.async_connect(*it, connect_handler);
    } else {
        std::cout << ec.message() << std::endl;
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
            ("messages,m", bpo::value<int>()->default_value(1), "number of messages to send async")
        ;

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
