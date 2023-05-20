#include <iostream>
#include <boost/asio.hpp>

int main() {
    using namespace boost;
    int backlog = 5;
    // "mach" on a phone keypad (minus repeats)
    size_t port = 6224;
    // initialize endpoint
    asio::ip::tcp::endpoint e(ip::address_v4::any(), port);
    // create context for i/o
    asio::io_context context;
    // tcp passive socket
    try {
        asio::ip::tcp::acceptor a(context, e.protocol());
        // bind socket to port
        a.bind(e);
        a.listen(backlog);
        asio::ip::tcp::socket s(context);
        a.accept(s);
    }
    catch (boost::system::error_code &err) {
        std::cout << "Acceptor Cannot Initialize:\nError Code: " << err.what() << "\nMessage: " << err.message()
                  << std::endl;
        return err.value();
    }
    return 0;
}
