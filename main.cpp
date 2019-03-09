#include <queue>
#include <vector>
#include <iostream>
#include <fstream>
#include <regex>
#include <unordered_set>
#include <string>
#include "process.h"
#include "event.h"
#include "thread.h"
#include "burst.h"

using namespace std;

//comparator for event priority_queue: lower arrival time = higher priority
struct CompareEvents{
    bool operator()(Event* e1, Event* e2){
        return e1->time > e2->time;
    }
};
//comparator for thread priority_queue: lower arrival time = higher priority
struct CompareThreads{
    bool operator()(Thread* t1, Thread* t2){
        return t1->arrivalTime > t2->arrivalTime;
    }
};

//global vars
vector<Process*> processes;

//Event Queue
priority_queue<Event*, vector<Event*>, CompareEvents> eventQ;
//Flags that are passed by the user
vector<string> tokens;
//flags in set form
unordered_set<string> tokenSet;
//Thread Ready queue
priority_queue<Thread*, vector<Thread*>, CompareThreads> threadReadyQ;
priority_queue<Thread*, vector<Thread*>, CompareThreads> threadBlockedQ;
priority_queue<Thread*, vector<Thread*>, CompareThreads> threadRunningQ;
Thread* previousThread;

//These values will be used in various functions
map<Thread*, int> allThreadsAndArrivalTime;

int numProcesses, threadOverhead, processOverhead;
int clockTime = 0;

//performance metric variables
int numSystemThreads = 0;
int numInteractiveThreads = 0;
int numNormalThreads = 0;
int numBatchThreads = 0;

int totalSystemResponseTime = 0;
int totalInteractiveResponseTime = 0;
int totalNormalResponseTime = 0;
int totalBatchResponseTime = 0;

int totalSystemTurnaroundTime = 0;
int totalInteractiveTurnaroundTime = 0;
int totalNormalTurnaroundTime = 0;
int totalBatchTurnaroundTime = 0;

int totalElapsedTime = 0;
int totalServiceTime = 0;
int totalIOTime = 0;
int totalDispatchTime = 0;
int totalIdleTime = 0;



vector<string> tokenizer(int argc, char* argv[]){

    //no flags
    if(argc == 2) {
        vector<string> tokensTemp;
        tokensTemp.push_back(argv[1]);
        return tokensTemp;
    }
    else {
        regex oneDashFlag("^-[tvah]{1,4}$");
        regex per_thread("^(\\s)*--per_thread(\\s)*$");
        regex verbose("^(\\s)*--verbose(\\s)*$");
        regex algorithm("^(\\s)*--algorithm(\\s)*$");
        regex help("^(\\s)*--help(\\s)*$");

        //loop through the command line args
        for(int i = 1; i < argc-1; i++){
            if(regex_match(argv[i], oneDashFlag)){
                //loop through chars in the single dash flag
                for(int j = 1; argv[i][j] != '\0'; j++){
                    string s(1, argv[i][j]);
                    tokenSet.insert(s);
                }
            }
            else if(regex_match(argv[i], per_thread)) tokenSet.insert("t");
            else if(regex_match(argv[i], verbose)) tokenSet.insert("v");
            else if(regex_match(argv[i], algorithm)) tokenSet.insert("a");
            else if(regex_match(argv[i], help)) tokenSet.insert("h");
        }
        vector<string> tokensTemp(tokenSet.begin(), tokenSet.end());
        tokensTemp.push_back(argv[argc-1]);
        return tokensTemp;
    }
}

priority_queue<Event*, vector<Event*>, CompareEvents> parseFile(string fileName){

    priority_queue<Event*, vector<Event*>, CompareEvents> tempQ;

    ifstream fileStream(fileName);
    if (fileStream.is_open())
    {
        //first line = numProcesses. threadOverhead, processOverhead
        fileStream >> numProcesses >> threadOverhead >> processOverhead;
        //cout << numProcesses << threadOverhead << processOverhead << endl;

        //Get process information
        for(int i = 0; i < numProcesses; i++){
            int pid, pType, numThreads;
            fileStream >> pid >> pType >> numThreads;
            Process* process = new Process(pid, static_cast<Process::Type>(pType));
            processes.push_back(process);

            //Get thread information for each process
            for(int j = 0; j < numThreads; j++){
                int threadArrival, numBursts;
                fileStream >> threadArrival >> numBursts;
                Thread* thread = new Thread(threadArrival, numBursts, j, process, Thread::NEW);
                process->threads.push_back(thread);
                //Add THREAD_ARRIVED event to the priority_queue
                Event* event = new Event(Event::THREAD_ARRIVED, threadArrival);
                event->thread = thread;
                tempQ.push(event);

                //Get burst information for each thread
                for(int k = 0; k < numBursts; k++){
                    int cpuTime, ioTime;
                    fileStream >> cpuTime;
                    Burst* cpuBurst = new Burst(cpuTime, Burst::CPU);

                    if(k != numBursts-1){
                        fileStream >> ioTime;
                        Burst* ioBurst = new Burst(ioTime, Burst::IO);
                        thread->bursts.push_back(cpuBurst);
                        thread->bursts.push_back(ioBurst);
                    }
                    else thread->bursts.push_back(cpuBurst);
                }
                //collect metrics
                if(thread->process->ptype == Process::SYSTEM) numSystemThreads++;
                else if(thread->process->ptype == Process::INTERACTIVE) numInteractiveThreads++;
                else if(thread->process->ptype == Process::NORMAL) numNormalThreads++;
                else if(thread->process->ptype == Process::BATCH) numBatchThreads++;

                allThreadsAndArrivalTime[thread] = thread->arrivalTime;
            }
        }
        fileStream.close();
    }

    return tempQ;
}

