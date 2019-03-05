#pragma once
#include "process.h"
#include "burst.h"

using namespace std;

//forward declare
class Process;

class Thread{
public:
    enum State {NEW, READY, RUNNING, BLOCKED, EXIT};
    vector<string> typeStrings = {"NEW", "READY", "RUNNING", "BLOCKED", "EXIT"};
    State state;
    int arrivalTime;
    int numBursts;
    int threadNum;
    Process* process;
    vector<Burst*> bursts;

    Thread(int arrivalTime, int numBursts, int threadNum, Process* process, State state){
        this->arrivalTime = arrivalTime;
        this->numBursts = numBursts;
        this->threadNum = threadNum;
        this->process = process;
        this->state = state;
    }
};
