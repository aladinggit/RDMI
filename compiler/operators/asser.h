#ifndef _ASSER_H
#define _ASSER_H

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <regex>

#include "op.h"
#include "../utils/colors.h"

using namespace std;

class Asser : public Op {
private:
	string addr_h;     // High address bound
	string addr_l;     // Low address bound
	friend class Policy;

public:
	Asser(){};

	void set_high(string offset) { this->addr_h = offset; }
	void set_low(string low) { this->addr_l = low; }

	string get_high() { return this->addr_h;}
	string get_low() { return this->addr_l;}
    string get_high_prev(){ return this->addr_h.substr(2, 4);}
	string get_high_post(){ return this->addr_h.substr(6, 4);}
    string get_low_prev(){ return this->addr_l.substr(2, 4);}
    string get_low_post(){ return this->addr_l.substr(6, 4);}

	string to_string() {
		string ans;

		ans += "High addr is : " + this->addr_h;
		ans += "\n";

		ans += "Low addr is: " + this->addr_l;
		ans += "\n";

		return ans;
	}
	void print() {
		cout << bold << yellow << "Asser:" << reset << endl;
		cout << yellow << this->to_string() <<reset << endl;
	}
	string get_op_name() { return "Asser"; }
	string gen_statemachine(){return "state_machine";};
};


#endif
