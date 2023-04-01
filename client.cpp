#include <iostream>
#include <chrono>



int main(int argc, const char *argv[]) {

    if (argc < 5) {
        std::cerr << "Error: not enough arguments provided" << std::endl;
        return 1;
    }
    
    /* takes 4 Parameters, request id, seconds to wait, server ip, server port*/
    uint32_t requestId = atoi(argv[1]); 
    int secs = atoi(argv[2]);
    const char *ip = argv[3];
    int port = atoi(argv[4]);

    std::string message = "Wake up lovely service";
    uint32_t cookieSize = message.length();

    std::cout << "Sending: " << message << ". with size: " << cookieSize << std::endl;

    using namespace std::chrono;
    uint64_t sec = duration_cast<seconds>(system_clock::now().time_since_epoch()).count() + secs;

    try {
        std::cout << "seconds: " << secs << std::endl;

    } catch (std::exception &e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }

    return 0;
}