void printVerbose(Event* event) {
    cout << "At time " << event->time << ":" << endl;
    cout << "\t" << event->typeStrings[event->type] << endl;
    cout << "\tThread " << event->thread->threadNum << " in process " <<
    event->thread->process->pid << " [" <<
    event->thread->process->typeStrings[event->thread->process->ptype] <<
    "]" << endl;
    switch(event->type){
        case Event::THREAD_ARRIVED:
        cout << "\tTransitioned from NEW to READY\n" << endl;
        break;
        case Event::THREAD_DISPATCH_COMPLETED:
        cout << "\tTransitioned from READY to RUNNING\n" << endl;
        break;
        case Event::PROCESS_DISPATCH_COMPLETED:
        cout << "\tTransitioned from READY to RUNNING\n" << endl;
        break;
        case Event::CPU_BURST_COMPLETED:
        cout << "\tTransitioned from RUNNING to BLOCKED\n" << endl;
        break;
        case Event::IO_BURST_COMPLETED:
        cout << "\tTransitioned from BLOCKED to READY\n" << endl;
        break;
        case Event::THREAD_COMPLETED:
        cout << "\tTransitioned from RUNNING to EXIT\n" << endl;
        break;
        case Event::THREAD_PREEMPTED:
        break;
        case Event::DISPATCHER_INVOKED:
        cout << "\tSelected from " << threadReadyQ.size() << " threads; will run to completion of burst\n" << endl;
        break;
        default:
        cout << "Event not found" << endl;
        break;
    }
}

void printPerThread() {
    cout << endl;
    cout << "PER THREAD METRICS" << endl;
    cout << endl;

    for(auto p: processes) {
        cout << "Process " << p->pid << " [" << p->typeStrings[p->ptype] << "] " << endl;
        for(auto t: p->threads){
            int arrTime = allThreadsAndArrivalTime[t];
            int ioTime = 0;
            int cpuTime = 0;
            int counter = 0;
            for(auto b: t->bursts){
                if(counter % 2 == 0) cpuTime += b->time;
                else ioTime += b->time;
                counter++;
            }

            cout << "\tThread " << t->threadNum << ":  Arr: " << arrTime <<
            " CPU: " << cpuTime << " I/O: " << ioTime << " TAT: " << t->endTime - arrTime <<
             " END: " << t->endTime<< endl;

        }
        cout << endl;
    }
}

//------------------------------------------------------------------
//+++++++++++++++++ EVENT HANDLER FUNCTIONS ++++++++++++++++++++++++
//------------------------------------------------------------------

void handlerThreadArrived(Event* event) {
    //change thread state from NEW to READY
    event->thread->state = Thread::READY;

    threadReadyQ.push(event->thread);

    if(threadRunningQ.empty()){
        Event* newEvent = new Event(Event::DISPATCHER_INVOKED, threadReadyQ.top()->arrivalTime);
        newEvent->thread = threadReadyQ.top();
        eventQ.push(newEvent);
    }
}

void handlerDispatcherInvoked(Event* event) {

    //If a new process, create a PROCESS_DISPATCH_COMPLETED event at correct time with overhead
    if(previousThread == NULL || threadReadyQ.top()->process->pid != previousThread->process->pid) {
        clockTime += processOverhead;
        totalDispatchTime += processOverhead;
        Event* newEvent = new Event(Event::PROCESS_DISPATCH_COMPLETED, event->time + processOverhead);
        newEvent->thread = threadReadyQ.top();
        eventQ.push(newEvent);

    } else {
        clockTime += threadOverhead;
        totalDispatchTime += threadOverhead;
        Event* newEvent = new Event(Event::THREAD_DISPATCH_COMPLETED, event->time + threadOverhead);
        newEvent->thread = threadReadyQ.top();
        eventQ.push(newEvent);
    }
    threadRunningQ.push(threadReadyQ.top());
    threadReadyQ.pop();
}

