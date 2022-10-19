#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using namespace boost;
using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
    system::error_code ec;
    asio::io_context ioContext;
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address(argv[1], ec), 65432);
    asio::ip::tcp::socket socket(ioContext);
    socket.connect(endpoint, ec);
    if (!ec) { std::cout << "connected" << std::endl; }
    else { std::cout << "failed" << std::endl; };
    if (socket.is_open()) {
        std::string request = "testing testing 123";
        socket.write_some(asio::buffer(request.data(), request.size()), ec);
        for (;;) {
            std::this_thread::sleep_for(1ms);
            size_t bytes = socket.available();
            socket.wait(socket.wait_read);
            if (bytes > 0) {
                std::vector<char> vBuffer(bytes);
                socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec);
                std::string message{vBuffer.begin(), vBuffer.end()};
                std::cout<<message<<std::endl;
                if(message=="quit"){
                    socket.close();
                }
                //for (auto c: vBuffer) {
                  //  std::cout << c;
               // }
                //std::cout << "" << std::endl;
            }
        }
    }
}