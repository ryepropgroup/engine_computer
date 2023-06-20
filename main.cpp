#include "LJM_Utilities.h"
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <fstream>
#include <iostream>
#include <modbus/modbus-tcp.h>
#include <thread>
#include <utility>
#include "helpers.h"

using namespace std::chrono_literals;
namespace ba = boost::asio;


void run_signal(const std::stop_token *t, mach::Server *s, const std::shared_ptr<mach::State> st) {
    // LabJack Initialization
    int err, handle, errorAddress = INITIAL_ERR_ADDRESS;
    long utn = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
    std::string utns = std::to_string(utn);
    std::string filename = utns += ".csv";
    try {
        err = LJM_Open(LJM_dtANY, LJM_ctANY, "LJM_idANY", &handle);
        ErrorCheck(err, "LJM_Open");
    }
    catch (...) {
        s->stop();
        std::cout << "LABJACK" << std::endl;
    }
    std::ofstream file(filename);
    file<<"elapsed"<<","<<"t3"<<","<<"p31"<<","<<"p21"<<","<<"p10"<<std::endl;
    err = LJM_eWriteNames(handle, int(st->lj.p1->params.size()),
                          mach::vectorToChar(st->lj.p1->params),
                          mach::vectorToDouble(st->lj.p1->settings), &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(handle, int(st->lj.p2->params.size()),
                          mach::vectorToChar(st->lj.p2->params),
                          mach::vectorToDouble(st->lj.p2->settings), &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(handle, int(st->lj.p3->params.size()),
                          mach::vectorToChar(st->lj.p3->params),
                          mach::vectorToDouble(st->lj.p3->settings), &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(handle, int(st->lj.t1->params.size()), mach::vectorToChar(st->lj.t1->params),
                          mach::vectorToDouble(st->lj.t1->settings), &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    while (!t->stop_requested()) {
        long now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        long elapsed = now-utn;
        std::string strelapsed = std::to_string(now);
        err = LJM_eReadName(handle, st->lj.p1->name.c_str(), &st->lj.p1val);
        ErrorCheck(err, "LJM_eReadName");
        err = LJM_eReadName(handle, st->lj.p2->name.c_str(), &st->lj.p2val);
        ErrorCheck(err, "LJM_eReadName");
        err = LJM_eReadName(handle, st->lj.p3->name.c_str(), &st->lj.p3val);
        ErrorCheck(err, "LJM_eReadName");
        err = LJM_eReadName(handle, st->lj.t1->name.c_str(), &st->lj.t1val);
        ErrorCheck(err, "LJM_eReadName");
        st->lj.p1val *= 300;
        st->lj.p2val *= 300;
        st->lj.p3val *= 300;
        file<<strelapsed<<","<<std::to_string(st->lj.t1val)<<","<<std::to_string(st->lj.p1val)<<","<<std::to_string(st->lj.p2val)<<","<<std::to_string(st->lj.p3val)<<std::endl;
        s->emit(st);
    }
    std::lock_guard<std::mutex> l(mach::coutm);
    std::cout << "done" << std::endl;
}

int main() {
    std::stop_source test;
    std::stop_token token = test.get_token();
    ba::io_context ioContext;
    auto st = std::make_shared<mach::State>();
    mach::Server s(ioContext, test, st);
    std::jthread th([&ioContext] { ioContext.run(); });
    std::this_thread::sleep_for(10s);
    std::jthread th2(run_signal, &token, &s, st);
    while (!token.stop_requested()) {
        std::this_thread::sleep_for(10ms);
    }
    s.stop();
    th.join();
    return 0;
}
