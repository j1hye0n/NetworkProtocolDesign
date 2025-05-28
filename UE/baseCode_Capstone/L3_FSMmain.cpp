#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_timer_ACCEPT.h"
#include "L3_timer_RSSI.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>


//FSM state -------------------------------------------------
#define L3STATE_IDLE                0
#define L3STATE_LND                 1
#define L3STATE_ACK                 2
#define RSSI_LIMIT                  -50

//Cell(Base Station) ID
static uint8_t C_ID[3] = {145, 208, 89};

//RSSI Comparison
static uint8_t my_cell_id = 0;
static int max_i = -1;
static int16_t rssi[100];
static int16_t max_rssi = -100;
static uint8_t id[100];
static int i = 0;
static int16_t b_rssi;
static int16_t rssi_L;


//state variables
static uint8_t main_state = L3STATE_IDLE; //rotocol state
static uint8_t prev_state = main_state;

//SDU (input)
static char originalWord[1030]; //우리가 보내는 메세지
// static uint8_t wordLen = 0;

static uint8_t sdu[1030];

//serial port interface
static Serial pc(USBTX, USBRX);
static uint8_t myDestId;

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
        case L3STATE_IDLE: {//IDLE state description            
            // RSSI timer로 일정시간(5초) 동안 들어온 신호들의 세기를 비교해서 고르는 코드
            if (L3_event_checkEventFlag(L3_event_arqTimeout))
            {
                pc.printf("IDLE is TIME OUT!\n\r");
                for (int j=0; j<i ; j++)   // rssi가 가장 큰 신호 id[j]구하기 condition 4
                {
                    if (rssi[j] > max_rssi)
                    {
                        max_rssi = rssi[j];
                        max_i = j;
                    }
                }
                
                if (max_i == -1)
                {
                    pc.printf("There is no signal.\n\r");
                }
                else
                {
                    myDestId = id[max_i];
                    L3_event_setEventFlag(L3_event_dataToSend);
                }

                L3_event_clearEventFlag(L3_event_arqTimeout);
            }
            else if (!L3_timer_getTimerStatus_R())
            {
                max_i = -1;
                L3_timer_startTimer_R(); 
                pc.printf("IDLE timer start\n\r");
            }

            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
            {
                id[i] = L3_LLI_getSrcId();
                if (id[i] == C_ID[0] || id[i] == C_ID[1] || id[i] == C_ID[2] ) //condition 1
                { 
                    b_rssi = L3_LLI_getRssi();

                    pc.printf("Id : %i rssi : %i\n\r",id[i], b_rssi); //출력 test

                    // pc.printf("RSSI_LIMIT : %i, b_rssi : %i, %i\n\r", RSSI_LIMIT, b_rssi, b_rssi > RSSI_LIMIT);

                    if (b_rssi > RSSI_LIMIT) //condition 2
                    { 
                        rssi[i] = b_rssi;
                        i++;
                        pc.printf("i : %d\n\r",i);
                    }
                }
                else 
                {
                    pc.printf("ID is not our basestation. ID : %i\n\r",id[i]); // 디버깅용
                }

                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent
            {
                //PDU 생성 "REQUEST"
                //msg header setting
                strcpy((char*) originalWord, "REQUEST\n\r");
                strcpy((char*) sdu, (char*) originalWord);

                uint8_t len = strlen(originalWord);

                L3_LLI_dataReqFunc(sdu, len+1, myDestId);

                pc.printf("Tried Request to %i.\n\r", myDestId);

                i = 0;
                std::memset(id, 0, sizeof(id));     // id값 전부 초기화
                std::memset(rssi, 0, sizeof(rssi));     // rssi값 전부 초기화

                L3_timer_stopTimer_R();
                main_state = L3STATE_ACK;
                    
                L3_event_clearEventFlag(L3_event_dataToSend);
        
            }

            break;
        }
    
        case L3STATE_ACK:{                       
            if (L3_event_checkEventFlag(L3_event_arqTimeout)) // 타이머 터지면 IDLE 상태로 감
            {
                pc.printf("Base don't ACCEPT\n\r");

                main_state = L3STATE_IDLE;
                L3_event_clearEventFlag(L3_event_arqTimeout);
            }
            else if(!L3_timer_getTimerStatus_A())
            {
                L3_timer_startTimer_A(); // ACCEPT 기다리는 타이머 실행(10초)
                pc.printf("ACK timer start\n\r");
            }

            if (L3_event_checkEventFlag(L3_event_msgRcvd)) // ACCEPT 수신하면
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                if (strcmp((char*) dataPtr, "ACCEPT\n\r") == 0)
                {
                    pc.printf("Base sent ACCEPT\n\r");
                    
                    // 타이머 중지
                    L3_timer_stopTimer_A(); // 타이머 중지

                    // 최근 선택한 기지국 ID 저장
                    my_cell_id = myDestId;
                    pc.printf("Connect with Base %u\n\r", my_cell_id);

                    main_state = L3STATE_LND;
                }
            }
            break;
        }
        
        case L3STATE_LND:{
            if (L3_event_checkEventFlag(L3_event_arqTimeout))
            {
                pc.printf("Base signal is unstable.\n\r");

                L3_timer_stopTimer();
                main_state = L3STATE_IDLE;
                
                L3_event_clearEventFlag(L3_event_arqTimeout);
            }
            else if(!L3_timer_getTimerStatus())
            {
                pc.printf("Connected!\n\r");
                L3_timer_startTimer();
            }

            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //PDU 수신 Event 1
            {
                uint8_t id_L = L3_LLI_getSrcId();
                if (id_L == my_cell_id){ // condition 3
                    max_rssi = L3_LLI_getRssi(); 

                    if(max_rssi > RSSI_LIMIT) // condition 2
                    {
                        L3_timer_stopTimer(); // 타이머 멈춤
                        L3_timer_startTimer(); // 타이머 재시작
                    }
                }
                else if(id_L != my_cell_id) // not condition 3
                {
                    if (id_L == C_ID[0] || id_L == C_ID[1] || id_L == C_ID[2]) //condition 1
                    {      
                        rssi_L = L3_LLI_getRssi();                   
                        if (rssi_L > max_rssi) //condition 4
                        { 
                            pc.printf("New Base is detected! %i, rssi : %i\n\r", id_L, rssi_L);

                            //PDU 생성 "REQUEST"
                            strcpy((char*) originalWord, "REQUEST\n\r");
                            myDestId = id_L;
                            L3_event_setEventFlag(L3_event_dataToSend);
                        }
                    }
                }

                L3_event_clearEventFlag(L3_event_msgRcvd);
            }
            else if (L3_event_checkEventFlag(L3_event_dataToSend))
            {
                uint8_t len = strlen(originalWord);

                strcpy((char*) sdu, (char*) originalWord);
                L3_LLI_dataReqFunc(sdu, len+1, myDestId);
                pc.printf("Send REQUEST to New Base.\n\r");

                L3_timer_stopTimer();
                main_state = L3STATE_ACK;
                
                L3_event_clearEventFlag(L3_event_dataToSend);
            }
            
            break;
        }

        default :
            break;
    }
}
