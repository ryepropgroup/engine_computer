#include "LJM_Utilities.h"
#include "LabJackM.h"
#include "helpers.h"
#include <array>
#include <bitset>
#include <boost/asio.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

using namespace std::chrono_literals;
namespace ba = boost::asio;
std::stop_source test;
std::stop_token token = test.get_token();
int SlowJack, FastJack;
int spr{1};
int numSlowJack{10}, numFastJack{8};
// AIN7, AIN9, AIN14, AIN0, AIN1, AIN2, AIN3, AIN4, AIN5, AIN11
const std::array<int, 10> slowScanList{14, 18, 28, 0, 2, 4, 6, 8, 10, 22};
// AIN 1, 7, 0, 2, 5, 6, 4, 14
const std::array<int, 8> fastScanList{2, 14, 0, 4, 10, 12, 16, 28};
auto scanListC = slowScanList.data();
auto scanListCFast = fastScanList.data();
double *scanRate = new double{10};
double *scanRateFast = new double{10};
std::mutex writem;
std::condition_variable suspend_write;
std::atomic<bool> enabled = true;
std::vector<std::jthread> threads{};
ba::thread_pool pool(50);
std::shared_ptr<ba::thread_pool> poolptr = std::make_shared<ba::thread_pool>(5);

void handlesigint(const int signal_num) {
  test.request_stop();
  std::cout << "Killing Threads" << std::endl;
  for (auto &thread : threads) {
    thread.request_stop();
    thread.join();
  }
  std::cout << "Disconnecting LabJack..."
            << "\n";
  int err = LJM_eStreamStop(SlowJack);
  ErrorCheck(err, "LJM_eStreamStop");
  err = LJM_eStreamStop(FastJack);
  ErrorCheck(err, "LJM_eStreamStop");
  exit(signal_num);
}
void run_signal(const std::shared_ptr<mach::State> st) {
  auto old_time = mach::now();
  auto utn = mach::now();
  auto filename =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream utns;
  utns << std::put_time(std::localtime(&filename), "%F-%T");
  std::ofstream file(utns.str() + ".csv");
  if (!file.is_open()) {
    std::cerr << "file not available" << std::endl;
    return;
  }

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
  // LabJack Initialization
  file << "elapsed"
       << ","
       << "p10"
       << ","
       << "p21"
       << ","
       << "p31"
       << ","
       << "p22"
       << ","
       << "p32"
       << ","
       << "t1"
       << ","
       << "t2"
       << ","
       << "p20"
       << ","
       << "p30"
       << ","
       << "pINJ"
       << ","
       << "inj1"
       << ","
       << "inj2"
       << ","
       << "ign"
       << ","
       << "lc"
       << "\n";

  while (!token.stop_requested()) {
    std::this_thread::sleep_for(69ms);
    std::unique_lock<std::mutex> lock(writem);
    suspend_write.wait(lock, [] { return bool(enabled); });
    mach::Timestamp now = mach::now();
    if ((now - old_time) > 0) {
      file << now << "," << st->lj.p10val << "," << st->lj.p21val << ","
           << st->lj.p31val << "," << st->lj.p22val << "," << st->lj.p32val
           << "," << st->lj.t1val << "," << st->lj.t2val << "," << st->lj.p20val
           << "," << st->lj.p30val << "," << st->lj.pinjval << ","
           << st->lj.inj1val << "," << st->lj.inj2val << "," << st->lj.ignval
           << "," << st->lj.lcellval << "\n";
      old_time = now;
    }
  }
  file.close();
}

