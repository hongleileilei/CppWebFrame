#ifndef SERVER_HTTP_HPP
#define SERVER_HTTP_HPP

#include "server_base.hpp"

namespace CppWeb {
    typedef boost::asio::ip::tcp::socket HTTP;
    template<>
    class Server<HTTP> : public ServerBase<HTTP> {
        public:
            // Through ports and threads to create server
            Server(unsigned short port, size_t num_threads = 1) :
                ServerBase<HTTP>::ServerBase(port, num_threads) {};
        private:
            // accept() function realization
            void accept(){
                // Create new socket for current connection
                // Shared pointer for passing temp object to anonymous functino
                // socket will be derived as shared_ptr<HTTP> type
                auto socket = std::make_shared<HTTP>(m_io_service);
                acceptor.async_accept( *socket,[this, socket](const boost::system::error_code& ec){
                                        // Start a connection immediately
                                           accept();
                                           if(!ec) process_request_and_respond(socket);
                                       });
            }
    };
}

#endif