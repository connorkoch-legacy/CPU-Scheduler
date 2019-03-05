#pragma once

using namespace std;

class Burst{
public:
    enum Type {CPU, IO};
    Type type;
    int time;

    Burst(int time, Type type){
        this->time = time;
        this->type = type;
    }
};
