#include <iostream>
#include <modbus/modbus-tcp.h>
#include <boost/signals2.hpp>
#include <thread>

struct ljSetup {
    double p1 = 0, p2 = 0, p3 = 0;
    const char *p1Name = {"AIN0"};
    const char *p2Name = {"AIN2"};
    const char *p3Name = {"AIN4"};
    std::array<const char *, 2> p1Params = {"AIN0_RANGE", "AIN0_NEGATIVE_CH"};
    std::array<const char *, 2> p2Params = {"AIN2_RANGE", "AIN2_NEGATIVE_CH"};
    std::array<const char *, 2> p4Params = {"AIN4_RANGE", "AIN4_NEGATIVE_CH"};
    std::array<double, 2> p1Vals = {0.1, 1.0};
    std::array<double, 2> p2Vals = {0.1, 3.0};
    std::array<double, 2> p4Vals = {0.1, 5.0};
};

void print_p1Name(ljSetup lj) {
    std::cout << lj.p1Name << std::endl;
}

void print_p2Name(ljSetup lj) {
    std::cout << lj.p2Name << std::endl;
}

void run_signal(std::shared_ptr<boost::signals2::signal<void(ljSetup)>> sig, std::stop_token t){
    ljSetup lj = *new ljSetup();
    while(!t.stop_requested())
    {
        (*sig)(lj);
    }
    std::cout<<"done"<<std::endl;
}
int main() {
    using namespace std::chrono_literals;
    // modbus_t *mb;
    // uint16_t tab_reg[32];
    // mb = modbus_new_tcp("", 502);
    // modbus_connect(mb);
    // modbus_read_registers(mb, 16386, 5, tab_reg);
    // modbus_close(mb);
    // modbus_free(mb);
    std::stop_source test;
    std::stop_token token = test.get_token();
    std::shared_ptr<boost::signals2::signal<void(ljSetup)>> testSig(new boost::signals2::signal<void(ljSetup)>);
    testSig->connect(&print_p1Name);
    testSig->connect(&print_p2Name);
    std::jthread one(run_signal, testSig, token);
    std::this_thread::sleep_for(1ms);
    test.request_stop();
    return 0;
}
