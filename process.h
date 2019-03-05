#pragma once
#include "thread.h"

using namespace std;

class Process{
public:
    enum Type {SYSTEM, INTERACTIVE, NORMAL, BATCH};
    vector<string> typeStrings = {"SYSTEM", "INTERACTIVE", "NORMAL", "BATCH"};
    Type ptype;
    int pid;
    vector<Thread*> threads;

    Process(int pid, Type ptype){
        this->pid = pid;
        this->ptype = ptype;
    }

};
