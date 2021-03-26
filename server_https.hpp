#ifndef SERVER_HTTPS_HPP
#define SERVER_HTTPS_HPP

#include "server_http.hpp"
#include <boost/asio/ssl.hpp>
#include <iostream>

namespace CppWeb{
    // Define HTTPS type
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> HTTPS;

    // Define HTTPS serverce, template type is HTTPS
    template<>
    class Server<HTTPS> : public ServerBase<HTTPS> {
        public:
            // HTTPS server has two more variables comparing with HTTP server
            // certificate and private key

            Server(unsigned short port, size_t num_threads, 
            const std::string& cert_file, const std::string& private_key_file) : 
            ServerBase<HTTPS>::ServerBase(port, num_threads), 
            context(boost::asio::ssl::context::sslv23){
                // Use certificate
                context.use_certificate_chain_file(cert_file);
                // Use private key
                context.use_private_key_file(private_key_file, boost::asio::ssl::context::pem);
                std::cout<<"keys and certificate set up..."<<std::endl;
            }
        private:
            // Comparing with HTTP server, we need one ssl context object
            boost::asio::ssl::context context;
            /*
            Difference in socket construction
            HTTPS would encrypt IO stream in socket, Thus accept() need initialize ssl context
            */
            void accept(){
                // New socket
                auto socket = std::make_shared<HTTPS>(m_io_service, context);


                acceptor.async_accept(
                    (*socket).lowest_layer(),
                    [this, socket](const boost::system::error_code& ec){
                        // Start and accept new connection
                        accept();

                        //
                        if(!ec){
                            (*socket).async_handshake(
                                boost::asio::ssl::stream_base::server,
                                [this, socket](const boost::system::error_code& ec){
                                    if(!ec){
                                        std::cout<<"async handshake set up..."<<std::endl;
                                        process_request_and_respond(socket);
                                        }
                                }
                            );
                        }
                    }

                );
            }
    };
}

#endif