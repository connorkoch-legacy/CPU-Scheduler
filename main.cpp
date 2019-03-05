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

//comparator for priority_queue: lower arrival time = higher priority
struct CompareEvents{
    bool operator()(Event* e1, Event* e2){
        return e1->time > e2->time;
    }
};

vector<string> tokenizer(int argc, char* argv[]){

    unordered_set<string> tokenSet;

    //no flags
    if(argc == 2) {
        vector<string> tokens;
        tokens.push_back(argv[1]);
        return tokens;
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
        vector<string> tokens(tokenSet.begin(), tokenSet.end());
        tokens.push_back(argv[argc-1]);
        return tokens;
    }
}

priority_queue<Event*, vector<Event*>, CompareEvents> parseFile(string fileName){

    priority_queue<Event*, vector<Event*>, CompareEvents> eventQ;

    ifstream fileStream(fileName);
    if (fileStream.is_open())
    {
        //first line = numProcesses. threadOverhead, processOverhead
        int numProcesses, threadOverhead, processOverhead;
        fileStream >> numProcesses >> threadOverhead >> processOverhead;
        //cout << numProcesses << threadOverhead << processOverhead << endl;
        vector<Process*> processes;

        for(int i = 0; i < numProcesses; i++){
            int pid, pType, numThreads;
            fileStream >> pid >> pType >> numThreads;
            Process* process = new Process(pid, static_cast<Process::Type>(pType));
            processes.push_back(process);

            for(int j = 0; j < numThreads; j++){
                int threadArrival, numBursts;
                fileStream >> threadArrival >> numBursts;
                Thread* thread = new Thread(threadArrival, numBursts, j, process, Thread::NEW);
                process->threads.push_back(thread);
                //Add THREAD_ARRIVED event to the priority_queue
                Event* event = new Event(Event::THREAD_ARRIVED, threadArrival);
                event->thread = thread;
                eventQ.push(event);

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
            }
        }

        fileStream.close();
    }

    return eventQ;
}

int main(int argc, char* argv[]){
    //tokenize command line args
    vector<string> tokens = tokenizer(argc, argv);
    //parse the input file and get the priority_queue of events
    priority_queue<Event*, vector<Event*>, CompareEvents> eventQ;
    eventQ = parseFile(tokens[tokens.size()-1]);

    //DELIVERABLE 1: output THREAD_ARRIVED events in the VERBOSE format
    //Prob need to abstract this to be more general in next deliverables
    while(!eventQ.empty()){
        Event nextEvent = *(eventQ.top());
        cout << "At time " << nextEvent.time << ":" << endl;
        cout << "\t" << nextEvent.typeStrings[nextEvent.type] << endl;
        cout << "\tThread " << nextEvent.thread->threadNum << " in process " <<
                nextEvent.thread->process->pid << " [" <<
                nextEvent.thread->process->typeStrings[nextEvent.thread->process->ptype] <<
                "]" << endl;
        cout << "\tTransitioned from NEW to READY\n" << endl;

        eventQ.pop();
    }

}
