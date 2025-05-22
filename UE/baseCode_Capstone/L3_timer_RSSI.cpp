#include "mbed.h"
#include "L3_FSMevent.h"
#include "protocol_parameters.h"


//ARQ retransmission timer
static Timeout timer_R;                       
static uint8_t timerStatus_R = 0;

//timer event : ARQ timeout
void L3_timer_timeoutHandler_R(void) 
{
    timerStatus_R = 0;
    L3_event_setEventFlag(L3_event_arqTimeout);
}

//timer related functions ---------------------------
void L3_timer_startTimer_R()
{
    uint8_t waitTime_R = 5;//L2_ARQ_MINWAITTIME + rand()%(L2_ARQ_MAXWAITTIME-L2_ARQ_MINWAITTIME); //timer length
    timer_R.attach(L3_timer_timeoutHandler_R, waitTime_R);
    timerStatus_R = 1;
}

void L3_timer_stopTimer_R()
{
    timer_R.detach();
    timerStatus_R = 0; //timeout 돼서 timerStatus가 자동으로 0이 되는 것과 구분하기 위함
}

uint8_t L3_timer_getTimerStatus_R()
{
    return timerStatus_R;
}