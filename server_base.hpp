/*
server_base.hpp
web_server
*/

#ifndef SERVER_BASE_HPP
#define SERVER_BASE_HPP

#include <boost/asio.hpp>

#include <regex>
#include <unordered_map>
#include <thread>
#include <map>

namespace CppWeb{

    struct Request{
        // Methods. POST, GET; Path; HTTP Version
        std::string method, path, http_version;
        // Content shared pointer counter
        std::shared_ptr<std::istream> content;
        // Hash container
        std::unordered_map<std::string, std::string> header;
        // Regular Expression for path match
        std::smatch path_match;
    };

    // Define a *resource_type* to simplify representation for resrouce types
    typedef std::map<std::string, std::unordered_map<std::string,
    std::function<void(std::ostream&, Request&)>> > resource_type;

    // socket type is to be HTTP or HTTPS
    template <typename socket_type>
    class ServerBase{
        public:
            resource_type resource;
            resource_type default_resource;
            
            // Initialize a server with ports. For default only one thread is used
            ServerBase(unsigned short port, size_t num_threads=1) :
                endpoint(boost::asio::ip::tcp::v4(), port),
                acceptor(m_io_service, endpoint),num_threads(num_threads){}

            void start(){
                for(auto it = resource.begin(); it != resource.end(); it++){
                    all_resources.push_back(it);
                }
                for(auto it = default_resource.begin(); it != default_resource.end(); it++){
                    all_resources.push_back(it);
                }
                accept();
                for(size_t c=1; c<num_threads; c++){
                    threads.emplace_back([this](){
                        m_io_service.run();
                    });
                }

                m_io_service.run();
                for(auto& t: threads){
                    t.join();
                }
            }
        protected:

            // io_service is to deal with async IO requests
            boost::asio::io_service m_io_service;
            // IP address, port number and agreement version makes an endpoint
            // Through this endpoint to generate tcp::acceptor object on server side
            // And waiting on given port
            boost::asio::ip::tcp::endpoint endpoint;
            // Mentioned above, an acceptor require two variables: io_service and endpoint
            boost::asio::ip::tcp::acceptor acceptor;

            size_t num_threads;
            std::vector<std::thread> threads;

            // All resources and default reousrce will be added to the vector's ends and be created through start()
            std::vector<resource_type::iterator> all_resources;
            
            // Different servers require specific realization
            virtual void accept(){}
            // Process request and response
            void process_request_and_respond(std::shared_ptr<socket_type> socket) const {
                auto read_buffer = std::make_shared<boost::asio::streambuf>();
                boost::asio::async_read_until(*socket, *read_buffer, "\r\n\r\n",
                [this, socket, read_buffer](const boost::system::error_code& ec, size_t bytes_transferred){
                    if(!ec){
                        size_t total = read_buffer->size();
                        std::istream stream(read_buffer.get());
                        auto request = std::make_shared<Request>();
                        *request = parse_request(stream);
                        
                        size_t num_additional_bytes = total-bytes_transferred;

                        if(request->header.count("Content-Length")>0){
                            boost::asio::async_read(*socket, *read_buffer,
                            boost::asio::transfer_exactly(stoull(request->header["Content-Length"]) - num_additional_bytes),
                            [this, socket, read_buffer, request](const boost::system::error_code& ec, size_t bytes_transferred){
                                if(!ec){
                                    request->content = std::shared_ptr<std::istream>(new std::istream(read_buffer.get()));
                                    respond(socket, request);
                                }
                            });
                        }else{
                            respond(socket, request);
                        }
                    }
                });
            }

            // Examine the Request by parsing it
            Request parse_request(std::istream& stream) const {
                Request request;
                
                std::regex e("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
                std::smatch sub_match;
                
                // Get request methods, path and HTTP version in line No.1
                std::string line;
                getline(stream, line);
                line.pop_back();

                if(std::regex_match(line, sub_match, e)) {
                    request.method       = sub_match[1];
                    request.path         = sub_match[2];
                    request.http_version = sub_match[3];

                    bool matched;
                    e="^([^:]*): ?(.*)$";
                    do {
                        getline(stream, line);
                        line.pop_back();
                        matched=std::regex_match(line, sub_match, e);
                        if(matched) {
                            request.header[sub_match[1]] = sub_match[2];
                        }

                    } while(matched==true);
                }
                return request;
            }


            void respond(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) const {
                // Match request path and methods and generate response
                for(auto res_it: all_resources) {
                    std::regex e(res_it->first);
                    std::smatch sm_res;
                    if(std::regex_match(request->path, sm_res, e)) {
                        if(res_it->second.count(request->method)>0) {
                            request->path_match = move(sm_res);

                            //  std::shared_ptr<boost::asio::streambuf>
                            auto write_buffer = std::make_shared<boost::asio::streambuf>();
                            std::ostream response(write_buffer.get());
                            res_it->second[request->method](response, *request);

                            // catch write_buffer from lambda to make sure it will exist until async_write finished
                            boost::asio::async_write(*socket, *write_buffer,
                            [this, socket, request, write_buffer](const boost::system::error_code& ec, size_t bytes_transferred) {
                                //HTTP connection
                                if(!ec && stof(request->http_version)>1.05)
                                    process_request_and_respond(socket);
                            });
                            return;
                        }
                    }
                }
            }

    };
    template<typename socket_type>
    class Server : public ServerBase<socket_type>{};
}

#endif /*SERVER_BASE_HPP*/