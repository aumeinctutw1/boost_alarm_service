#include <iostream>
#include <boost/asio.hpp>

#include "source/server.h"

int main(int argc, const char *argv[]) {
    
    short port = 2000;

    try {
        boost::asio::io_context io_context;
        Server serverInstance(io_context, port);
        io_context.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}