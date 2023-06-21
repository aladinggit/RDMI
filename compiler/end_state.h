#ifndef _STACK_H
#define _STACK_H

#include <string>
#include <vector>
#include <iostream>

using namespace std;

class end_st {
public:
//    string name;
    int dqpn; // ib_bth.dqpn
    int qpn; // md.qpn

//    end(string name){
//        this->name = name;
//    }

    void set_dqpn(int dqpn){
        this->dqpn = dqpn;
    }

    void set_qpn(int qpn){
        this->qpn = qpn;
    }

//    string get_name(){
//        return this->name;
//    }

    int get_dqpn(){
        return this->dqpn;
    }

    int get_qpn(){
        return this->qpn;
    }
};
#endif
