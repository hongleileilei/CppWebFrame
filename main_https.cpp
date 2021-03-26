#include "server_https.hpp"
#include "handler.hpp"

using namespace CppWeb;

int main(){
    Server<HTTPS> server(8080, 4, "server.crt", "server.key");
    start_server<Server<HTTPS>>(server);
    return 0;
}