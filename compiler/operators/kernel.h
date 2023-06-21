#ifndef _KERNELGRAPH_H
#define _KERNELGRAPH_H

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <regex>

#include "op.h"
#include "../utils/colors.h"

using namespace std;

class KernelGraph : public Op {
private:
	string root;

	friend class Policy;

public:
	KernelGraph(){};

	void set_root(string root) { this->root = root;	}
	string get_root() {return this->root;}
	string to_string() {
		string ans;
		ans += "Root: " + this->root;
		ans += "\n";

		return ans;
	}
	void print() {
		cout << bold << yellow << "KernelGraph:" << reset << endl;
		cout << yellow << this->to_string() <<reset << endl;
	}
	string get_op_name() { return "KernelGraph"; }
	string gen_statemachine(){return "state_machine";};
};


#endif