#include "helpers.h"
#include <memory>
#include <string>
#include <vector>
#include "LabJackM.h"

const double *mach::vectorToDouble(const std::vector<double> &doubleVector) {
    return doubleVector.data();
}

const char **mach::vectorToChar(const std::vector<std::string> &stringVector) {
    const char **charPtrArray = new const char *[stringVector.size()];
    for (size_t i = 0; i < stringVector.size(); i++) {
        charPtrArray[i] = stringVector[i].c_str();
    }

    return charPtrArray;
}

void mach::SocketConn::send(std::string msg, bool immediate) {
    post(_s.get_executor(),
         [self = shared_from_this(), msg = std::move(msg), immediate] {
             if (self->nq(msg, immediate))
                 self->_write();
         });
}

void mach::dispatchValve(std::string name, int handle){
    if (name == "stop") {
        std::cout << "potato" << std::endl;
        return;
    }
    if (name == "seq") {
        // open
        std::this_thread::sleep_for(3s);
        // close
        return;
    }
    size_t nindex = name.rfind('_');
    std::string valve = name.substr(0, nindex);
    std::string status = name.substr(nindex+1);
    std::cout<<valve<<status<<std::endl;
    if(status=="open")
        LJM_eWriteName(handle, mach::vlj.at(valve).c_str(), 0);
    else
        LJM_eWriteName(handle, mach::vlj.at(valve).c_str(), 1);
}
void mach::SocketConn::start() { _read(); }

mach::SocketConn::SocketConn(ba::any_io_executor const &ioContext,
                             std::stop_source &stopSource, const int& lj)
        : _s(ioContext), _ss(stopSource), labjack(lj) {};

void mach::SocketConn::input() {
    std::string val;
    if (std::getline(std::istream(&_buf), val)) {
        std::lock_guard<std::mutex> l(coutm);
//        std::cout << val << std::endl;
        mach::dispatchValve(val, labjack);
    }
}

bool mach::SocketConn::nq(std::string msg, bool immediate) {
    immediate &= !_mlist.empty(); // assert same
    if (immediate)
        _mlist.insert(std::next(begin(_mlist)),
                      std::move(msg)); // insert at beginning of list
    else
        _mlist.push_back(std::move(msg)); // queue to end
    return _mlist.size() == 1;
}

bool mach::SocketConn::dq() {
    assert(!_mlist.empty());
    _mlist.pop_front();
    return !_mlist.empty();
}

void mach::SocketConn::_read() {
    ba::async_read_until(_s, _buf, "\n",
                         [this, self = shared_from_this()](
                                 boost::system::error_code ec, size_t n) {
                             input();
                             if (!ec)
                                 _read();
                         });
};

void mach::SocketConn::_write() {
    ba::async_write(_s, ba::buffer(_mlist.front()),
                    [this, self = shared_from_this()](
                            boost::system::error_code ec, size_t n) {
                        if (!ec && dq())
                            _write();
                    });
};

mach::Server::Server(ba::io_context &ioContext, std::stop_source &stopSource,
                     const std::shared_ptr<State> &st, const int &lj)
        : _ioc(ioContext), _ss(stopSource), _st(st), labjack(lj) {
    try {
        //TODO: server constructor grab LJ control magic
    } catch (...) {
        std::cout << "control" << std::endl;
    }
    _acc.bind({{}, PORT});
    _acc.set_option(ba::ip::tcp::acceptor::reuse_address());
    _acc.listen();
    accept();
}

void mach::Server::stop() {
    _ioc.post([this] {
        this->_acc.cancel();
        this->_acc.close();
    });
}

size_t mach::Server::emit(const std::shared_ptr<State> &s) {
    _emit_event(s);
    return _emit_event.num_slots();
}

size_t mach::Server::reg(const std::shared_ptr<SocketConn> &c) {
    c->_sig = _emit_event.connect(
            [safe_c = std::weak_ptr<SocketConn>(c)](const std::shared_ptr<State> &s) {
                if (auto c = safe_c.lock()) {
                    std::string msg = s->toJSON().dump() + "\n";
                    c->send(msg, true);
                }
            });
    return _emit_event.num_slots();
}

void mach::Server::accept() {
    auto sess = std::make_shared<SocketConn>(_acc.get_executor(), _ss, labjack);
    _acc.async_accept(sess->_s, [this, sess](boost::system::error_code ec) {
        auto endpoint = ec ? ba::ip::tcp::endpoint{} : sess->_s.remote_endpoint();
        std::lock_guard<std::mutex> l(coutm);
        std::cout << "Connected from " << endpoint << " (" << ec.message() << ")"
                  << std::endl;
        if (!ec) {
            reg(sess);
            sess->start();
            accept();
        }
    });
}
