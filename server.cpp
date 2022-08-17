#include "boost/asio/steady_timer.hpp"
#include <cstdio>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <chrono>

#define MAX_CONNECTIONS 2

using boost::asio::ip::tcp;

class Server;

struct request {
    uint32_t requestId;
    uint64_t dueTime;
    uint32_t cookieSize;
    std::string cookieData;
};

class Session: public std::enable_shared_from_this <Session> {
    public:
        Session(tcp::socket socket, boost::asio::io_context &io_context, Server *server)
                :   socket_(std::move(socket)), 
                    timer_(io_context),
                    server_(server)
        {       
            // count the active connections
            connections++;
        }

        ~Session(){
            // reduce active sessions if client disconnects
            connections--;

            if(connections < MAX_CONNECTIONS){
                openServer(server_);
            }
        }

        void openServer(Server *server);

        void start() {
            // start by reading the header to get the message length
            read_header();  
        }

        static int connections;

    private:
        void read_header(){
            auto self(shared_from_this());
            boost::asio::async_read(socket_, boost::asio::buffer((char*)&inbound_header_.front(), header_length), 
                [this, self](boost::system::error_code ec, std::size_t length){
                    if (!ec) {
                        // check if the client has more then 100 timers runnning
                        if(requests.size() >= 100){
                            // error
                        }

                        // create a new request
                        request req;

                        // read the header into request struct
                        req.requestId = ntohl(inbound_header_[0]);
                      
                        uint32_t high = ntohl(inbound_header_[1]);
                        uint32_t low = ntohl(inbound_header_[2]);
                        req.dueTime = (((uint64_t) high) << 32) | ((uint64_t) low);

                        req.cookieSize = ntohl(inbound_header_[3]);

                        requests.push_back(req);

                        //resize the inbound data vector so the cookiedata can fit
                        inbound_data_.resize(req.cookieSize);

                        // start reading the cookiedata
                        read_data(req.requestId);
                    }
                });
        }

        void read_data(uint32_t requestId){
            auto self(shared_from_this());
            boost::asio::async_read(socket_, boost::asio::buffer(inbound_data_), 
                [this, self, requestId](boost::system::error_code ec, std::size_t length){
                    if (!ec) {
                        std::string cookieData(&inbound_data_[0], inbound_data_.size());
                        
                        request req;

                        // find the request by id
                        for(auto &i : requests){
                            if (i.requestId == requestId){
                                i.cookieData = cookieData;
                                req = i;
                                break;
                            }
                        }
                        // set the timer for the found request
                        set_timer(req);
                    }
                });
        }

        void respond_client(const request &req){            
            const char *req_id = reinterpret_cast<const char*>(&req.requestId);
            const char *cookie_size = reinterpret_cast<const char*>(&req.cookieSize);

            // build buffer
            std::vector<boost::asio::const_buffer> buffers;
            buffers.push_back(boost::asio::buffer(req_id, sizeof(uint32_t)));
            buffers.push_back(boost::asio::buffer(cookie_size, sizeof(uint32_t)));
            buffers.push_back(boost::asio::buffer(req.cookieData, req.cookieSize));

            boost::asio::async_write(socket_, buffers,
                [this](boost::system::error_code ec, std::size_t length){
                    if (!ec){
                        std::cout << "send len: " << length << std::endl;                        
                    }else{
                        std::cout << "error responding: " << ec.value() << std::endl;
                    }
                });
        }

        void set_timer(const request &req){ 
            auto self(shared_from_this());
            timer_.expires_after(boost::asio::chrono::seconds(1));
            timer_.async_wait([this, self, req](boost::system::error_code errorCode){
                if(!errorCode){
                    // get system time
                    using namespace std::chrono;
                    uint64_t sysTime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

                    if(sysTime >= req.dueTime){
                        // time is up, respond to client
                        std::cout << "time is up" << std::endl;
                        //respond to client
                        respond_client(req);
                    } else {
                        //keep on waiting
                        std::cout << "ID: " << req.requestId << " Waiting..." << std::endl;
                        set_timer(req);
                    }
                } else {
                    std::cout << "error setting timer: " << errorCode.message() << std::endl;
                }
            });
        }

        enum {
            header_length = 16,
        };

        boost::asio::steady_timer timer_;
        tcp::socket socket_;
        Server *server_;

        // buffers, the data is then stored in the request
        std::vector<char> inbound_data_;
        std::vector<uint32_t> inbound_header_{0,0,0,0};

        // hold the data from the request, needs to be a vector or similar
        std::vector<request> requests;

        std::string testdata;
};


class Server {
    public:
        Server(boost::asio::io_context &io_context, short port)
                :   endpoint_(tcp::v4(), port),
                    acceptor_(io_context), 
                    timer_(io_context),
                    port_(port),
                    context_(io_context) 
        {
            std::cout << port << std::endl;
            open_acceptor();            
        }

        void open_acceptor(){
            boost::system::error_code errorCode;

            if(!acceptor_.is_open()){
                acceptor_.open(endpoint_.protocol());
                // reuse port and adress
                acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
                acceptor_.bind(endpoint_);
                acceptor_.listen(boost::asio::socket_base::max_listen_connections, errorCode);
                // accept new connections
                if(!errorCode){
                    do_accept();
                }
            }
        }

    private:
        void do_accept() { 
            acceptor_.async_accept([this](boost::system::error_code errorCode, tcp::socket socket) {
                if (!errorCode) {
                    std::make_shared<Session>(std::move(socket), context_, this)->start();
                    // at 100 active connections
                    if(Session::connections == MAX_CONNECTIONS && acceptor_.is_open()){
                        // close the acceptor
                        acceptor_.close();
                    } else {
                        do_accept();
                    }
                }
            });
        }

        tcp::endpoint endpoint_;
        tcp::acceptor acceptor_;
        boost::asio::steady_timer timer_;
        short port_;
        boost::asio::io_context &context_;
};

int Session::connections = 0;

void Session::openServer(Server *server){
    server->open_acceptor();
}

int main() {
    
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