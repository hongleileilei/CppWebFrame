#include <iostream>
#include "server_http.hpp"
#include "handler.hpp"

using namespace CppWeb;

int main(){
    Server<HTTP> server(12345, 4);
    std::cout<<"Server starting at port: 12345"<<std::endl;
    start_server<Server<HTTP>> (server);
    return 0;
}