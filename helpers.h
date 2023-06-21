#include "./lib/json.hpp"
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <fstream>
#include <iostream>
#include <modbus/modbus-tcp.h>
#include <thread>
#include <utility>
#include <vector>
namespace ba = boost::asio;
namespace bs = boost::signals2;
using json = nlohmann::json;
using namespace std::chrono_literals;
namespace mach {
inline std::mutex coutm;
struct Sensor;
struct LJSensors;
struct SocketConn;
struct Server;
inline unsigned short PORT = 6969;

const char **vectorToChar(const std::vector<std::string> &stringVector);

const double *vectorToDouble(const std::vector<double> &doubleVector);

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
  double p1val = 0, p2val = 0, p3val = 0, t1val;
  mach::Sensor *p1 = // p31
      new Sensor(std::string("AIN0"),
                 std::vector<std::string>{"AIN0_range", /*"AIN0_NEGATIVE_CH"*/},
                 std::vector<double>{10 /*, 1.0*/});
  mach::Sensor *p2 = // p21
      new Sensor(std::string("AIN2"),
                 std::vector<std::string>{"AIN2_range", /*"AIN2_NEGATIVE_CH"*/},
                 std::vector<double>{10 /*, 3.0*/});
  mach::Sensor *p3 = // p10
      new Sensor(
          std::string("AIN4"),
          std::vector<std::string>{"AIN4_range" /*, "AIN4_NEGATIVE_CH"*/},
          std::vector<double>{10 /*, 5.0*/});
  // t3
  mach::Sensor *t1 =
      new Sensor(std::string("AIN6_EF_READ_A"),
                 std::vector<std::string>{
                     "AIN6_EF_INDEX", "AIN6_EF_CONFIG_B", "AIN6_EF_CONFIG_D",
                     "AIN6_EF_CONFIG_E", "AIN6_EF_CONFIG_A"},
                 std::vector<double>{22, 60052, 1.0, 0.0, 1});
};
struct State {
  State() {
    valves["V10_SB"] = false;
    valves["V11_S"] = true; // NO valve
    valves["V12_S"] = true; // NO valve
    valves["V20_SB"] = false;
    valves["V21_SB"] = false;
    valves["V23_SB"] = true; // NO valve
    valves["V30_SB"] = false;
    valves["V31_SB"] = false;
    valves["V34_SB"] = true; // NO valve
    valves["V35_S"] = false;
    valves["V36_SB"] = false;
    valves["V37_S"] = true; // NO valve
  }

  bool launched = false;
  LJSensors lj;
  std::map<std::string, bool> valves;

  [[nodiscard]] json toJSON() const {
    std::vector<std::string> db = {
        std::to_string(int(lj.p1val)), std::to_string(int(lj.p2val)),
        std::to_string(int(lj.p3val)), std::to_string(int(lj.t1val))};
    for (auto &d : db) {
      while (d.length() < 4) {
        d = "0" + d;
      }
    }
    json jsonState;
    jsonState["launched"] = launched;
    jsonState["lj"]["p1val"] = db[0];
    jsonState["lj"]["p2val"] = db[1];
    jsonState["lj"]["p3val"] = db[2];
    jsonState["lj"]["t1val"] = db[3];
    jsonState["valves"] = valves;
    return jsonState;
  }
};

std::string valveFuncNO(const int num, const std::string &status,
                        const std::string &valve,
                        const std::shared_ptr<State> &state);

std::string valveFunc(const int num, const std::string &status,
                      const std::string &valve,
                      const std::shared_ptr<State> &state);

struct SocketConn : std::enable_shared_from_this<SocketConn> {
  explicit SocketConn(ba::any_io_executor const &ioContext,
                      std::stop_source &stopSource,
                      std::shared_ptr<ba::serial_port> sp,
                      std::shared_ptr<State> st);

  void start();

  void send(std::string msg, bool immediate = false);

private:
  void input();

  bool nq(std::string msg, bool immediate);

  // return true if messages are pending post-dequeue
  bool dq();

  void _read();

  void _write();
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
  explicit Server(ba::io_context &ioContext, std::stop_source &stopSource,
                  const std::shared_ptr<State> &st);

  void stop();

  size_t emit(const std::shared_ptr<State> &s);

private:
  size_t reg(const std::shared_ptr<SocketConn> &c);

  void accept();

  ba::io_context &_ioc;
  ba::ip::tcp::acceptor _acc{_ioc, ba::ip::tcp::v4()};
  bs::signal<void(std::shared_ptr<State> const &s)> _emit_event;
  std::stop_source _ss;
  std::shared_ptr<ba::serial_port> _sp;
  std::shared_ptr<State> _st;
};

} // namespace mach
