#ifndef _TRAVERSE_H
#define _TRAVERSE_H

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <regex>

#include "op.h"
#include "../utils/colors.h"

using namespace std;

class Traverse : public Op {
private:
	string offset;  // 1st arg: the offset to the "next"
	string end;     // 2nd arg: the end address
	string type;    // 3rd arg: the link list data structure
	friend class Policy;

public:
	Traverse(){};

	void set_offset(string offset) { this->offset = offset; }
	void set_end(string end) { this->end = end; }
	void set_type(string type) { this->type = type; }


    string get_high(){ return this->end.substr(2, 8);}
    string get_low(){ return this->end.substr(10, 8);}
    string get_offset(){ return this->offset;}
    string get_type(){ return this->type;}

	string to_string() {
		string ans;

		ans += "Next offset is : " + this->offset;
		ans += "\n";

		ans += "End: " + this->end;
		ans += "\n";

		ans += "Data type is : " + this->type;
		ans += "\n";

		return ans;
	}
	void print() {
		cout << bold << yellow << "Traverse:" << reset << endl;
		cout << yellow << this->to_string() <<reset << endl;
	}
	string get_op_name() { return "Traverse"; }
	string gen_statemachine(){return "state_machine";};
};


#endif