int main() {
  signal(SIGINT, handlesigint);
  std::shared_ptr<mach::State> sharedState(new mach::State);
  ba::io_context ioContext;
  int err, errorAddress = INITIAL_ERR_ADDRESS;
  try {
    err = LJM_Open(LJM_dtANY, LJM_ctANY, "SlowJack", &SlowJack);
    ErrorCheck(err, "LJM_Open");
    err = LJM_Open(LJM_dtANY, LJM_ctANY, "FastJack", &FastJack);
    ErrorCheck(err, "LJM_Open");
    err = LJM_eWriteNames(SlowJack, int(sharedState->lj.p10->params.size()),
                          mach::vectorToChar(sharedState->lj.p10->params),
                          mach::vectorToDouble(sharedState->lj.p10->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(SlowJack, int(sharedState->lj.p21->params.size()),
                          mach::vectorToChar(sharedState->lj.p21->params),
                          mach::vectorToDouble(sharedState->lj.p21->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(SlowJack, int(sharedState->lj.p31->params.size()),
                          mach::vectorToChar(sharedState->lj.p31->params),
                          mach::vectorToDouble(sharedState->lj.p31->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(SlowJack, int(sharedState->lj.p22->params.size()),
                          mach::vectorToChar(sharedState->lj.p22->params),
                          mach::vectorToDouble(sharedState->lj.p22->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(SlowJack, int(sharedState->lj.p32->params.size()),
                          mach::vectorToChar(sharedState->lj.p32->params),
                          mach::vectorToDouble(sharedState->lj.p32->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    // err = LJM_eWriteNames(SlowJack, int(sharedState->lj.t1->params.size()),
    //                       mach::vectorToChar(sharedState->lj.t1->params),
    //                       mach::vectorToDouble(sharedState->lj.t1->settings),
    //                       &errorAddress);
    // ErrorCheck(err, "LJM_eWriteNames");
    // err = LJM_eWriteNames(SlowJack, int(sharedState->lj.t2->params.size()),
    //                       mach::vectorToChar(sharedState->lj.t2->params),
    //                       mach::vectorToDouble(sharedState->lj.t2->settings),
    //                       &errorAddress);
    // ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(FastJack, int(sharedState->lj.p20->params.size()),
                          mach::vectorToChar(sharedState->lj.p20->params),
                          mach::vectorToDouble(sharedState->lj.p20->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(FastJack, int(sharedState->lj.p30->params.size()),
                          mach::vectorToChar(sharedState->lj.p30->params),
                          mach::vectorToDouble(sharedState->lj.p30->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(FastJack, int(sharedState->lj.pinj->params.size()),
                          mach::vectorToChar(sharedState->lj.pinj->params),
                          mach::vectorToDouble(sharedState->lj.pinj->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(FastJack, int(sharedState->lj.lc->params.size()),
                          mach::vectorToChar(sharedState->lj.lc->params),
                          mach::vectorToDouble(sharedState->lj.lc->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(FastJack, int(sharedState->lj.inj1->params.size()),
                          mach::vectorToChar(sharedState->lj.inj1->params),
                          mach::vectorToDouble(sharedState->lj.inj1->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(FastJack, int(sharedState->lj.inj2->params.size()),
                          mach::vectorToChar(sharedState->lj.inj2->params),
                          mach::vectorToDouble(sharedState->lj.inj2->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
    err = LJM_eWriteNames(FastJack, int(sharedState->lj.ign->params.size()),
                          mach::vectorToChar(sharedState->lj.ign->params),
                          mach::vectorToDouble(sharedState->lj.ign->settings),
                          &errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");
  } catch (...) {
    std::cout << "LABJACK ISSUE" << std::endl;
  }
  mach::Server s(ioContext, test, sharedState, SlowJack);
  threads.emplace_back([&ioContext] { ioContext.run(); });
  threads.emplace_back(run_signal, sharedState);
  //  threads.emplace_back(waitPause, SlowJack);
  err = LJM_eWriteName(SlowJack, "STREAM_TRIGGER_INDEX", 0);
  err = LJM_eWriteName(FastJack, "STREAM_TRIGGER_INDEX", 0);
  err = LJM_eWriteName(SlowJack, "AIN7_RESOLUTION_INDEX", 0);
  err = LJM_eWriteName(SlowJack, "AIN7_RANGE", 0.1);
  err = LJM_eWriteName(SlowJack, "AIN9_RESOLUTION_INDEX", 0);
  err = LJM_eWriteName(SlowJack, "AIN9_RANGE", 0.1);
  err = LJM_eStreamStart(SlowJack, spr, numSlowJack, scanListC, scanRate);
  ErrorCheck(err, "LJM_eStreamStart");
  err = LJM_eStreamStart(FastJack, spr, numFastJack, scanListCFast, scanRateFast);
  std::cout<<"my list"<<std::endl;
  ErrorCheck(err, "LJM_eStreamStart");
  while (!token.stop_requested()) {
    int devicebacklog;
    int ljmbacklog;
    auto *slowData = new double[10];
    auto *fastData = new double[8];
    mach::udouble ljbits{};
    LJM_eReadName(SlowJack, "DIO_STATE", &ljbits.d);
    std::bitset<sizeof(double) * 8> btest(ljbits.u);
    auto bits = btest.to_string();
    auto vals = bits.substr(18, 16);
    std::reverse(vals.begin(), vals.end());
    auto state = sharedState->getValves();
    for (size_t i = 0; i < vals.size(); i++) {
      std::string k = "DIO" + std::to_string(i);
      char v = vals.at(i);
      int nb = v - '0';
      if (mach::vljf.at(k).find('_') != std::string::npos) {
        state[mach::vljf.at(k)] = nb;
      } else {
        state[mach::vljf.at(k)] = !nb;
      }
    }
    double t1K, t2K, inj1K, inj2K, ignK = 0;
    err = LJM_eStreamRead(SlowJack, slowData, &devicebacklog, &ljmbacklog);
    err = LJM_eStreamRead(FastJack, fastData, &devicebacklog, &ljmbacklog);
    double slowTempK = (slowData[2] * -92.6 + 467.6);
    double fastTempK = (fastData[7] * -92.6 + 467.6);
    double fastTempK2 = (fastData[7] * -92.6 + 467.6);
    double fastTempK3 = (fastData[7] * -92.6 + 467.6);
    err = LJM_TCVoltsToTemp(6004, slowData[0], slowTempK, &t1K);
    err = LJM_TCVoltsToTemp(6004, slowData[1], slowTempK, &t2K);
    err = LJM_TCVoltsToTemp(6004, fastData[4], fastTempK, &inj1K);
    err = LJM_TCVoltsToTemp(6004, fastData[5], fastTempK2, &inj2K);
    err = LJM_TCVoltsToTemp(6004, fastData[6], fastTempK3, &ignK);
    sharedState->lj.t1val = (t1K - 273);
    sharedState->lj.t2val = (t2K - 273);
    sharedState->lj.inj1val = (inj1K - 273);
    sharedState->lj.inj2val = (inj2K - 273);
    sharedState->lj.ignval = (ignK - 273);
    sharedState->lj.p31val = (slowData[3] * 300);
    sharedState->lj.p21val = (slowData[5] * 300);
    sharedState->lj.p22val = (slowData[9] * 300);
    sharedState->lj.p32val = (slowData[6] * 300);
    sharedState->lj.p10val = (slowData[7] * 300);
    sharedState->lj.p20val = (fastData[0] * 300);
    sharedState->lj.p30val = (fastData[1] * 300);
    sharedState->lj.pinjval = (fastData[2] * 300);
    sharedState->lj.lcellval = (fastData[3] * 200);
    //      std::cout << "t2val:" << sharedState->lj.t2val << " ";
    //      std::cout << "p10val:" << sharedState->lj.p10val << "\n";
    //      std::cout << "p21val:" << sharedState->lj.p21val << " ";
    //      std::cout << "p31val:" << sharedState->lj.p31val << "\n";
    //      std::cout << "p22val:" << sharedState->lj.p22val << " ";
    //      std::cout << "p32val:" << sharedState->lj.p32val << "\n";
    // err = LJM_eReadName(SlowJack, sharedState->lj.p10->name.c_str(),
    //                      &sharedState->lj.p10val);
    //  err = LJM_eReadName(SlowJack, sharedState->lj.p21->name.c_str(),
    //                      &sharedState->lj.p21val);
    //  err = LJM_eReadName(SlowJack, sharedState->lj.p31->name.c_str(),
    //                      &sharedState->lj.p31val);
    //        err = LJM_eReadName(SlowJack, sharedState->lj.t2->name.c_str(),
    //         &sharedState->lj.t2val);
    // sharedState->lj.p10val *= 300;
    //  sharedState->lj.p21val *= 300;
    //  sharedState->lj.p31val *= 300;
    sharedState->setValves(state);
    s.emit(sharedState);
  }
  // std::this_thread::sleep_for(10s);
  s.stop();
  test.request_stop();
  for (auto &thread : threads) {
    thread.request_stop();
    thread.join();
  }
  return 0;
}
