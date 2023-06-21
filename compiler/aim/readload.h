#ifndef _READLOAD_H
#define _READLOAD_H

#include <string>

#include "aim.h"

using namespace std;

class ReadLoad : public Aim  {
private:
	friend class Policy;
    int str; // used to specify whether the content will be stored
	int reg_index = -1;
	int offset = 0; 
    int size = 8; // need to specify size somehow
    vector<int> prev_qpns;
    int post_qpn = -1;
    int tran_qpn = -1;
    int tran_dqpn = -1;
    int range_check = -1;

    // range check bound
    string addr_h = "0";
    string addr_l = "0";

public:
	ReadLoad(int offset){
		this->offset = offset;
	};

    void set_range_check(int check){
        this->range_check = check;
    }

    void set_reg_index(int idx){
        this->reg_index = idx;
    }

    void add_prev_qpn(int prev_qpn) {
        this->prev_qpns.push_back(prev_qpn);
    }

    void set_post_qpn(int post_qpn) {
        this->post_qpn = post_qpn;
    }

    void set_tran_qpn(int tran_qpn){
        this->tran_qpn = tran_qpn;
    }

    void set_tran_dqpn(int tran_dqpn){
        this->tran_dqpn = tran_dqpn;
    }

    // assert logic
	void set_high(string offset) { this->addr_h = offset; }
	void set_low(string low) { this->addr_l = low; }


    string get_high_prev(){ return this->addr_h.substr(2, 4);}
	string get_high_post(){ return this->addr_h.substr(6, 4);}
    string get_low_prev(){ return this->addr_l.substr(2, 4);}
    string get_low_post(){ return this->addr_l.substr(6, 4);}

    int get_post_qpn(){ return this->post_qpn; }
    int get_tran_dqpn() { return this->tran_dqpn; }
    int get_tran_qpn(){ return this->tran_qpn; }
    int get_reg_index(){ return this->reg_index;}
    vector<int> get_prev_qpn(){ return this->prev_qpns; }
    int get_prev_qpn_size(){ return this->prev_qpns.size();}
    int get_offset(){return this->offset;}
    int get_range_check() { return this->range_check;}



    string to_string(){
        string ans;

        ans += "offset is " + std::to_string(this->offset);
        ans += '\n';

        ans += "size to read is " + std::to_string(this->size);
        ans += '\n';

        ans += "register to store value is " + std::to_string(this->reg_index);
        ans += '\n';

        ans += "range check is " + std::to_string(this->range_check);
        ans += '\n';

        ans += "for the ending load: new md.QPN to transfer to is " + std::to_string(this->tran_qpn) +
                    " dqpn is " +  std::to_string(this->tran_dqpn);
        ans += '\n';


        int num = 0;
        for (num = 0; num < prev_qpns.size(); num++){
            ans += "previous qpn is " + std::to_string(this->prev_qpns.at(num));
            ans += '\n';
        }

        ans += "qpn to execute is " + std::to_string(this->post_qpn);
        ans += '\n';

        return ans;
    }

	void print(){
        cout << bold << yellow << "Read Load(offset)" << reset << endl;
        cout << yellow << this->to_string() << reset << endl;
    }
    string get_aim_name() {return "ReadLoad";}
    string get_ctrl_rules(){return "null";}

};


#endif // _READLOAD_H
