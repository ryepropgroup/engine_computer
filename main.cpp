#include "LJM_Utilities.h"
#include "json.hpp"
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <iostream>
#include <modbus/modbus-tcp.h>
#include <thread>

using json = nlohmann::json;
using namespace std::chrono_literals;
namespace ba = boost::asio;
namespace bs = boost::signals2;
unsigned short PORT = 6969;
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
  bool launched = false;
  LJSensors lj;
  std::map<std::string, bool> valves;

  [[nodiscard]] json toJSON() const {
    json jsonState;
    jsonState["launched"] = launched;
    jsonState["lj"]["p1val"] = lj.p1val;
    jsonState["lj"]["p2val"] = lj.p2val;
    jsonState["lj"]["p3val"] = lj.p3val;
    jsonState["valves"] = valves;
    return jsonState;
  }
};

struct SocketConn : std::enable_shared_from_this<SocketConn> {
  explicit SocketConn(ba::any_io_executor const &ioContext,
                      std::stop_source &stopSource)
      : _s(ioContext), _ss(stopSource){};

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
      std::cout << val << std::endl;
      if (val == "stop") {
        std::cout << "STOPPING WRITE LOOP" << std::endl;
        _ss.request_stop();
      }
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
};

struct Server {
  explicit Server(ba::io_context &ioContext, std::stop_source &stopSource)
      : _ioc(ioContext), _ss(stopSource) {
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

  size_t emit(State const &s) {
    _emit_event(s);
    return _emit_event.num_slots();
  }

private:
  size_t reg(std::shared_ptr<SocketConn> c) {
    c->_sig =
        _emit_event.connect([safe_c = std::weak_ptr<SocketConn>(c)](State s) {
          if (auto c = safe_c.lock()) {
            std::string msg = s.toJSON().dump() + "\n";
            c->send(msg, true);
          }
        });
    return _emit_event.num_slots();
  }

  void accept() {
    auto sess = std::make_shared<SocketConn>(_acc.get_executor(), _ss);
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
  bs::signal<void(State const &s)> _emit_event;
  std::stop_source _ss;
};

void run_signal(const std::stop_token *t, Server *s) {
  State st = *new State();
  while (!t->stop_requested()) {
    s->emit(st);
  }
  std::lock_guard<std::mutex> l(coutm);
  std::cout << "done" << std::endl;
}

int main() {
  std::stop_source test;
  std::stop_token token = test.get_token();
  ba::io_context ioContext;
  Server s(ioContext, test);
  std::jthread th([&ioContext] { ioContext.run(); });
  std::this_thread::sleep_for(10s);
  std::jthread th2(run_signal, &token, &s);
  std::this_thread::sleep_for(10s);
  test.request_stop();
  s.stop();
  th.join();
  return 0;
}
