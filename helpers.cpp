#include "helpers.h"
#include "LabJackM.h"
#include <memory>
#include <string>
#include <vector>
extern std::mutex sigm;
extern std::condition_variable sigcondition;
extern std::string vData;
extern std::atomic<bool> isString;
extern std::atomic<bool> enabled;
extern std::condition_variable suspend_write;
mach::Timestamp mach::now() {
  return mach::Timestamp(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count());
}
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

void mach::dispatchValve(const std::string& name , int handle) {
  std::cout<<"potato"<<std::endl;
  if(name=="wstart"){
    enabled = true;
    suspend_write.notify_all();
    return;
  }
  if(name=="wstop"){
    enabled=false;
    suspend_write.notify_all();
    return;
  }
  if (name == "stop") {
    LJM_eWriteName(handle, mach::vlj.at("V10").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V21").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V20").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V30").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V31").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V32").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V34").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V36").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V37").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V11_NO").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V12_NO").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V22_NO").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V23_NO").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V33_NO").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V35_NO").c_str(), 1);
    LJM_eWriteName(handle, mach::vlj.at("V38_NO").c_str(), 1);
    return;
  }
  /*
   * 3.6.1 which is tank pressurization
   * open v21
   * open v32 at the same time preferably
   *
   */
  if (name == "361") {
    return;
  }
  /*
   * 3.6.2 tank pressurization continued
   * open v31
   * 1 second later
   * close v32
   */
  if (name == "362") {
    return;
  }
  /*
   * 3.5.1 nitrous fill probably
   * open v21
   * open v32 at the same time preferably
   */
  if (name == "351") {
    return;
  }
  /*
   * 4.1.1f which i guess is eth
   * open v21
   * 10 seconds later
   * open v20 (t-0)
   * 7 seconds later
   * close v21
   * 3 seconds later
   * close v20
   */
  if (name == "411f") {
    // open v21
    //        LJM_eWriteName(handle, vlj.at("V21").c_str(), 0);
    std::this_thread::sleep_for(10s);
    // open v20
    LJM_eWriteName(handle, vlj.at("V20").c_str(), 0);
    std::this_thread::sleep_for(7s);
    // close v21
    LJM_eWriteName(handle, vlj.at("V21").c_str(), 1);
    std::this_thread::sleep_for(3s);
    // close v20
    LJM_eWriteName(handle, vlj.at("V20").c_str(), 1);
    // close
    return;
  }
  /*
   * 4.1.1o which i guess is oxidizer
   * open v34
   * 10 seconds later
   * close v34
   * open v30 preferably at same time (t-0)
   * 3 seconds later
   * close v30
   * close v31 preferably at the same time
   * open v33 preferably also at the same time
   * 7 seconds later
   * close v33
   */
  if (name == "411o") {
    // open v34
    LJM_eWriteName(handle, vlj.at("V34").c_str(), 0);
    std::this_thread::sleep_for(10s);
    // close v34
    LJM_eWriteName(handle, vlj.at("V34").c_str(), 1);
    // open v30
    LJM_eWriteName(handle, vlj.at("V30").c_str(), 0);
    std::this_thread::sleep_for(3s);
    // close v30 and v31 and open v33
    LJM_eWriteName(handle, vlj.at("V30").c_str(), 1);
    LJM_eWriteName(handle, vlj.at("V31").c_str(), 1);
    LJM_eWriteName(handle, vlj.at("V33_NO").c_str(), 0);
    std::this_thread::sleep_for(7s);
    LJM_eWriteName(handle, vlj.at("V33_NO").c_str(), 1);
    return;
  };
  /*
   * 4.1.1 which i guess is oxidizer final
   * open v34
   * 10 seconds later
   * close v34
   * (IGNITER LATER) [t-0]
   * supposed thermocouple detection
   * open v20
   * .25 seconds later
   * open v30
   * 5 seconds later
   * close v30
   * close v31 preferably at the same time
   * open v33 preferably also at the same time
   * 7 seconds later
   * close v33
   * close v21
   */
  if (name == "411") {
    return;
  }
  /*
   * close v11, v12, v35, v36, v23
   */
  if (name == "cavv") {
    return;
  }
  /*
   * open v11,v12,v35,v36,v23
   */
  if (name == "oavv") {
    return;
  }
  size_t nindex = name.rfind('_');
  std::string valve = name.substr(0, nindex);
  std::string status = name.substr(nindex + 1);
  std::cout << valve << status << std::endl;
  size_t count =
      std::count_if(name.begin(), name.end(), [](char c) { return c == '_'; });
  if (count == 2) {
    if (status == "open")
      LJM_eWriteName(handle, mach::vlj.at(valve).c_str(), 1);
    else
      LJM_eWriteName(handle, mach::vlj.at(valve).c_str(), 0);
  } else {
    if (status == "open")
      LJM_eWriteName(handle, mach::vlj.at(valve).c_str(), 0);
    else
      LJM_eWriteName(handle, mach::vlj.at(valve).c_str(), 1);
  }
}

void mach::SocketConn::start() { _read(); }

mach::SocketConn::SocketConn(ba::any_io_executor const &ioContext,
                             std::stop_source &stopSource, const int &lj)
    : _s(ioContext), _ss(stopSource), labjack(lj), qioc((ba::io_context &)ioContext){};

void mach::SocketConn::input() {
  std::string val;
  if (std::getline(std::istream(&_buf), val)) {
    {
      std::lock_guard<std::mutex> l(coutm);
      std::cout << val << std::endl;
    }
    {
      std::lock_guard<std::mutex> lock(sigm);
      vData = val;
      isString = true;
    }
    sigcondition.notify_all();

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
    // TODO: server constructor grab LJ control magic
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
