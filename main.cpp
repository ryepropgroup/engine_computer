#include "LJM_Utilities.h"
#include "json.hpp"
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <fstream>
#include <iostream>
#include <modbus/modbus-tcp.h>
#include <thread>
#include <utility>

using json = nlohmann::json;
using namespace std::chrono_literals;
namespace ba = boost::asio;
namespace bs = boost::signals2;
unsigned short PORT = 6970;
std::mutex coutm;

struct Sensor {
    Sensor(std::string name, std::vector<std::string> params,
           const std::vector<double> &settings) {
        this->name = std::move(name);
        this->params = std::move(params);
        this->settings = settings;
    }

    std::string name;
    std::vector<std::string> params;
    std::vector<double> settings;
};

struct LJSensors {
    double p1val = 0, p2val = 0, p3val = 0;
    Sensor *p1 =
            new Sensor(std::string("AIN0"),
                       std::vector<std::string>{"AIN0_range", "AIN0_NEGATIVE_CH"},
                       std::vector<double>{0.1, 1.0});
    Sensor *p2 =
            new Sensor(std::string("AIN2"),
                       std::vector<std::string>{"AIN2_range", "AIN2_NEGATIVE_CH"},
                       std::vector<double>{0.1, 3.0});
    Sensor *p3 =
            new Sensor(std::string("AIN4"),
                       std::vector<std::string>{"AIN4_range", "AIN4_NEGATIVE_CH"},
                       std::vector<double>{0.1, 5.0});
};

struct State {
    State() {
        valves["V10_SB"] = false;
        valves["V11_S"] = true; //NO valve
        valves["V12_S"] = true;//NO valve
        valves["V20_SB"] = false;
        valves["V21_SB"] = false;
        valves["V23_SB"] = true; //NO valve
        valves["V30_SB"] = false;
        valves["V31_SB"] = false;
        valves["V34_SB"] = true; // NO valve
        valves["V35_S"] = false;
        valves["V36_SB"] = false;
        valves["V37_S"] = true; //NO valve
    }

    bool launched = false;
    LJSensors lj;
    std::map<std::string, bool> valves;

    [[nodiscard]] json toJSON() const {
        std::vector<std::string> db = {std::to_string(int(lj.p1val)),
                                       std::to_string(int(lj.p2val)),
                                       std::to_string(int(lj.p3val))};
        for (auto &d: db) {
            while (d.length() < 4) {
                d = "0" + d;
            }
        }
        json jsonState;
        jsonState["launched"] = launched;
        jsonState["lj"]["p1val"] = db[0];
        jsonState["lj"]["p2val"] = db[1];
        jsonState["lj"]["p3val"] = db[2];
        jsonState["valves"] = valves;
        return jsonState;
    }
};

std::string valveFunc(const int num, const std::string& status, const std::string& valve, const std::shared_ptr<State>& state){
    std::string r;
    switch(num){
        case 10:
            r = "1";
            break;
        case 11:
            r = "2";
            break;
        case 12:
            r = "3";
            break;
        case 20:
            r = "4";
            break;
        case 21:
            r = "5";
            break;
        case 23:
            r = "6";
            break;
        case 30:
            r = "7";
            break;
        case 31:
            r = "8";
            break;
        case 34:
            r = "9";
            break;
        case 35:
            r = "10";
            break;
        case 36:
            r = "11";
            break;
        case 37:
            r = "12";
            break;
        default:
            break;
    }
    if(status == "_open"){
        r.append("1");
        state->valves[valve] = true;
    }
    else if (status == "_close"){
        r.append("0");
        state->valves[valve] = false;
    }
    return r;
}
struct SocketConn : std::enable_shared_from_this<SocketConn> {
    explicit SocketConn(ba::any_io_executor const &ioContext,
                        std::stop_source &stopSource, std::shared_ptr<ba::serial_port> sp, std::shared_ptr<State> st)
            : _s(ioContext), _ss(stopSource), _sp(std::move(sp)), _st(std::move(st)) {};

    void start() { _read(); }

    void send(std::string msg, bool immediate = false) {
        post(_s.get_executor(),
             [self = shared_from_this(), msg = std::move(msg), immediate] {
                 if (self->nq(msg, immediate))
                     self->_write();
             });
    }

private:
    void input() {
        std::string val;
        if (std::getline(std::istream(&_buf), val)) {
            std::lock_guard<std::mutex> l(coutm);
            size_t nindex = val.rfind('_');
            std::string valstr = val.substr(0, nindex) ;
            std::string status = val.substr(nindex) ;
            std::string valve = val.substr(1, 2);
//            std::cout << valve << std::endl;
            if(val == "stop") {
                std::cout << "STOPPING WRITE LOOP" << std::endl;
                _ss.request_stop();
            }
            std::string tosend = valveFunc(stoi(valve), status, valstr, _st);
            _sp->write_some(ba::buffer(tosend));
        }
    }

    bool nq(std::string msg, bool immediate) {
        immediate &= !_mlist.empty(); // assert same
        if (immediate)
            _mlist.insert(std::next(begin(_mlist)),
                          std::move(msg)); // insert at beginning of list
        else
            _mlist.push_back(std::move(msg)); // queue to end
        return _mlist.size() == 1;
    }

    // return true if messages are pending post-dequeue
    bool dq() {
        assert(!_mlist.empty());
        _mlist.pop_front();
        return !_mlist.empty();
    }

    void _read() {
        ba::async_read_until(_s, _buf, "\n",
                             [this, self = shared_from_this()](
                                     boost::system::error_code ec, size_t n) {
                                 input();
                                 if (!ec)
                                     _read();
                             });
    };

