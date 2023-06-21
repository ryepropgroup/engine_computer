#include "helpers.h"
#include <memory>
#include <string>
#include <vector>
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

std::string mach::valveFuncNO(const int num, const std::string &status,
                              const std::string &valve,
                              const std::shared_ptr<mach::State> &state) {
  std::string r;
  switch (num) {
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
  if (status == "_open") {
    r.append("0");
    state->valves[valve] = false;
  } else if (status == "_close") {
    r.append("1");
    state->valves[valve] = true;
  }
  return r;
}

std::string mach::valveFunc(const int num, const std::string &status,
                            const std::string &valve,
                            const std::shared_ptr<State> &state) {
  std::string r;
  switch (num) {
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
  if (status == "_open") {
    r.append("1");
    state->valves[valve] = true;
  } else if (status == "_close") {
    r.append("0");
    state->valves[valve] = false;
  }
  return r;
}

void mach::SocketConn::send(std::string msg, bool immediate) {
  post(_s.get_executor(),
       [self = shared_from_this(), msg = std::move(msg), immediate] {
         if (self->nq(msg, immediate))
           self->_write();
       });
}

void mach::SocketConn::start() { _read(); }

mach::SocketConn::SocketConn(ba::any_io_executor const &ioContext,
                             std::stop_source &stopSource,
                             std::shared_ptr<ba::serial_port> sp,
                             std::shared_ptr<State> st)
    : _s(ioContext), _ss(stopSource), _sp(std::move(sp)), _st(std::move(st)){};
void mach::SocketConn::input() {
  std::string val;
  if (std::getline(std::istream(&_buf), val)) {
    std::lock_guard<std::mutex> l(coutm);
    std::cout << val << std::endl;
    if (val == "stop") {
      std::cout << "potato" << std::endl;
      _sp->write_some(ba::buffer(std::string("999")));
      return;
    }
    if (val == "seq") {
      // open
      _sp->write_some(ba::buffer(std::string("71")));
      std::this_thread::sleep_for(3s);
      // close
      _sp->write_some(ba::buffer(std::string("70")));
      return;
    }
    size_t nindex = val.rfind('_');
    std::string valstr = val.substr(0, nindex);
    std::string status = val.substr(nindex);
    std::string valve = val.substr(1, 2);
    std::string tosend;
    if (val.find("_NO_") != std::string::npos) {
      //                std::cout<<"NO valve"<<std::endl;
      tosend = valveFuncNO(stoi(valve), status, valstr, _st);
    } else {
      //                std::cout<<"regular valve"<<std::endl;
      tosend = valveFunc(stoi(valve), status, valstr, _st);
    }
    std::cout << tosend << std::endl;
    _sp->write_some(ba::buffer(tosend));
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
                     const std::shared_ptr<State> &st)
    : _ioc(ioContext), _ss(stopSource), _st(st) {
  try {
    _sp = std::make_shared<ba::serial_port>(_ioc, "/dev/ttyACM1");
  } catch (...) {
    std::cout << "ARDUINO" << std::endl;
  }
  _sp->set_option(ba::serial_port_base::baud_rate(115200));
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
