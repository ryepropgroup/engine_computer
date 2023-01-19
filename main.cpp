#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <thread>
#include <boost/thread.hpp>
#include <chrono>
#include <stdlib.h>
#include <jsoncpp/json/json.h>
#include <time.h>
using namespace boost;
using namespace boost::asio::ip;
using namespace std::chrono_literals;
using namespace boost::asio;
    std::atomic<bool> loopBool(true);
    system::error_code ec;
    io_context service;
    ip::tcp::endpoint endpoint(ip::address::from_string("172.20.10.10"), 65432);
    
    std::string socket_response;
    std::string serial_response; 
       
template <typename T> std::string receive(T& connection, system::error_code& ec){
    using namespace std;
    vector<char> buf (1024);
    size_t length = connection.read_some(asio::buffer(buf), ec);
    string received(buf.begin(),buf.end());
    received.resize(length);
    return received;
}

void connection_manager(ip::tcp::socket& socket, serial_port& serial){
    try{
        std::cout<<"testing testing 123"<<std::endl;
        socket.connect(endpoint,ec);
        // serial.open("/dev/ttyACM0", ec);
        std::cout<<"potato"<<std::endl;
        // serial.set_option(serial_port_base::baud_rate(115200));
        std::cout<<"potato1"<<std::endl;
        while(loopBool){
            std::this_thread::sleep_for(1ms);
            socket_response= receive(socket, ec);
            if(socket_response.size()>0){
                std::cout<<"socket:"<<socket_response<<std::endl;
                // serial.write_some(asio::buffer(socket_response),ec);
                // serial_response = receive(serial, ec);
                // std::cout<<"serial:"<<serial_response<<std::endl;
            }
            if(socket_response == "quit"){
                loopBool = false;
            }
        }
    }
    catch(system::error_code& ec){
        std::cerr<<ec.value()<<std::endl;
    }
}

void data_send(ip::tcp::socket& socket, serial_port& serial){
    using namespace std;
    string unixtimenow = to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    string filename = unixtimenow += ".csv";
    ofstream file(filename);
    file<<"time(unix)"<<","<<"tc"<<","<<"p1"<<","<<"p2"<<","<<"p3"<<endl;
    while(loopBool){
        unixtimenow = to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        std::this_thread::sleep_for(1ms);
        vector<string> tojsons(4);
        tojsons[0] = to_string(rand() % 10 + 500);
        tojsons[1] = to_string(rand() % 10 + 700);
        tojsons[2] = to_string(rand() % 10 + 800);
        tojsons[3]= to_string(rand() % 10 + 900);
        file<<unixtimenow<<",";
        for(string& json: tojsons){
            while(json.length() < 4){
                json = "0"+json;
            }
            file<<json<<",";
        }
        file<<"\n";
        Json::Value potato;
        Json::FastWriter writer;
        cout<<tojsons[0]<<endl;
        potato["tc"] = tojsons[0];
        potato["p1"] = tojsons[1];
        potato["p2"] = tojsons[2];
        potato["p3"] = tojsons[3];
        string banana = writer.write(potato);
        std::cout<<banana<<std::endl;
	// socket.write_some(asio::buffer(banana),ec);
    }
    
}

int main() {
	srand (time(NULL));
    ip::tcp::socket socket(service);
    serial_port serial(service);
    std::thread uno(connection_manager, std::ref(socket),std::ref(serial));
    std::thread duo(data_send, std::ref(socket), std::ref(serial));
    duo.join();
    uno.join();
    
    return 0;
}
//     asio::ip::tcp::endpoint endpoint(asio::ip::make_address(argv[1], ec), std::atoi(argv[2]));
//     asio::ip::tcp::socket socket(ioContext);
//     try {
// //        asio::serial_port serial(ioContext);
// //        serial.open("/dev/ttyACM0", ec);
// //        serial.set_option(asio::serial_port_base::baud_rate(115200));
//         socket.connect(endpoint, ec);
//         if (!ec) std::cout << "SOCKET CONNECTED" << std::endl;
//         else std::cout << "failed" << std::endl;
//         auto socketopen = socket.is_open();
//         if (socketopen) {
//             std::cout<<"POTATO"<<std::endl;
//             std::string request = "Status: READY";
//             std::cout<<request<<std::endl;
//             socket.write_some(asio::buffer(request.data(), request.size()), ec);
//             for (;;) {
//                 std::this_thread::sleep_for(1ms);
//                 size_t bytes = socket.available();
//                 socket.wait(socket.wait_read);
//                 if (bytes > 0) {
//                     std::vector<char> vBuffer(bytes);
// //                    std::vector<char> serialBuffer(bytes);
// //                    auto socketStart = std::chrono::high_resolution_clock::now();
//                     socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec);
// //                    auto socketStop = std::chrono::high_resolution_clock::now();
// //                    auto socketDuration = std::chrono::duration_cast<std::chrono::milliseconds>(socketStop-socketStart);
//                     std::string message{vBuffer.begin(), vBuffer.end()};
//                     if (message == "quit"){
//                         socket.close(ec);
// //                        serial.close(ec);
//                         exit(0);
//                     }
// //                    auto start = std::chrono::high_resolution_clock::now();
// //                    asio::write(serial, asio::buffer(message));
// //                    auto stop = std::chrono::high_resolution_clock::now();
// //                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop-start);
//                     std::cout << "from socket: "<< message << std::endl;
// //                    std::cout<< "time taken: "<<socketDuration.count()<<" ms"<<std::endl;
// //                    auto readStart = std::chrono::high_resolution_clock::now();
// //                    serial.read_some(asio::buffer(serialBuffer.data(), serialBuffer.size()), ec);
// //                    auto readStop = std::chrono::high_resolution_clock::now();
// //                    auto readDuration = std::chrono::duration_cast<std::chrono::milliseconds>(readStop-readStart);
// //                    std::string serialMessage{serialBuffer.begin(), serialBuffer.end()};
// //                    std::cout<< "from serial: "<< serialMessage << std::endl;
// //                    std::cout<< "time taken write "<<duration.count()<<" ms"<<std::endl;
// //		    std::cout<<" time taken read " << readDuration.count()<<" ms"<<std::endl;
// //                socket.write_some(asio::buffer(vBuffer));

//                 }
//             }
//         }
//     }
//     catch(system::error_code& ec){
//         std::cerr<<ec.value()<<std::endl;
//     }
//}