    void _write() {
        ba::async_write(_s, ba::buffer(_mlist.front()),
                        [this, self = shared_from_this()](
                                boost::system::error_code ec, size_t n) {
                            if (!ec && dq())
                                _write();
                        });
    };
    friend struct Server;
    boost::asio::streambuf _buf;
    std::list<std::string> _mlist;
    ba::ip::tcp::socket _s;
    bs::scoped_connection _sig;
    std::stop_source _ss;
    std::shared_ptr<ba::serial_port> _sp;
    std::shared_ptr<State> _st;
};

struct Server {
    explicit Server(ba::io_context &ioContext, std::stop_source &stopSource, const std::shared_ptr<State> &st)
            : _ioc(ioContext), _ss(stopSource), _st(st) {
        try {
            _sp = std::make_shared<ba::serial_port>(_ioc, "/dev/ttyACM0");
        }
        catch (...) {
            std::cout << "ARDUINO" << std::endl;
        }
        _sp->set_option(ba::serial_port_base::baud_rate(115200));
        _acc.bind({{}, PORT});
        _acc.set_option(ba::ip::tcp::acceptor::reuse_address());
        _acc.listen();
        accept();
    }

    void stop() {
        _ioc.post([this] {
            this->_acc.cancel();
            this->_acc.close();
        });
    }

    size_t emit(const std::shared_ptr<State> &s) {
        _emit_event(s);
        return _emit_event.num_slots();
    }

private:
    size_t reg(const std::shared_ptr<SocketConn> &c) {
        c->_sig = _emit_event.connect(
                [safe_c = std::weak_ptr<SocketConn>(c)](const std::shared_ptr<State> &s) {
                    if (auto c = safe_c.lock()) {
                        std::string msg = s->toJSON().dump() + "\n";
                        c->send(msg, true);
                    }
                });
        return _emit_event.num_slots();
    }

    void accept() {
        auto sess = std::make_shared<SocketConn>(_acc.get_executor(), _ss, _sp, _st);
        _acc.async_accept(sess->_s, [this, sess](boost::system::error_code ec) {
            auto endpoint = ec ? ba::ip::tcp::endpoint{} : sess->_s.remote_endpoint();
            std::lock_guard<std::mutex> l(coutm);
            std::cout << "Connected from " << endpoint << " (" << ec.message() << ")"
                      << std::endl;
            if (!ec) {
                auto n = reg(sess);
                sess->start();
                accept();
            }
        });
    }

    ba::io_context &_ioc;
    ba::ip::tcp::acceptor _acc{_ioc, ba::ip::tcp::v4()};
    bs::signal<void(std::shared_ptr<State> const &s)> _emit_event;
    std::stop_source _ss;
    std::shared_ptr<ba::serial_port> _sp;
    std::shared_ptr<State> _st;
};

const char **vectorToChar(const std::vector<std::string> &stringVector) {
    const char **charPtrArray = new const char *[stringVector.size()];
    for (size_t i = 0; i < stringVector.size(); i++) {
        charPtrArray[i] = stringVector[i].c_str();
    }

    return charPtrArray;
}

const double *vectorToDouble(const std::vector<double> &doubleVector) {
    return doubleVector.data();
}

void run_signal(const std::stop_token *t, Server *s, const std::shared_ptr<State> st) {
    // LabJack Initialization
    int err, handle, errorAddress = INITIAL_ERR_ADDRESS;
    std::string utn =
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                                   .count());
    std::string filename = utn += ".csv";
    try {
        err = LJM_Open(LJM_dtANY, LJM_ctANY, "LJM_idANY", &handle);
        ErrorCheck(err, "LJM_Open");
    }
    catch (...) {
        s->stop();
        std::cout << "LABJACK" << std::endl;
    }
//    std::ofstream file(filename);
    err = LJM_eWriteNames(handle, int(st->lj.p1->params.size()),
                          vectorToChar(st->lj.p1->params),
                          vectorToDouble(st->lj.p1->settings), &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(handle, int(st->lj.p2->params.size()),
                          vectorToChar(st->lj.p2->params),
                          vectorToDouble(st->lj.p2->settings), &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(handle, int(st->lj.p3->params.size()),
                          vectorToChar(st->lj.p3->params),
                          vectorToDouble(st->lj.p3->settings), &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    while (!t->stop_requested()) {
        err = LJM_eReadName(handle, st->lj.p1->name.c_str(), &st->lj.p1val);
        ErrorCheck(err, "LJM_eReadName");
        err = LJM_eReadName(handle, st->lj.p2->name.c_str(), &st->lj.p2val);
        ErrorCheck(err, "LJM_eReadName");
        err = LJM_eReadName(handle, st->lj.p3->name.c_str(), &st->lj.p3val);
        ErrorCheck(err, "LJM_eReadName");
        st->lj.p1val *= 25000;
        st->lj.p2val *= 25000;
        st->lj.p3val *= 25000;
        s->emit(st);
    }
    std::lock_guard<std::mutex> l(coutm);
    std::cout << "done" << std::endl;
}

int main() {
    std::stop_source test;
    std::stop_token token = test.get_token();
    ba::io_context ioContext;
    auto st = std::make_shared<State>();
    Server s(ioContext, test, st);
    std::jthread th([&ioContext] { ioContext.run(); });
    std::this_thread::sleep_for(10s);
    std::jthread th2(run_signal, &token, &s, st);
    while (!token.stop_requested()) {
        std::this_thread::sleep_for(10ms);
    }
    s.stop();
    th.join();
    return 0;
}
