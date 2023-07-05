#include "LabJackM.h"
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <fstream>
#include <iostream>
#include <modbus/modbus-tcp.h>
#include <thread>
#include <utility>
#include "helpers.h"
#include "LJM_Utilities.h"

using namespace std::chrono_literals;
namespace ba = boost::asio;


//void run_signal(const std::stop_token *t, mach::Server *s, const std::shared_ptr<mach::State> st) {
//    // LabJack Initialization
//    long utn = std::chrono::duration_cast<std::chrono::seconds>(
//                    std::chrono::system_clock::now().time_since_epoch()).count();
//    std::string utns = std::to_string(utn);
//    std::string filename = utns += ".csv";
//
//    std::ofstream file(filename);
//    file<<"elapsed"<<","<<"t3"<<","<<"p31"<<","<<"p21"<<","<<"p10"<<std::endl;
//
//    while (!t->stop_requested()) {
//        //TODO: move writing to main function
//        long now = std::chrono::duration_cast<std::chrono::seconds>(
//                std::chrono::system_clock::now().time_since_epoch()).count();
//        long elapsed = now-utn;
//        std::string strelapsed = std::to_string(now);
//        // TODO: move reading to other thread
//        err = LJM_eReadName(handle, st->lj.p1->name.c_str(), &st->lj.p1val);
//        ErrorCheck(err, "LJM_eReadName");
//        err = LJM_eReadName(handle, st->lj.p2->name.c_str(), &st->lj.p2val);
//        ErrorCheck(err, "LJM_eReadName");
//        err = LJM_eReadName(handle, st->lj.p3->name.c_str(), &st->lj.p3val);
//        ErrorCheck(err, "LJM_eReadName");
//        err = LJM_eReadName(handle, st->lj.t1->name.c_str(), &st->lj.t1val);
//        ErrorCheck(err, "LJM_eReadName");
//        st->lj.p1val *= 300;
//        st->lj.p2val *= 300;
//        st->lj.p3val *= 300;
//        file<<strelapsed<<","<<std::to_string(st->lj.t1val)<<","<<std::to_string(st->lj.p1val)<<","<<std::to_string(st->lj.p2val)<<","<<std::to_string(st->lj.p3val)<<std::endl;
//        s->emit(st);
//    }
//    std::lock_guard<std::mutex> l(mach::coutm);
//    std::cout << "done" << std::endl;
//}

int main() {
    std::shared_ptr<mach::State> sharedState(new mach::State);
    std::stop_source test;
    std::stop_token token = test.get_token();
    ba::io_context ioContext;
    int err, handle, errorAddress = INITIAL_ERR_ADDRESS;
    try {
        err = LJM_Open(LJM_dtANY, LJM_ctANY, "SlowJack", &handle);
        ErrorCheck(err, "LJM_Open");
        }
    catch (...) {
        std::cout << "LABJACK ISSUE" << std::endl;
    }
    mach::Server s(ioContext, test, sharedState, handle);
    std::jthread th([&ioContext] { ioContext.run(); });
    std::this_thread::sleep_for(10s);
    while (!token.stop_requested()) {
        std::this_thread::sleep_for(10ms);
        double test;
        LJM_eReadName(handle, "DIO_STATE",&test);
        auto btest = reinterpret_cast<char*>(&test);
        std::cout<<btest[0]<<std::endl;
//        auto state = sharedState->getValves();
//        for (auto io: mach::vlj) {
//            double val;
//            LJM_eReadName(handle, io.second.c_str(), &val);
//            if(io.first.find('_')!=std::string::npos){
//                state[io.first] = val;
//            }
//            else{
//            state[io.first] = !val;
//            }
//        }
//        sharedState->setValves(state)
        s.emit(sharedState);
    }
    s.stop();
    th.join();
    return 0;
}
