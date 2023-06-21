#ifndef _IN_H
#define _IN_H

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <regex>

#include "op.h"
#include "../utils/colors.h"

using namespace std;

class In: public Op {
private:
	string offset;
    bool dec = 0;
    string type;
    string var;
	friend class Policy;

public:
	In(){};

	void set_offset(string offset) { this->offset = offset;	}
    void set_type(string type) { this->type = type;}
    void set_var(string var) { this->var = var;}
    void set_dec(bool i) { this->dec = i;}
    
	string get_offset() {return this->offset;}
    string get_type() {return this->type;}
    string get_var() {return this->var;}
    bool get_dec() {return this->dec;}

	string to_string() {
		string ans;
		ans += "Offset is : " + this->offset;
		ans += "\n";

		return ans;
	}
	void print() {
		cout << bold << yellow << "In:" << reset << endl;
		cout << yellow << this->to_string() <<reset << endl;
	}
	string get_op_name() { return "In"; }
	string gen_statemachine(){return "state_machine";};
};


#endif
