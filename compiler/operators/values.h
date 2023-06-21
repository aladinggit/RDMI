#ifndef _VALUES_H
#define _VALUES_H

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <regex>

#include "op.h"
#include "../utils/colors.h"

using namespace std;

class Values : public Op {
private:
	vector<string> fields;
    string semantic_map = "0";
    int reg_nr = -1; // position to find max_fds
    string name;
	friend class Policy;
	string addr_h = "0";     // High address bound used for assert
	string addr_l = "0";     // Low address bound used for assert

public:
	Values(){};

    void set_name(string name) {this->name = name;}
    void set_regnr(int i) { this->reg_nr = i;} // set register to store max_fd
    void set_smtbit(string map) { this->semantic_map = map; }
    string get_smtbit() {return this->semantic_map; }
	void add_field(string field) { this->fields.push_back(field); }
	vector<string> get_fields() {return this->fields;}
    int get_num() {return this->fields.size();}
    int get_regnr() { return this->reg_nr;}
    string get_name() { return this->name;}

    // assert logic
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
		ans += "field: [";
		for (string field: this->fields)
			ans += field + ",";
		ans.pop_back();
		ans += "]\n";

        ans += "the check is :" + this->semantic_map;
        ans += "\n";

        ans += "high addr to check :" + this->addr_h;
        ans += "\n";

        ans += "low addr to check :" + this->addr_l;
        ans += "\n";

        ans += "name is: " + this->name;
        ans += "\n";

        ans += "the " + this->name + "is stored at" ;
        cout << " the seq is " << this->reg_nr << endl;
        ans += '\n';
		
        return ans;
	}

	void print() {
		cout << bold << yellow << "Values:" << reset << endl;
		cout << yellow << this->to_string() <<reset << endl;
	}
	string get_op_name() { return "Values"; }
	string gen_statemachine(){return "state_machine";};
};


#endif
