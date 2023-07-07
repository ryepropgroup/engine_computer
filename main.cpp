#include "LabJackM.h"
#include <bitset>
#include <boost/asio.hpp>
#include <fstream>
#include <iostream>
#include "helpers.h"
#include "LJM_Utilities.h"

using namespace std::chrono_literals;
namespace ba = boost::asio;

std::stop_source test;
std::stop_token token = test.get_token();

void handlesigint(int signal_num){
  test.request_stop();
  exit(signal_num);
}
void run_signal(const std::shared_ptr<mach::State> st) {
    // LabJack Initialization
    long utn = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
    std::string utns = std::to_string(utn);
    std::string filename = utns += ".csv";

    std::ofstream file(filename);
    file<<"elapsed"<<","<<"p31"<<","<<"p21"<<","<<"p10"<<std::endl;

    while (!token->stop_requested()) {
        long now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        long elapsed = now-utn;
        std::string strelapsed = std::to_string(now);
        file<<strelapsed<<","<<std::to_string(st->lj.p31val)<<","<<std::to_string(st->lj.p21val)<<","<<std::to_string(st->lj.p10val)<<"\n";
    }
    file<<std::endl;
}

int main() {
    signal(SIGINT, handlesigint);
    std::shared_ptr<mach::State> sharedState(new mach::State);
    ba::io_context ioContext;
    int err, handle, errorAddress = INITIAL_ERR_ADDRESS;
    try {
        err = LJM_Open(LJM_dtANY, LJM_ctANY, "SlowJack", &handle);
        ErrorCheck(err, "LJM_Open");
        err = LJM_eWriteNames(handle, int(sharedState->lj.p10->params.size()),
                              mach::vectorToChar(sharedState->lj.p10->params),
                              mach::vectorToDouble(sharedState->lj.p10->settings), &errorAddress);
        ErrorCheck(err, "LJM_eWriteNames");
        err = LJM_eWriteNames(handle, int(sharedState->lj.p21->params.size()),
                              mach::vectorToChar(sharedState->lj.p21->params),
                              mach::vectorToDouble(sharedState->lj.p21->settings), &errorAddress);
        ErrorCheck(err, "LJM_eWriteNames");
        err = LJM_eWriteNames(handle, int(sharedState->lj.p31->params.size()),
                              mach::vectorToChar(sharedState->lj.p31->params),
                              mach::vectorToDouble(sharedState->lj.p31->settings), &errorAddress);
        ErrorCheck(err, "LJM_eWriteNames");}
    catch (...) {
        std::cout << "LABJACK ISSUE" << std::endl;
    }
    mach::Server s(ioContext, test, sharedState, handle);
    std::jthread th([&ioContext] { ioContext.run(); });
    std::jthread th2(run_signal, sharedState);
    std::this_thread::sleep_for(10s);
    while (!token.stop_requested()) {
        std::this_thread::sleep_for(10ms);
        mach::udouble test;
        LJM_eReadName(handle, "DIO_STATE",&test.d);
        std::bitset<sizeof(double)*8>btest(test.u);
        auto bits = btest.to_string();
        auto vals = bits.substr(18,16);
        std::reverse(vals.begin(),vals.end());
        auto state = sharedState->getValves();
        for (size_t i = 0; i < vals.size(); i++) {
            std::string k = "DIO"+std::to_string(i);
            char v = vals.at(i);
            int nb = v-'0';
            if(mach::vljf.at(k).find('_')!=std::string::npos){
               state[mach::vljf.at(k)] = nb;
            }
            else{
                state[mach::vljf.at(k)] = !nb;
            }
        }
        err = LJM_eReadName(handle, sharedState->lj.p10->name.c_str(), &sharedState->lj.p10val);
        err = LJM_eReadName(handle, sharedState->lj.p21->name.c_str(), &sharedState->lj.p21val);
        err = LJM_eReadName(handle, sharedState->lj.p31->name.c_str(), &sharedState->lj.p31val);
//        err = LJM_eReadName(handle, sharedState->lj.t2->name.c_str(), &sharedState->lj.t2val);
      std::cout<<sharedState->lj.p10val<<std::endl;
      std::cout<<"-----------"<<std::endl;
        sharedState->lj.p10val *= 300;
        sharedState->lj.p21val *= 300;
        sharedState->lj.p31val *= 300;
        sharedState->setValves(state);
        std::cout<<sharedState->lj.p10val<<std::endl;
//        std::cout<<sharedState->toJSON()<<std::endl;
        s.emit(sharedState);
    }
    s.stop();
    th2.join();
    th.join();
    return 0;
}
