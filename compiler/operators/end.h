#ifndef _END_H
#define _END_H

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <regex>

#include "op.h"
#include "../utils/colors.h"

using namespace std;

class End : public Op {
private:
	friend class Policy;

public:
	End(){};

	string to_string() {
		string ans;
		ans += "end ";
		ans += "\n";

		return ans;
	}
	void print() {
		cout << bold << yellow << "End:" << reset << endl;
		cout << yellow << this->to_string() <<reset << endl;
	}
	string get_op_name() { return "End"; }
	string gen_statemachine(){return "state_machine";};
};


#endif
