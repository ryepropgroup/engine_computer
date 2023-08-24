#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ba = boost::asio;
namespace bs = boost::signals2;
using json = nlohmann::json;
using namespace std::chrono_literals;

extern std::mutex sigm;
extern ba::thread_pool pool;
extern std::condition_variable sigcondition;
extern std::string vData;
extern std::atomic<bool> isString;
extern std::atomic<bool> enabled;
extern std::condition_variable suspend_write;
namespace mach {
using Timestamp = uint64_t;
Timestamp now();
union udouble {
  double d;
  unsigned long long u;
};
inline const std::map<std::string, std::string> vlj = {
    {"V11_NO", "DIO0"},  {"V30", "DIO1"},     {"V12_NO", "DIO2"},
    {"V31", "DIO3"},     {"V20", "DIO4"},     {"V32", "DIO5"},
    {"V21", "DIO6"},     {"V33_NO", "DIO7"},  {"V35_NO", "DIO8"},
    {"V36", "DIO9"},     {"V34", "DIO10"},    {"V38_NO", "DIO11"},
    {"V37", "DIO12"},    {"V23_NO", "DIO13"}, {"V10", "DIO14"},
    {"V22_NO", "DIO15"},
};

inline const std::map<std::string, std::string> vljf = {
    {"DIO0", "V11_NO"},  {"DIO1", "V30"},     {"DIO2", "V12_NO"},
    {"DIO3", "V31"},     {"DIO4", "V20"},     {"DIO5", "V32"},
    {"DIO6", "V21"},     {"DIO7", "V33_NO"},  {"DIO8", "V35_NO"},
    {"DIO9", "V36"},     {"DIO10", "V34"},    {"DIO11", "V38_NO"},
    {"DIO12", "V37"},    {"DIO13", "V23_NO"}, {"DIO14", "V10"},
    {"DIO15", "V22_NO"},
};

inline std::mutex coutm;
struct Sensor;
struct LJSensors;
struct SocketConn;
struct Server;
inline unsigned short PORT = 6970;

uint64_t sToMs(uint32_t seconds);
void sleep(uint64_t milliseconds);
void dispatchValve(const std::string &name, int handle);

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
/*
 * labjack 1
 * p31 -> AIN 0
 * p21 -> AIN 2
 * p32 -> AIN 3
 * p10 -> AIN 4
 * TC 1 -> AIN 7
 * TC 2 -> AIN 9
 * p22 -> AIN 11
 *
 * labjack 2
 * p20 -> AIN 1
 * p30 -> AIN 7
 * pINJ -> AIN 0
 * lc -> AIN 2
 * inj1 -> AIN 5
 * inj2 -> AIN 6
 * ign -> AIN 4
 *
 */
struct LJSensors {
  double p10val = 0, p21val = 0, p31val = 0, p22val = 0, p32val = 0, p20val = 0,
         p30val = 0, pinjval = 0, lcellval = 0, t1val = 0, t2val = 0,
         inj1val = 0, inj2val = 0, ignval = 0;

  // TODO: swap to proper AINs to change
  mach::Sensor *p31 = // p31
      new Sensor(std::string("AIN0"),
                 std::vector<std::string>{"AIN0_RANGE", /*"AIN0_NEGATIVE_CH"*/},
                 std::vector<double>{10 /*, 1.0*/});
  mach::Sensor *p21 = // p21
      new Sensor(std::string("AIN2"),
                 std::vector<std::string>{"AIN2_RANGE", /*"AIN4_NEGATIVE_CH"*/},
                 std::vector<double>{10 /*, 3.0*/});
  mach::Sensor *p10 = // p10
      new Sensor(
          std::string("AIN4"),
          std::vector<std::string>{"AIN4_RANGE" /*, "AIN4_NEGATIVE_CH"*/},
          std::vector<double>{10 /*, 5.0*/});
  mach::Sensor *p22 = // p22
      new Sensor(std::string("AIN11"), std::vector<std::string>{"AIN11_range"},
                 std::vector<double>{10});
  mach::Sensor *p32 = // p32
      new Sensor(std::string("AIN3"), std::vector<std::string>{"AIN3_range"},
                 std::vector<double>{10});
  mach::Sensor *t1 = new Sensor(
      std::string("AIN7"),
      std::vector<std::string>{"AIN7_EF_INDEX", "AIN7_EF_CONFIG_B",
                               "AIN7_EF_CONFIG_D", "AIN7_EF_CONFIG_E",
                               "AIN7_EF_CONFIG_A", "AIN7_NEGATIVE_CH"},
      std::vector<double>{22, 60052, 1.0, 0.0, 1, 1});

