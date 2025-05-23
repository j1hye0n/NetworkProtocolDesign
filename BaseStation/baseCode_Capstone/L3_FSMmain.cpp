#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"


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
// static uint8_t myL2Id;

// application event handler : generating SDU from keyboard input
// static void L3service_processInputWord(void)
// {
//     // char c = pc.getc();
//     if (!L3_event_checkEventFlag(L3_event_dataToSend))
//     {
//         // if (c == '\n' || c == '\r')
//         // {
//         //     originalWord[wordLen++] = '\0';
//         //     L3_event_setEventFlag(L3_event_dataToSend);
//         //     debug_if(DBGMSG_L3,"word is ready! ::: %s\n", originalWord);
//         // }
//         // else
//         // {
//         //     originalWord[wordLen++] = c;
//         //     if (wordLen >= L3_MAXDATASIZE-1)
//         //     {
//         //         originalWord[wordLen++] = '\0';
//         //         L3_event_setEventFlag(L3_event_dataToSend);
//         //         pc.printf("\n max reached! word forced to beready :::: %s\n", originalWord);
//         //     }
//         // }
//         strcpy((char*) originalWord, "Sending DATA\0");
//         L3_event_setEventFlag(L3_event_dataToSend);
//         pc.printf("");
//     }
// }



void L3_initFSM(uint8_t destId)
{

    myDestId = destId;

    // pc.attach(&L3service_processInputWord,Serial::RxIrq);
    
    // //initialize service layer
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

            // 주기적 DATA 송신을 위한 timer 작동
            // 처음 PDU 셋팅
            if (!L3_timer_getTimerStatus())
            {
                strcpy((char*) originalWord, "Sending DATA\0");
                L3_event_setEventFlag(L3_event_dataToSend);
                L3_timer_startTimer();
                // pc.printf("sending data\n");
            }

            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens (event a)
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                // uint8_t size = L3_LLI_getSize();
                uint8_t rssi = L3_LLI_getRssi();    //Rssi&ID variables are added date.05/13
                uint8_t srcId = L3_LLI_getSrcId();

                // debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                //             dataPtr, size);
                
                if (srcId == 221) // condition1
                {
                    if (strcmp((char*) dataPtr, "REQUEST\n\r") == 0) //condition2
                    {
                        pc.printf("Request from UE 221 to be connected.\n\r");

                        strcpy((char*) originalWord, "ACCEPT\n\r"); //action 2
                        L3_event_setEventFlag(L3_event_dataToSend);
                        pc.printf("Tried to Send ACCEPT to UE 221.\n\r");
                    }
                }
                else //not condition1
                {
                    pc.printf("Unknown signal is coming.\n\r");
                }
                
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            //else if 안의 내용은 initFSM & case IDLE에 옮김
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent
            {
                //msg header setting
                strcpy((char*) sdu, (char*) originalWord);
                //L2에 data Request
                L3_LLI_dataReqFunc(sdu, 200, myDestId);
                // debug("[L3] msg length : %i\n", wordLen);
                // debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                //wordLen = 0;

                //PDU 기본값으로 셋팅
                strcpy((char*) originalWord, "Sending DATA\n\r");
                
                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            
            if (L3_event_checkEventFlag(L3_event_arqTimeout))
            {   
                // 일정 시간(Timer)마다 PDU(기지국 DATA) 재전송
                // L3_timer_stopTimer();

                // 타이머 재작동
                L3_timer_startTimer();

                L3_event_clearEventFlag(L3_event_arqTimeout);
            }
            break;

        default :
            break;
    }
}