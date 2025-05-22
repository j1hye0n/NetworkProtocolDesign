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
#define RSSI_LIMIT                  0 // rssi : -30~-100이라서 실제 기준치는 이 사이값으로 잡아야할 것 같긴한데, 지금은 신호가 들어오는지 확인해야 하니까 0으로 둘게

//Cell(Base Station) ID
static uint8_t C_ID[3] = {145, 208, 89};
static uint8_t my_cell_id = 0;
static int j = 0;
static int16_t rssi[100];
static int16_t max_rssi;    // array del
static uint8_t id[100];

//state variables
static uint8_t main_state = L3STATE_IDLE; //rotocol state
static uint8_t prev_state = main_state;

//SDU (input)
static uint8_t* originalWord[1030]; //우리가 보내는 메세지
static uint8_t wordLen = 0;

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
            int i=0;
            
            // RSSI timer로 일정시간동안 들어온 신호들의 세기를 비교해서 고르는 코드
            if (!L3_timer_getTimerStatus_R())
            {
                L3_timer_startTimer_R(); 
            }
            
            while (1) 
            {
               if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
                {
                    id[i] = L3_LLI_getSrcId();
                    if (id[i] == C_ID[0] || id[i] == C_ID[1] || id[i] == C_ID[2] ){ //condition 1
                        int16_t b_rssi = (L3_LLI_getRssi());
                        pc.printf("Id : %i rssi : %u\n",id[i], b_rssi); //출력 test
                        if (b_rssi >= RSSI_LIMIT){ //condition 2
                            rssi[i] = b_rssi;
                            i++;
                        }
                    }
                    
                    L3_event_clearEventFlag(L3_event_msgRcvd);
                }
                
                if (L3_event_checkEventFlag(L3_event_arqTimeout))
                {
                    L3_event_clearEventFlag(L3_event_arqTimeout);
                    break;
                }
            }

            if (i == 0 || rssi[0] == 0) // 여기가 영원히 반복됨.. 왜일까..?
            {
                pc.printf("There is no signal.\n\r");
            }
            else
            {
                for (j=0; j<=i ; j++)   // rssi가 가장 큰 신호 id[j]구하기 condition 4
                {
                    if (rssi[j] >= max_rssi)
                    {
                        max_rssi = rssi[j];
                    }
                }
                myDestId = id[j];
                L3_event_setEventFlag(L3_event_dataToSend);
            }
            
            if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent
            {
                //PDU 생성 "REQUEST"
                //msg header setting
                strcpy((char*) originalWord, "REQUEST\n\r");
                strcpy((char*) sdu, (char*) originalWord);
                L3_LLI_dataReqFunc(sdu, 200, myDestId);

                std::memset(rssi, 0, sizeof(rssi));     // rssi값 전부 초기화

                main_state = L3STATE_ACK;
                    
                L3_event_clearEventFlag(L3_event_dataToSend);
            }

            break;
        }

        case L3STATE_ACK:{

            if (!L3_timer_getTimerStatus_A())
            {
                L3_timer_startTimer_A(); // ACCEPT 기다리는 타이머 실행
            }
            
            if(L3_event_checkEventFlag(L3_event_msgRcvd)) // ACCEPT 수신하면
            {
                //Retrieving data info.
                uint8_t* dataPtr = L3_LLI_getMsgPtr();
                if (strcmp((char*) dataPtr, "ACCEPT\n\r") == 0)
                {
                    // 최근 선택한 기지국 ID 저장
                    my_cell_id = myDestId;
                    
                    // 타이머 중지
                    L3_timer_stopTimer_A(); // 타이머 중지
                    main_state = L3STATE_LND;
                }
                else if (L3_event_checkEventFlag(L3_event_arqTimeout)) // 타이머 터지면 IDLE 상태로 감
                {
                    main_state = L3STATE_IDLE;
                    L3_event_clearEventFlag(L3_event_arqTimeout);
                }
            }
            
            break;
        }
        
        case L3STATE_LND:{

            if (!L3_timer_getTimerStatus())
            {
                L3_timer_startTimer();
            }
            
            if (L3_event_checkEventFlag(L3_event_msgRcvd)) //PDU 수신 Event 1
            {
                uint8_t id_L = L3_LLI_getSrcId();
                if (id_L == my_cell_id){ // condition 3
                    max_rssi = (L3_LLI_getRssi()); // rssi 절댓값 취함
                    if(max_rssi >= RSSI_LIMIT) // condition 2
                    {
                        L3_timer_stopTimer(); // timerStatus = 2, 타이머 멈춤
                        L3_timer_startTimer(); // 타이머 재시작
                    }
                }
                else if(id_L != my_cell_id) // not condition 3
                {
                    if (id_L == C_ID[0] || id_L == C_ID[1] || id_L == C_ID[2]) //condition 1
                    {
                        int16_t rssi_L = (L3_LLI_getRssi()); // rssi 절댓값 취함
                        pc.printf("%i",rssi_L);
                        
                        if (rssi_L <= max_rssi) //condition 4
                        {
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
                strcpy((char*) sdu, (char*) originalWord);
                L3_LLI_dataReqFunc(sdu, 200, myDestId);

                main_state = L3STATE_ACK;
                
                L3_event_clearEventFlag(L3_event_dataToSend);
            }

            if (L3_event_checkEventFlag(L3_event_arqTimeout))
            {
                main_state = L3STATE_IDLE;
                
                L3_event_clearEventFlag(L3_event_arqTimeout);
            }
            
            break;
        }

        default :
            break;
    }
}
