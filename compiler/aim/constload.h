#ifndef _CONSTLOAD_H
#define _CONSTLOAD_H

#include <string>

#include "aim.h"

using namespace std;

class ConstLoad : public Aim {
private:
	friend class Policy;
	int reg_index = -1;
	int value = -1;
    vector<int> prev_qpns;
	int post_qpn = 0;
    int seq = -1;

public:
	ConstLoad(int reg_index){
		this->reg_index = reg_index;
		// this->value = value;
	};

    void set_seq(int seq){ // which entry to store the max_fd
        this->seq = seq;
    }

    void set_value(int val){
        this->value = val;
    }

    void add_prev_qpn(int prev_qpn) {
        this->prev_qpns.push_back(prev_qpn);
    }

    void set_post_qpn(int post_qpn) {
        this->post_qpn = post_qpn;
    }

    int get_seq(){ return this->seq;}
    int get_value(){return this->value;}

    int get_post_qpn() {
        return this->post_qpn;
    }

    vector<int> get_prev_qpn(){
        return this->prev_qpns;
    }

    string to_string(){
        string ans;

        ans += "value to load to register is  " + std::to_string(this->value);
        ans += '\n';


        ans += "register to store value is " + std::to_string(this->reg_index);
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
        cout << bold << yellow << "Const Load($value)" << reset << endl;
        cout << yellow << this->to_string() << reset << endl;
    }

	string get_aim_name() {return "ConstLoad";}
	string get_ctrl_rules(){return "null";}
};


#endif // _CONSTLOAD_H
