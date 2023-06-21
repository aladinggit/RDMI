#ifndef _ITER_H
#define _ITER_H

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <regex>

#include "op.h"
#include "../utils/colors.h"

using namespace std;

class Iter : public Op {
private:
	string offset;
	string ssteps;
    string size;
    int dynamic = 0;
    int seq = -1;  // position to find max_fds
	friend class Policy;

public:
	Iter(){};

    void set_seq(int seq) {this->seq = seq;}
    void set_dynamic(int dyn) {
        //cout<<"qqqqv "<< dyn << endl;
        this->dynamic = dyn; }
	void set_offset(string offset) { this->offset = offset; }
	void set_sstep(string step) {
		this->ssteps = step;
//		this->stoi_step(step);
	}
    void set_size(string size) {
        this->size = size;
    }
    int get_dynamic() { return this->dynamic;}
    int get_seq(){return this->seq;}
	string get_offset() {return this->offset;}
	string get_sstep() {return this->ssteps; }
    string get_size() {return this->size;}

	string to_string() {
		string ans;

		ans += "Offset: " + this->offset;
		ans += "\n";

		ans += "ssteps: " + this->ssteps;
		ans += "\n";

		ans += "size: " + this->size;
		ans += "\n";

        ans += "dynamic: ";
        cout << "dynamic is " << this->dynamic << endl;
        ans += "\n";

        ans += "seq: " ;
        cout << "seq is " << this->seq << endl;

        ans += '\n';

		return ans;
	}
	void print() {
		cout << bold << yellow << "Iter:" << reset << endl;
		cout << yellow << this->to_string() <<reset << endl;
	}
	string get_op_name() { return "Iter"; }
	string gen_statemachine(){return "state_machine";};
};


#endif