  mach::Sensor *t2 =
      new Sensor(std::string("AIN9"),
                 std::vector<std::string>{
                     "AIN9_EF_INDEX", "AIN9_EF_CONFIG_B", "AIN9_EF_CONFIG_D",
                     "AIN9_EF_CONFIG_E", "AIN9_EF_CONFIG_A"},
                 std::vector<double>{22, 60052, 1.0, 0.0, 1});
  mach::Sensor *pinj =
      new Sensor(std::string("AIN0"), std::vector<std::string>{"AIN0_RANGE"},
                 std::vector<double>{10});
  mach::Sensor *lc = new Sensor(std::string("AIN2"),
                                std::vector<std::string>{"AIN2_NEGATIVE_CH"},
                                std::vector<double>{3});
  mach::Sensor *inj1 =
      new Sensor(std::string("AIN5"), std::vector<std::string>{"AIN5_RANGE"},
                 std::vector<double>{0.1});
  mach::Sensor *inj2 =
      new Sensor(std::string("AIN6"), std::vector<std::string>{"AIN6_RANGE"},
                 std::vector<double>{0.1});
  mach::Sensor *ign =
      new Sensor(std::string("AIN4"), std::vector<std::string>{"AIN4_RANGE"},
                 std::vector<double>{0.1});
  mach::Sensor *p20 = // p20
      new Sensor(std::string("AIN1"), std::vector<std::string>{"AIN1_RANGE"},
                 std::vector<double>{10});
  mach::Sensor *p30 = // p30
      new Sensor(std::string("AIN7"), std::vector<std::string>{"AIN7_RANGE"},
                 std::vector<double>{10});
};

struct State {
private:
  bool launched = false;
  std::map<std::string, bool> valves;
  mutable std::shared_mutex slock;

public:
  LJSensors lj;
  State() {
    valves["V10"] = false;
    valves["V11_NO"] = true; // NO valve
    valves["V12_NO"] = true; // NO valve
    valves["V20"] = false;
    valves["V21"] = false;
    valves["V22_NO"] = true; // NO valve
    valves["V23_NO"] = true; // NO valve
    valves["V30"] = false;
    valves["V31"] = false;
    valves["V32"] = false;
    valves["V33_NO"] = true; // NO valve
    valves["V34"] = false;
    valves["V35_NO"] = true; // NO valve
    valves["V36"] = false;
    valves["V37"] = false;
    valves["V38_NO"] = true; // NO valve
  }

  [[nodiscard]] json toJSON() const {
    std::shared_lock guard(slock);
    std::vector<std::string> db = {
        std::to_string(int(lj.p10val)),   std::to_string(int(lj.p21val)),
        std::to_string(int(lj.p31val)),   std::to_string(int(lj.t1val)),
        std::to_string(int(lj.t2val)),    std::to_string(int(lj.p20val)),
        std::to_string(int(lj.p30val)),   std::to_string(int(lj.p22val)),
        std::to_string(int(lj.p32val)),   std::to_string(int(lj.pinjval)),
        std::to_string(int(lj.lcellval)), std::to_string(int(lj.inj1val)),
        std::to_string(int(lj.inj2val)),  std::to_string(int(lj.ignval)),
    };
    for (auto &d : db) {
      while (d.length() < 4) {
        d = "0" + d;
      }
    }
    json jsonState;
    jsonState["launched"] = launched;
    jsonState["writing"] = bool(enabled);
    jsonState["lj"]["p10val"] = db[0];
    jsonState["lj"]["p21val"] = db[1];
    jsonState["lj"]["p31val"] = db[2];
    jsonState["lj"]["t1val"] = db[3];
    jsonState["lj"]["t2val"] = db[4];
    jsonState["lj"]["p20val"] = db[5];
    jsonState["lj"]["p30val"] = db[6];
    jsonState["lj"]["p22val"] = db[7];
    jsonState["lj"]["p32val"] = db[8];
    jsonState["lj"]["pinjval"] = db[9];
    jsonState["lj"]["lcval"] = db[10];
    jsonState["lj"]["inj1val"] = db[11];
    jsonState["lj"]["inj2val"] = db[12];
    jsonState["lj"]["ignval"] = db[13];
    jsonState["valves"] = valves;
    return jsonState;
  }

  [[nodiscard]] bool isLaunched() const {
    std::shared_lock guard(slock);
    return launched;
  }

  void setLaunched(bool status) {
    std::unique_lock guard(slock);
    State::launched = status;
  }

  [[nodiscard]] const std::map<std::string, bool> &getValves() const {
    std::shared_lock guard(slock);
    return valves;
  }

  void setValves(const std::map<std::string, bool> &v) {
    std::unique_lock guard(slock);
    State::valves = v;
  }
};

struct SocketConn : std::enable_shared_from_this<SocketConn> {
  explicit SocketConn(ba::any_io_executor const &ioContext,
                      std::stop_source &stopSource, const int &lj);

  void start();

  void send(const std::string msg, const bool immediate = false);

private:
  void input();

  bool nq(const std::string msg, const bool immediate);

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
  int labjack;
};

struct Server {
  explicit Server(ba::io_context &ioContext, std::stop_source &stopSource,
                  const std::shared_ptr<State> &st, const int &labjack);

  void stop();

  size_t emit(const std::shared_ptr<State> &s);

private:
  size_t reg(const std::shared_ptr<SocketConn> &c);

  void accept();

  ba::io_context &_ioc;
  ba::ip::tcp::acceptor _acc{_ioc, ba::ip::tcp::v4()};
  bs::signal<void(std::shared_ptr<State> const &s)> _emit_event;
  std::stop_source _ss;
  std::shared_ptr<State> _st;
  int labjack;
};

} // namespace mach
