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
                    auto socketStart = std::chrono::high_resolution_clock::now();
                    socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec);
                    auto socketStop = std::chrono::high_resolution_clock::now();
                    auto socketDuration = std::chrono::duration_cast<std::chrono::milliseconds>(socketStop-socketStart);
                    std::string message{vBuffer.begin(), vBuffer.end()};
                    if (message == "quit"){
                        socket.close(ec);
                        serial.close(ec);
                        exit(0);
                    }
                    auto start = std::chrono::high_resolution_clock::now();
                    asio::write(serial, asio::buffer(message));
                    auto stop = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop-start);
                    std::cout << "from socket: "<< message << std::endl;
                    std::cout<< "time taken: "<<socketDuration.count()<<" ms"<<std::endl;
                    auto readStart = std::chrono::high_resolution_clock::now();
                    serial.read_some(asio::buffer(serialBuffer.data(), serialBuffer.size()), ec);
                    auto readStop = std::chrono::high_resolution_clock::now();
                    auto readDuration = std::chrono::duration_cast<std::chrono::milliseconds>(readStop-readStart);
                    std::string serialMessage{serialBuffer.begin(), serialBuffer.end()};
                    std::cout<< "from serial: "<< serialMessage << std::endl;
                    std::cout<< "time taken write "<<duration.count()<<" ms"<<std::endl;
		    std::cout<<" time taken read " << readDuration.count()<<" ms"<<std::endl; 
//                socket.write_some(asio::buffer(vBuffer));

                }
            }
        }
    }
    catch(system::error_code& ec){
        std::cerr<<ec.what()<<std::endl;
    }
}