void handlerThreadDispatchCompleted(Event* event) {
    //change thread state from READY to RUNNING
    threadRunningQ.top()->state = Thread::RUNNING;

    if(threadRunningQ.top()->firstRunTime == 0) threadRunningQ.top()->firstRunTime = event->time;
    //Get the burst time of the current cpu burst
    int cpuBurstTime = threadRunningQ.top()->bursts[threadRunningQ.top()->burstNum]->time;
    clockTime += cpuBurstTime;
    threadRunningQ.top()->burstNum++;

    totalServiceTime += cpuBurstTime;
    Event* newEvent = new Event(Event::CPU_BURST_COMPLETED, event->time + cpuBurstTime);
    newEvent->thread = threadRunningQ.top();
    eventQ.push(newEvent);

}

void handlerProcessDispatchCompleted(Event* event) {
    //change thread state from READY to RUNNING
    threadRunningQ.top()->state = Thread::RUNNING;

    if(threadRunningQ.top()->firstRunTime == 0) threadRunningQ.top()->firstRunTime = event->time;
    //Get the burst time of the current cpu burst
    int cpuBurstTime = threadRunningQ.top()->bursts[threadRunningQ.top()->burstNum]->time;
    clockTime += cpuBurstTime;
    threadRunningQ.top()->burstNum++;

    totalServiceTime += cpuBurstTime;
    Event* newEvent = new Event(Event::CPU_BURST_COMPLETED, event->time + cpuBurstTime);
    newEvent->thread = threadRunningQ.top();
    eventQ.push(newEvent);
}

void handlerCPUBurstCompleted(Event* event) {
    int ioBurstTime = 0;

    //If this is the last CPU burst in the thread
    if(threadRunningQ.top()->burstNum >= threadRunningQ.top()->bursts.size()-1) {
        threadRunningQ.top()->endTime = event->time;
        Event* newEvent = new Event(Event::THREAD_COMPLETED, event->time);
        newEvent->thread = threadRunningQ.top();
        eventQ.push(newEvent);

    } else {
        //change thread state from RUNNING to BLOCKED
        threadRunningQ.top()->state = Thread::BLOCKED;

        int ioBurstTime = threadRunningQ.top()->bursts[threadRunningQ.top()->burstNum]->time;
        //clockTime += ioBurstTime;
        totalIOTime += ioBurstTime;

        Event* newEvent = new Event(Event::IO_BURST_COMPLETED, event->time + ioBurstTime);
        newEvent->thread = threadRunningQ.top();
        eventQ.push(newEvent);

        threadBlockedQ.push(threadRunningQ.top());
    }
    previousThread = threadRunningQ.top();
    threadRunningQ.pop();

    if(previousThread != threadReadyQ.top()){
        Event* newEvent2 = new Event(Event::DISPATCHER_INVOKED, event->time);
        newEvent2->thread = threadReadyQ.top();
        eventQ.push(newEvent2);
    }
}

void handlerIOBurstCompleted(Event* event) {

    threadBlockedQ.top()->arrivalTime = clockTime + threadBlockedQ.top()->bursts[threadBlockedQ.top()->burstNum]->time;
    threadBlockedQ.top()->burstNum++;
    threadReadyQ.push(threadBlockedQ.top());
    threadBlockedQ.pop();

    if(threadRunningQ.empty()){
        Event* newEvent = new Event(Event::DISPATCHER_INVOKED, event->time);
        newEvent->thread = threadReadyQ.top();
        eventQ.push(newEvent);
    }
}

void handlerThreadCompleted(Event* event) {
    //change thread state from RUNNING to EXIT
    event->thread->state = Thread::EXIT;

    totalElapsedTime = event->time;
}

void handlerThreadPreempted(Event* event) {

}


//Helper function that calls the correct handler function
void handleEventType(Event* event){
    switch(event->type){
        case Event::THREAD_ARRIVED:
        handlerThreadArrived(event);
        break;
        case Event::THREAD_DISPATCH_COMPLETED:
        handlerThreadDispatchCompleted(event);
        break;
        case Event::PROCESS_DISPATCH_COMPLETED:
        handlerProcessDispatchCompleted(event);
        break;
        case Event::CPU_BURST_COMPLETED:
        handlerCPUBurstCompleted(event);
        break;
        case Event::IO_BURST_COMPLETED:
        handlerIOBurstCompleted(event);
        break;
        case Event::THREAD_COMPLETED:
        handlerThreadCompleted(event);
        break;
        case Event::THREAD_PREEMPTED:
        handlerThreadPreempted(event);
        break;
        case Event::DISPATCHER_INVOKED:
        handlerDispatcherInvoked(event);
        break;
        default:
        cout << "Event not found" << endl;
        break;
    }
}

