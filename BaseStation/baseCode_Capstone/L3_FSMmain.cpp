#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <sstream>

//FSM state -------------------------------------------------
#define L3STATE_IDLE                0


//state variables
static uint8_t main_state = L3STATE_IDLE; //protocol state
static uint8_t prev_state = main_state;

//SDU (input)
static uint8_t* originalWord[1030];
static uint8_t wordLen = 0;

static uint8_t sdu[1030];

//serial port interface
static Serial pc(USBTX, USBRX);
static uint8_t myDestId;
static uint8_t myL2Id;

// application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)
{
    // char c = pc.getc();
    if (!L3_event_checkEventFlag(L3_event_dataToSend))
    {
        // if (c == '\n' || c == '\r')
        // {
        //     originalWord[wordLen++] = '\0';
        //     L3_event_setEventFlag(L3_event_dataToSend);
        //     debug_if(DBGMSG_L3,"word is ready! ::: %s\n", originalWord);
        // }
        // else
        // {
        //     originalWord[wordLen++] = c;
        //     if (wordLen >= L3_MAXDATASIZE-1)
        //     {
        //         originalWord[wordLen++] = '\0';
        //         L3_event_setEventFlag(L3_event_dataToSend);
        //         pc.printf("\n max reached! word forced to beready :::: %s\n", originalWord);
        //     }
        // }
        strcpy((char*) originalWord, "Sending DATA\0");
        L3_event_setEventFlag(L3_event_dataToSend);
        pc.printf("");
    }
}



void L3_initFSM(uint8_t destId) //여기서 pdu 생성
{

    myDestId = destId;
    //initialize service layer
    //pc.attach(&L3service_processInputWord, Serial::RxIrq); 

    // //sdu = 기지국ID(CID) + PDU인지 알려주는 문구
    // std::stringstream ss;
    // ss << "Sending Data... ID: " << static_cast<int>(myL2Id);
    // std::string sdu = ss.str();
    // std::cout << "SDU: " << sdu << std::endl;

    // pc.printf("Give a word to send : "); msg 전달용 이라고 생각해서 주석 05/16
}

void L3_FSMrun(void)
{   
    if (prev_state != main_state)
    {
        debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    //FSM should be implemented here! ---->>>>
    switch (main_state)
    {
        case L3STATE_IDLE: //IDLE state description

            // pdu 송신(L2로 보냄) & timer돌리고, 초기화하는 로직을 구현 >>>
            L3_LLI_dataReqFunc(sdu, 7, myDestId); // pdu 송신(L2)

            debug_if(DBGMSG_L3, "[L3] sending msg....\n");

            L3_timer_startTimer(); // DATA 주기적으로 보내는 Timer

            
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens (event a)
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                uint8_t size = L3_LLI_getSize();
                uint8_t rssi = L3_LLI_getRssi();    //Rssi&ID variables are added date.05/13
                uint8_t srcId = L3_LLI_getSrcId();

                // debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                //             dataPtr, size);
                
                if (srcId == 221) // condition1
                {
                    if (strcmp((char*)dataPtr, "REQUEST") == 0) //condition2
                    {
                        pc.printf("Request from UE 221 to be connected.\n");
                        strcpy((char*) originalWord, "ACCEPT\0"); //action 2
                        pc.printf("Send ACCEPT to UE 221.\n");
                    }
                }
                else //not condition1
                {
                    pc.printf("Unknown signal is coming.\n");
                    // strcpy((char*) originalWord, "Sending DATA\0");
                }
                
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            //else if 안의 내용은 initFSM & case IDLE에 옮김
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input) 
            {
                //msg header setting
                strcpy((char*)sdu, (char*)originalWord);
                // debug("[L3] msg length : %i\n", wordLen);
                L3_LLI_dataReqFunc(sdu, 200, myDestId);
                
                //debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                //wordLen = 0;
                strcpy((char*) originalWord, "Sending DATA\0");
                
                // pc.printf("Give a word to send : ");
                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            break;

        default :
            break;
    }
}