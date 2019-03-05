#pragma once
#include "thread.h"

using namespace std;

class Event{
public:
    enum Type {
        THREAD_ARRIVED, THREAD_DISPATCH_COMPLETED, PROCESS_DISPATCH_COMPLETED,
        CPU_BURST_COMPLETED, IO_BURST_COMPLETED, THREAD_COMPLETED,
        THREAD_PREEMPTED, DISPATCHER_INVOKED
    };
    vector<string> typeStrings = {
        "THREAD_ARRIVED", "THREAD_DISPATCH_COMPLETED", "PROCESS_DISPATCH_COMPLETED",
        "CPU_BURST_COMPLETED", "IO_BURST_COMPLETED", "THREAD_COMPLETED",
        "THREAD_PREEMPTED", "DISPATCHER_INVOKED"
    };
    Type type;
    int time;
    Thread* thread;

    Event(Type type, int time){
        this->type = type;
        this->time = time;
    }
};
