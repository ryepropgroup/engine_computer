#include <iostream>
#include <modbus/modbus-tcp.h>
#include <boost/signals2.hpp>
#include <thread>
#include "LJM_Utilities.h"
#include "json.hpp"
using json = nlohmann::json;
using namespace std::chrono_literals;

struct Sensor {
    Sensor(std::string name, std::vector<std::string> params, const std::vector<double>& settings) {
        this->name = std::move(name);
        this->params = std::move(params);
        this->settings = settings;
    }
    std::string name;
    std::vector<std::string> params;
    std::vector<double> settings;
};

struct LJSensors{
    double p1val = 0, p2val = 0, p3val = 0;
    Sensor *p1 = new Sensor(std::string("AIN0"), std::vector<std::string>{"AIN0_range","AIN0_NEGATIVE_CH"}, std::vector<double>{0.1, 1.0});
    Sensor *p2 = new Sensor(std::string("AIN2"), std::vector<std::string>{"AIN2_range","AIN2_NEGATIVE_CH"}, std::vector<double>{0.1, 3.0});
    Sensor *p4 = new Sensor(std::string("AIN4"), std::vector<std::string>{"AIN4_range","AIN4_NEGATIVE_CH"}, std::vector<double>{0.1, 5.0});
};

struct State{
    bool launched = false;
    LJSensors lj;
    std::map<std::string, bool> valves;
};
void print_p1Name(LJSensors lj) {
    std::cout << lj.p1->name << std::endl;
}

void print_p2Name(LJSensors lj) {
    std::cout << lj.p2->name << std::endl;
}

void run_signal(const boost::signals2::signal<void(LJSensors)> *sig, const std::stop_token *t) {
    LJSensors lj = *new LJSensors();
    while (!t->stop_requested()) {
        (*sig)(lj);
    }
    std::cout << "done" << std::endl;
}

void stop_func(const std::stop_source *ss) {
    std::this_thread::sleep_for(1ms);
    ss->request_stop();
}

int main() {
//     modbus_t *mb;
//     uint16_t tab_reg[32];
//     mb = modbus_new_tcp("", 502);
//     modbus_connect(mb);
//     modbus_read_registers(mb, 16386, 5, tab_reg);
//     modbus_close(mb);
//     modbus_free(mb);
    std::stop_source test;
    std::stop_token token = test.get_token();
    boost::signals2::signal<void(LJSensors)> testSig;
    testSig.connect(&print_p1Name);
    testSig.connect(&print_p2Name);
    std::jthread one(run_signal, &testSig, &token);
    std::jthread two(stop_func, &test);
    return 0;
}
