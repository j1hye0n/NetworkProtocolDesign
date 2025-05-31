#include "mbed.h"
#include "string.h"
#include "L2_FSMmain.h"
#include "L3_FSMmain.h"


//serial port interface
Serial pc(USBTX, USBRX);

//GLOBAL variables (DO NOT TOUCH!) ------------------------------------------

//source/destination ID
uint8_t input_thisId=89; // 기지국 각각의 ID 기입
uint8_t input_destId=255; // broadcast

//FSM operation implementation ------------------------------------------------
int main(void){

    pc.printf("endnode : %i, dest : %i\n", input_thisId, input_destId);

    //initialize lower layer stacks
    L2_initFSM(input_thisId);
    L3_initFSM(input_destId);
    

    while(1)
    {
        L2_FSMrun();
        L3_FSMrun();
    }
}
