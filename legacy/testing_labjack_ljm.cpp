#include <stdio.h>
#include <LabJackM.h>
#include <string>
#include "./libs/LJM_Utilities.h"
#include <chrono>
#include <thread>
#define NUM_FRAMES 5
#define NUM_FRAMES2 2
int main() {
    int err, handle;
    double value = 0;
    double value2 = 0;
    double value3 = 0;
    double value4 = 0;
    int errorAddress = INITIAL_ERR_ADDRESS;

 //   const char * NAME = {"AIN0_EF_READ_A"};
    const char * NAME2 = {"AIN0"};
    const char * NAME3 = {"AIN2"};
    const char * NAME4 = {"AIN4"};
//    const char* names[NUM_FRAMES]={"AIN0_EF_INDEX","AIN0_EF_CONFIG_A","AIN0_EF_CONFIG_B","AIN0_EF_CONFIG_D","AIN0_EF_CONFIG_E"};
    const char* names2[NUM_FRAMES2]={"AIN0_RANGE","AIN0_NEGATIVE_CH"};
    const char* names3[NUM_FRAMES2]={"AIN2_RANGE","AIN2_NEGATIVE_CH"};
    const char* names4[NUM_FRAMES2]={"AIN4_RANGE","AIN4_NEGATIVE_CH"};
  //  double aValues[NUM_FRAMES]={22,1,60052,1.0,0.0};
    double aValues2[NUM_FRAMES2]={0.1, 1.0};
    double aValues3[NUM_FRAMES2]={0.1, 3.0};
    double aValues4[NUM_FRAMES2]={0.1, 5.0};
    double max{-10000};

    // Open first found LabJack
    err = LJM_Open(LJM_dtANY, LJM_ctANY, "LJM_idANY", &handle);
    ErrorCheck(err, "LJM_Open");

    for(;;){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //err = LJM_eWriteNames(handle, NUM_FRAMES, names, aValues,&errorAddress);
    err = LJM_eWriteNames(handle, NUM_FRAMES2, names2, aValues2,&errorAddress);
    err = LJM_eWriteNames(handle, NUM_FRAMES2, names3, aValues3,&errorAddress);
    err = LJM_eWriteNames(handle, NUM_FRAMES2, names4, aValues4,&errorAddress);
    ErrorCheck(err, "LJM_eWriteNames");

    // Call LJM_eReadName to read the serial number from the LabJack.
    //err = LJM_eReadName(handle, NAME, &value);
    err = LJM_eReadName(handle, NAME2, &value2);
    err = LJM_eReadName(handle, NAME3, &value3);
    err = LJM_eReadName(handle, NAME4, &value4);
    ErrorCheck(err, "LJM_eReadName");

//   printf("eReadName result:\n");
//   printf("%s = %f\n", NAME, value);
     int intvalue2 = (25000*value2);
     int intvalue3 = (25000*value3);
     int intvalue4 = (25000*value4);
//     if(value2>max){
//         max = value2;
//    };
     printf("eReadName 2 result:\n");
     printf("    %s = %d\n", NAME2, intvalue2);
     printf("    %s = %d\n", NAME3, intvalue3);
     printf("    %s = %d\n", NAME4, intvalue4);
//     printf("max is %f\n", max);
    }

    // Close device handle
    err = LJM_Close(handle);
    ErrorCheck(err, "LJM_Close");

    return LJME_NOERROR;

}
