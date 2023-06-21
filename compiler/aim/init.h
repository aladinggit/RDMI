#ifndef _INIT_H
#define _INIT_H

#include <string>

#include "aim.h"

using namespace std;

class Init : public Aim  {
private:
	friend class Policy;

	int init_qpn = -1;
	int post_qpn = 999;

public:
	Init(){};

	void set_init_qpn(int prev_qpn) {
		this->init_qpn = prev_qpn;
	}

	void set_post_qpn(int post_qpn) {
		this->post_qpn = post_qpn;
	}

	int get_post_qpn() {
		return this->post_qpn;
	}

    int get_init_qpn(){
        return this->init_qpn;
    }

    string to_string(){
        string ans;

        ans += "the init_qpn is " + std::to_string(this->init_qpn);
        ans += '\n';

        ans += "The current drop state is " + std::to_string(this->post_qpn);
        ans += '\n';

        return ans;
    }

	void print(){
        cout << bold << yellow << "Init " << reset << endl;
        cout << yellow << this->to_string() << reset << endl;
    }
	string get_aim_name() {return "Init";}
	string get_ctrl_rules(){return "null";}
};


#endif // _INIT_H
