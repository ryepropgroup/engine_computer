#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using namespace boost;
using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
    if(argc!=3){
        throw std::invalid_argument("No arguments passed!");
    }
    system::error_code ec;
    asio::io_context ioContext;
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address(argv[1], ec), std::atoi(argv[2]));
    asio::ip::tcp::socket socket(ioContext);
    try {
        asio::serial_port serial(ioContext);
        serial.open("/dev/ttyACM0", ec);
        serial.set_option(asio::serial_port_base::baud_rate(115200));
        socket.connect(endpoint, ec);
        if (!ec) std::cout << "SOCKET CONNECTED" << std::endl;
        else std::cout << "failed" << std::endl;
        auto socketopen = socket.is_open();
        if (socketopen) {
            std::cout<<"POTATO"<<std::endl;
            std::string request = "Status: READY";
            std::cout<<request<<std::endl;
            socket.write_some(asio::buffer(request.data(), request.size()), ec);
            for (;;) {
                std::this_thread::sleep_for(1ms);
                size_t bytes = socket.available();
                socket.wait(socket.wait_read);
                if (bytes > 0) {
                    std::vector<char> vBuffer(bytes);
                    std::vector<char> serialBuffer(bytes);
                    socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec);
                    std::string message{vBuffer.begin(), vBuffer.end()};
                    asio::write(serial, asio::buffer(message));
                    std::cout << "from socket: "<< message << std::endl;
                    serial.read_some(asio::buffer(serialBuffer.data(), serialBuffer.size()), ec);
                    std::string serialMessage{serialBuffer.begin(), serialBuffer.end()};
                    std::cout<< "from serial: "<< serialMessage << std::endl;
//                socket.write_some(asio::buffer(vBuffer));
                    if (message == "quit"){
                        socket.close(ec);
                        serial.close(ec);
                        return 0;
                    }
                }
            }
        }
    }
    catch(system::error_code& ec){
        std::cerr<<ec.what()<<std::endl;
    }
}