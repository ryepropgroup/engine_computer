#include "LJM_Utilities.h"
#include "LabJackM.h"
#include "helpers.h"
#include <array>
#include <bitset>
#include <boost/asio.hpp>
#include <chrono>
#include <fstream>
#include <iostream>

using namespace std::chrono_literals;
namespace ba = boost::asio;

std::stop_source test;
std::stop_token token = test.get_token();
int handle;
int spr{1};
int numaddr{7};
// AIN7, AIN14, AIN2, AIN4, AIN6, AIN11, AIN3
const std::array<int, 7> scanList{14, 28, 4, 8, 12, 22, 6};
auto scanListC = scanList.data();
double *scanRate = new double{10};
std::mutex writem;
std::mutex supersecure;
std::queue<std::string> vqueue{};
std::condition_variable suspend_write;
std::condition_variable vqueuecheck;
std::atomic<bool> enabled = true;
std::vector<std::jthread> threads{};

void valveActFunc(int handle){
  while(!token.stop_requested()){
    std::unique_lock<std::mutex> lock(supersecure);
    vqueuecheck.wait(lock, []{return !vqueue.empty();});
    std::string val = vqueue.front();
    vqueue.pop();
    lock.unlock();
    mach::dispatchValve(val, handle);
  }
}
void handlesigint(const int signal_num) {
  test.request_stop();
  std::cout<<"Killing Threads"<<std::endl;
  for(auto& thread: threads){
    thread.request_stop();
    thread.join();
  }
  std::cout<<"Disconnecting LabJack..."<<"\n";
  int err = LJM_eStreamStop(handle);
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
  std::ofstream file(utns.str()+".csv");
  if (!file.is_open()) {
    std::cerr << "file not available" << std::endl;
    return;
  }
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
       << "t2"
       << "\n";

  while (!token.stop_requested()) {
    std::unique_lock<std::mutex> lock(writem);
    suspend_write.wait(lock, [] { return bool(enabled); });
    mach::Timestamp now = mach::now();
    if ((now - old_time) > 0) {
      file << now << "," << st->lj.p10val << "," << st->lj.p21val << ","
           << st->lj.p31val << ","<< st->lj.p22val << ","<< st->lj.p32val << ","<< st->lj.t2val <<"\n";
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
    //    err = LJM_Open(LJM_dtANY, LJM_ctANY, "ANY", &handle);
    //    ErrorCheck(err, "LJM_Open");
    //    err = LJM_eWriteNames(handle, int(sharedState->lj.p10->params.size()),
    //                          mach::vectorToChar(sharedState->lj.p10->params),
    //                          mach::vectorToDouble(sharedState->lj.p10->settings),
    //                          &errorAddress);
    //    ErrorCheck(err, "LJM_eWriteNames");
    //    err = LJM_eWriteNames(handle, int(sharedState->lj.p21->params.size()),
    //                          mach::vectorToChar(sharedState->lj.p21->params),
    //                          mach::vectorToDouble(sharedState->lj.p21->settings),
    //                          &errorAddress);
    //    ErrorCheck(err, "LJM_eWriteNames");
    //    err = LJM_eWriteNames(handle, int(sharedState->lj.p31->params.size()),
    //                          mach::vectorToChar(sharedState->lj.p31->params),
    //                          mach::vectorToDouble(sharedState->lj.p31->settings),
    //                          &errorAddress);
    //    ErrorCheck(err, "LJM_eWriteNames");
    //    err = LJM_eWriteNames(handle, int(sharedState->lj.p22->params.size()),
    //                          mach::vectorToChar(sharedState->lj.p22->params),
    //                          mach::vectorToDouble(sharedState->lj.p22->settings),
    //                          &errorAddress);
    //    ErrorCheck(err, "LJM_eWriteNames");
    //    err = LJM_eWriteNames(handle, int(sharedState->lj.p32->params.size()),
    //                          mach::vectorToChar(sharedState->lj.p32->params),
    //                          mach::vectorToDouble(sharedState->lj.p32->settings),
    //                          &errorAddress);
    //    ErrorCheck(err, "LJM_eWriteNames");
    // err = LJM_eWriteNames(handle, int(sharedState->lj.t2->params.size()),
    // mach::vectorToChar(sharedState->lj.t2->params),
    // mach::vectorToDouble(sharedState->lj.t2->settings), &errorAddress);
    // ErrorCheck(err, "LJM_eWriteNames");
  } catch (...) {
    std::cout << "LABJACK ISSUE" << std::endl;
  }
  mach::Server s(ioContext, test, sharedState, handle);
  threads.emplace_back([&ioContext] { ioContext.run(); });
  threads.emplace_back(run_signal, sharedState));
  threads.emplace_back(valveActFunc, handle);
  //  err = LJM_eWriteName(handle, "STREAM_TRIGGER_INDEX", 0);
  //  err = LJM_eWriteName(handle, "AIN0_RESOLUTION_INDEX", 0);
  //  err = LJM_eWriteName(handle, "AIN0_RANGE", 0.1);
  //  err = LJM_eStreamStart(handle, spr, numaddr, scanListC, scanRate);
  //  ErrorCheck(err, "LJM_eStreamStart");
  //  while (!token.stop_requested()) {
  //    int devicebacklog;
  //    int ljmbacklog;
  //    auto *aData = new double[7];
  //    mach::udouble ljbits{};
  //    LJM_eReadName(handle, "DIO_STATE", &ljbits.d);
  //    std::bitset<sizeof(double) * 8> btest(ljbits.u);
  //    auto bits = btest.to_string();
  //    auto vals = bits.substr(18, 16);
  //    std::reverse(vals.begin(), vals.end());
  //    auto state = sharedState->getValves();
  //    for (size_t i = 0; i < vals.size(); i++) {
  //      std::string k = "DIO" + std::to_string(i);
  //      char v = vals.at(i);
  //      int nb = v - '0';
  //      if (mach::vljf.at(k).find('_') != std::string::npos) {
  //        state[mach::vljf.at(k)] = nb;
  //      } else {
  //        state[mach::vljf.at(k)] = !nb;
  //      }
  //    }
  //    double t1K = 0;
  //    err = LJM_eStreamRead(handle, aData, &devicebacklog, &ljmbacklog);
  //    ErrorCheck(err, "LJM_eStreamRead");
  //    double dTempK = (aData[1] * -92.6 + 467.6);
  //    err = LJM_TCVoltsToTemp(6004, aData[0], dTempK, &t1K);
  //    sharedState->lj.t2val = (t1K);
  //    sharedState->lj.p10val = (aData[4] * 300);
  //    sharedState->lj.p21val = (aData[3] * 300);
  //    sharedState->lj.p31val = (aData[2] * 300);
  //    sharedState->lj.p22val = (aData[5] * 300);
  //    sharedState->lj.p32val = (aData[6] * 300);
  //    std::cout << "t2val:" << sharedState->lj.t2val << " ";
  //    std::cout << "p10val:" << sharedState->lj.p10val << "\n";
  //    std::cout << "p21val:" << sharedState->lj.p21val << " ";
  //    std::cout << "p31val:" << sharedState->lj.p31val << "\n";
  //    std::cout << "p22val:" << sharedState->lj.p22val << " ";
  //    std::cout << "p32val:" << sharedState->lj.p32val << "\n";
  //    // err = LJM_eReadName(handle, sharedState->lj.p10->name.c_str(),
  //    //                      &sharedState->lj.p10val);
  //    //  err = LJM_eReadName(handle, sharedState->lj.p21->name.c_str(),
  //    //                      &sharedState->lj.p21val);
  //    //  err = LJM_eReadName(handle, sharedState->lj.p31->name.c_str(),
  //    //                      &sharedState->lj.p31val);
  //    //        err = LJM_eReadName(handle, sharedState->lj.t2->name.c_str(),
  //    //         &sharedState->lj.t2val);
  //    // sharedState->lj.p10val *= 300;
  //    //  sharedState->lj.p21val *= 300;
  //    //  sharedState->lj.p31val *= 300;
  //    sharedState->setValves(state);
  //    s.emit(sharedState);
  //  }
  std::this_thread::sleep_for(10s);
  s.stop();
  test.request_stop();
  for(auto& thread: threads){
    thread.request_stop();
    thread.join();
  }
  return 0;
}
