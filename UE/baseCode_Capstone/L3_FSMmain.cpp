#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"
#include <iostream>
#include <vector>
#include <string>


//FSM state -------------------------------------------------
#define L3STATE_IDLE                0
#define L3STATE_LND                 1
#define L3STATE_ACK                 2

//Cell(Base Station) ID
static uint8_t C_ID[3] = {145, 208, 089};

//state variables
static uint8_t main_state = L3STATE_IDLE; //protocol state
static uint8_t prev_state = main_state;

//SDU (input)
static uint8_t originalWord[1030];
static uint8_t wordLen=0;
static uint8_t sdu[1030];

//serial port interface
static Serial pc(USBTX, USBRX);
static uint8_t myDestId;

//application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)    // 이거 keyboard input 필요한가? 
{
    char c = pc.getc();
    if (!L3_event_checkEventFlag(L3_event_dataToSend))
    {
        // if (c == '\n' || c == '\r')
        // {
            // originalWord[wordLen++] = '\0';
            // L3_event_setEventFlag(L3_event_dataToSend);
            // debug_if(DBGMSG_L3,"word is ready! ::: %s\n", originalWord);
        // }
        // else
        // {
            // originalWord[wordLen++] = c;
            // if (wordLen >= L3_MAXDATASIZE-1)
            // {
                // originalWord[wordLen++] = '\0';
                // L3_event_setEventFlag(L3_event_dataToSend);
                // pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
            // }
        // }
    }
}



void L3_initFSM(uint8_t destId)
{

    myDestId = destId;
    //initialize service layer
    // pc.attach(&L3service_processInputWord, Serial::RxIrq);

    // pc.printf("Give a word to send : ");
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
            
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {

                // ========================
                // class로 입력 받는거 
                // ========================

                class Device {
                    private:
                        uint8_t id;
                        uint8_t rssi;

                    public:
                        Device(uint8_t rssi, uint8_t id)
                            : id(id), rssi(rssi) {}

                        uint8_t L3_LLI_getRssi() const {
                            return rssi;
                        }

                        uint8_t L3_LLI_getSrcId() const {

                            return id;
                        }
                };

                
                // 
                std::signals<Device> devices;

                std::cout   << "가장 높은 RSSI를 가진 장치:\n";
                std::cout   << "ID: " << maxDevice->L3_LLI_getId()
                            << ", RSSI: " << maxDevice->L3_LLI_getRssi()

                //Retrieving data info.
                // uint8_t* dataPtr = L3_LLI_getMsgPtr();
                // uint8_t size = L3_LLI_getSize();
                // uint8_t rssi = L3_LLI_getRssi();    // rssi 필요해서 넣었어
                
                L3_LLI_dataReqFunc(sdu, 200, myDestId);

                // debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
                            // dataPtr, size);
                
                // pc.printf("Give a word to send : ");
                
                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
            {
                //msg header setting
                strcpy((char*)sdu, (char*)originalWord);
                debug("[L3] msg length : %i\n", wordLen);
                L3_LLI_dataReqFunc(sdu, wordLen, myDestId);

                debug_if(DBGMSG_L3, "[L3] sending msg....\n");
                wordLen = 0;

                pc.printf("Give a word to send : ");

                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            break;

        case L3STATE_ACK:
            
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) // PDU - ACCEPT 수신, 숭
            {
                //최근 선택한 기지국 ID 저장, Timer 시작
                L3_timer_startTimer(); //이거 Timer 2개 나눠야 하지 않아?
                main_state = L3STATE_LND;
            }
            
            else if() //Data CNF negative 수신 Event2
            {
                main_state = L3STATE_IDLE;
            }

            break;

        case L3STATE_LND:

            if (!L3_timer_getTimerStatus()) // timerstatus = 0 (즉, 타이머 작동 X)
            {
                main_state = L3STATE_IDLE; //IDLE state로 돌아감
            }

            else if (L3_event_checkEventFlag(L3_event_msgRcvd)) //PDU 수신 Event1
            {
                if() //not C3 & C4
                {
                    //PDU 생성 "REQUEST"
                    main_state = L3STATE_ACK; 
                }

                else if () //C2 & C3
                {
                    L3_timer_stopTimer(); // timerStatus = 2, 타이머 멈춤
                    L3_timer_startTimer(); // 타이머 재시작
                    main_state = L3STATE_LND; // 어차피 계속 LND state일 텐데 필요한 코드인가..? 누가 아는 사람이 해결해주길 바람
                }
            }
            
            break;

        default :
            break;
    }
}