void requiredMetrics(){

        //response time
        for(auto t: allThreadsAndArrivalTime){
            if(t.first->process->ptype == Process::SYSTEM) {
                totalSystemResponseTime += (t.first->firstRunTime - t.second);
                totalSystemTurnaroundTime += (t.first->endTime - t.second);
            }
            else if(t.first->process->ptype == Process::INTERACTIVE) {
                totalInteractiveResponseTime += (t.first->firstRunTime - t.second);
                totalInteractiveTurnaroundTime += (t.first->endTime - t.second);
            }
            else if(t.first->process->ptype == Process::NORMAL) {
                totalNormalResponseTime += (t.first->firstRunTime - t.second);
                totalNormalTurnaroundTime += (t.first->endTime - t.second);
            }
            else if(t.first->process->ptype == Process::BATCH) {
                totalBatchResponseTime += (t.first->firstRunTime - t.second);
                totalBatchTurnaroundTime += (t.first->endTime - t.second);
            }
        }
        double avgSystemResponseTime = (double)totalSystemResponseTime / numSystemThreads;
        double avgInteractiveResponseTime = (double)totalInteractiveResponseTime / numInteractiveThreads;
        double avgNormalResponseTime = (double)totalNormalResponseTime / numNormalThreads;
        double avgBatchResponseTime = (double)totalBatchResponseTime / numBatchThreads;

        double avgSystemTurnaroundTime = (double)totalSystemTurnaroundTime / numSystemThreads;
        double avgInteractiveTurnaroundTime = (double)totalInteractiveTurnaroundTime / numInteractiveThreads;
        double avgNormalTurnaroundTime = (double)totalNormalTurnaroundTime / numNormalThreads;
        double avgBatchTurnaroundTime = (double)totalBatchTurnaroundTime / numBatchThreads;

        totalIdleTime = totalElapsedTime - totalServiceTime - totalDispatchTime;

        double cpuUtil = ((double)totalServiceTime + (double)totalDispatchTime) / totalElapsedTime*100.0;
        double cpuEfficiency = (double)totalServiceTime / (double)totalElapsedTime*100.0;

        //every run requires the follow output
        cout << endl;
        cout << "SYSTEM THREADS:" << endl;
        cout << "\tTotal Count: " << numSystemThreads << endl;
        cout << "\tAvg Response Time: " << avgSystemResponseTime << endl;
        cout << "\tAvg Turnaround Time: " << avgSystemTurnaroundTime << endl;
        cout << endl;
        cout << "INTERACTIVE THREADS:" << endl;
        cout << "\tTotal Count: " << numInteractiveThreads << endl;
        cout << "\tAvg Response Time: " << avgInteractiveResponseTime << endl;
        cout << "\tAvg Turnaround Time: " << avgInteractiveTurnaroundTime << endl;
        cout << endl;
        cout << "NORMAL THREADS:" << endl;
        cout << "\tTotal Count: " << numNormalThreads << endl;
        cout << "\tAvg Response Time: " << avgNormalResponseTime << endl;
        cout << "\tAvg Turnaround Time: " << avgNormalTurnaroundTime << endl;
        cout << endl;
        cout << "BATCH THREADS:" << endl;
        cout << "\tTotal Count: " << numBatchThreads << endl;
        cout << "\tAvg Response Time: " << avgBatchResponseTime << endl;
        cout << "\tAvg Turnaround Time: " << avgBatchTurnaroundTime << endl;
        cout << endl;
        cout << "Total elapsed time: " << totalElapsedTime << endl;
        cout << "Total service time: " << totalServiceTime << endl;
        cout << "Total I/O time: " << totalIOTime << endl;
        cout << "Total dispatch time: " << totalDispatchTime << endl;
        cout << "Total idle time: " << totalIdleTime << endl;
        cout << endl;
        cout << "CPU utilization: " << cpuUtil << endl;
        cout << "CPU efficiency: " << cpuEfficiency << endl;

}


int main(int argc, char* argv[]){
    //tokenize command line args, flags are returned. Last element is always fileName
    tokens = tokenizer(argc, argv);
    //parse the input file and get the priority_queue of events
    eventQ = parseFile(tokens[tokens.size()-1]);

    //DELIVERABLE 1: output THREAD_ARRIVED events in the VERBOSE format
    //Prob need to abstract this to be more general in next deliverables
    while(!eventQ.empty()){
        Event* nextEvent = eventQ.top();
        //if(nextEvent->type == Event::THREAD_COMPLETED) return -1;

        if(tokenSet.count("v") == 1) printVerbose(nextEvent);

        //Checks the event and calls the respective handler function
        handleEventType(nextEvent);

        eventQ.pop();
    }
    if(tokenSet.count("t") == 1) printPerThread();
    requiredMetrics();

}
