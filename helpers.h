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
    union udouble {
        double d;
        unsigned long long u;
    };
    const std::map<std::string, std::string> vlj = {
            {"V11_NO", "DIO0"},
            {"V30",    "DIO1"},
            {"V12_NO", "DIO2"},
            {"V31",    "DIO3"},
            {"V20",    "DIO4"},
            {"V32",    "DIO5"},
            {"V21",    "DIO6"},
            {"V33_NO", "DIO7"},
            {"V35_NO", "DIO8"},
            {"V36",    "DIO9"},
            {"V34",    "DIO10"},
            {"V38_NO", "DIO11"},
            {"V37",    "DIO12"},
            {"V23_NO", "DIO13"},
            {"V10",    "DIO14"},
            {"V22_NO", "DIO15"},
    };

    const std::map<std::string, std::string> vljf = {
            {"DIO0", "V11_NO"},
            {"DIO1",    "V30"},
            {"DIO2", "V12_NO"},
            {"DIO3",    "V31"},
            {"DIO4",    "V20"},
            {"DIO5",    "V32"},
            {"DIO6",    "V21"},
            {"DIO7", "V33_NO"},
            {"DIO8", "V35_NO"},
            {"DIO9",    "V36"},
            {"DIO10",    "V34"},
            {"DIO11", "V38_NO"},
            {"DIO12",    "V37"},
            {"DIO13", "V23_NO"},
            {"DIO14",    "V10"},
            {"DIO15", "V22_NO"},
    };
    inline std::mutex coutm;
    struct Sensor;
    struct LJSensors;
    struct SocketConn;
    struct Server;
    inline unsigned short PORT = 6969;

    void dispatchValve(std::string name, int handle);

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
        //TODO: swap to proper AINs to change
        double p10val = 0, p21val = 0, p31val = 0, t2val, t3val;
        mach::Sensor *p31 = // p31
                new Sensor(std::string("AIN0"),
                           std::vector<std::string>{"AIN0_range", /*"AIN0_NEGATIVE_CH"*/},
                           std::vector<double>{10 /*, 1.0*/});
        mach::Sensor *p21 = // p21
                new Sensor(std::string("AIN2"),
                           std::vector<std::string>{"AIN2_range", /*"AIN2_NEGATIVE_CH"*/},
                           std::vector<double>{10 /*, 3.0*/});
        mach::Sensor *p10 = // p10
                new Sensor(
                        std::string("AIN4"),
                        std::vector<std::string>{"AIN4_range" /*, "AIN4_NEGATIVE_CH"*/},
                        std::vector<double>{10 /*, 5.0*/});
        mach::Sensor *t2 =
                new Sensor(std::string("AIN6_EF_READ_A"),
                           std::vector<std::string>{
                                   "AIN6_EF_INDEX", "AIN6_EF_CONFIG_B", "AIN6_EF_CONFIG_D",
                                   "AIN6_EF_CONFIG_E", "AIN6_EF_CONFIG_A"},
                           std::vector<double>{22, 60052, 1.0, 0.0, 1});

        mach::Sensor *t3 =
                new Sensor(std::string("AIN6_EF_READ_A"),
                           std::vector<std::string>{
                                   "AIN6_EF_INDEX", "AIN6_EF_CONFIG_B", "AIN6_EF_CONFIG_D",
                                   "AIN6_EF_CONFIG_E", "AIN6_EF_CONFIG_A"},
                           std::vector<double>{22, 60052, 1.0, 0.0, 1});
    };

    struct State {
    private:
        bool launched = false;
        LJSensors lj;
        std::map<std::string, bool> valves;
        mutable std::mutex slock;
    public:
        State() {
            valves["V10"] = false;
            valves["V11_NO"] = true; // NO valve
            valves["V12_NO"] = true; // NO valve
            valves["V20"] = false;
            valves["V21"] = false;
            valves["V22_NO"] = true; //NO valve
            valves["V23_NO"] = true; // NO valve
            valves["V30"] = false;
            valves["V31"] = false;
            valves["V32"] = false;
            valves["V33_NO"] = true; // NO valve
            valves["V34"] = false;
            valves["V35_NO"] = true; //NO valve
            valves["V36"] = false;
            valves["V37"] = false;
            valves["V38_NO"] = true; // NO valve
        }

        [[nodiscard]] json toJSON() const {
            std::lock_guard<std::mutex> guard(slock);
            std::vector<std::string> db = {
                    std::to_string(int(lj.p10val)), std::to_string(int(lj.p21val)),
                    std::to_string(int(lj.p31val)), std::to_string(int(lj.t2val)), std::to_string(int(lj.t3val))};
            for (auto &d: db) {
                while (d.length() < 4) {
                    d = "0" + d;
                }
            }
            json jsonState;
            jsonState["launched"] = launched;
            jsonState["lj"]["p10val"] = db[0];
            jsonState["lj"]["p21val"] = db[1];
            jsonState["lj"]["p31val"] = db[2];
            jsonState["lj"]["t2val"] = db[3];
            jsonState["lj"]["t3val"] = db[4];
            jsonState["valves"] = valves;
            return jsonState;
        }

        [[nodiscard]] bool isLaunched() const {
            std::lock_guard<std::mutex> guard(slock);
            return launched;
        }

        void setLaunched(bool status) {
            std::lock_guard<std::mutex> guard(slock);
            State::launched = status;
        }

        [[nodiscard]] const std::map<std::string, bool> &getValves() const {
            std::lock_guard<std::mutex> guard(slock);
            return valves;
        }

        void setValves(const std::map<std::string, bool> &v) {
            std::lock_guard<std::mutex> guard(slock);
            State::valves = v;
        }
    };

//    std::string valveFuncNO(const int num, const std::string &status,
//                            const std::string &valve,
//                            const std::shared_ptr<State> &state);

//    std::string valveFunc(const int num, const std::string &status,
//                          const std::string &valve,
//                          const std::shared_ptr<State> &state);
    struct SocketConn : std::enable_shared_from_this<SocketConn> {
        explicit SocketConn(ba::any_io_executor const &ioContext,
                            std::stop_source &stopSource, const int &lj);

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
