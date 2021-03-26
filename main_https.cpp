#include "server_https.hpp"
#include "handler.hpp"
#include <iostream>

using namespace CppWeb;

int main(){
    Server<HTTPS> server(12345, 4, "server.crt", "server.key");
    std::cout<<"Server set up...port: 12345 with 4 threads"<<std::endl;
    start_server<Server<HTTPS>>(server);
    return 0;
}