#include "mbed.h"
#include "L3_FSMevent.h"
#include "protocol_parameters.h"


//ARQ retransmission timer
static Timeout timer;                       
static uint8_t timerStatus = 0;


//timer event : ARQ timeout
void L3_timer_timeoutHandler(void) 
{
    timerStatus = 0;
    L3_event_setEventFlag(L3_event_arqTimeout);
}

//timer related functions ---------------------------
void L3_timer_startTimer()
{
    uint8_t waitTime = 5;//L2_ARQ_MINWAITTIME + rand()%(L2_ARQ_MAXWAITTIME-L2_ARQ_MINWAITTIME); //timer length
    timer.attach(L3_timer_timeoutHandler, waitTime);
    timerStatus = 1;
}

void L3_timer_stopTimer()
{
    timer.detach();
    timerStatus = 0; //timeout 돼서 timerStatus가 자동으로 0이 되는 것과 구분하기 위함
}

uint8_t L3_timer_getTimerStatus()
{
    return timerStatus;
}

