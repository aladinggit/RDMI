#ifndef _POP_H
#define _POP_H

#include <string>

#include "aim.h"

using namespace std;

class Pop : public Aim  {
private:
	friend class Policy;

	vector<int> prev_qpns;
	int post_qpn = -1;

public:
	Pop(){};

	void add_prev_qpn(int prev_qpn) {
		this->prev_qpns.push_back(prev_qpn);
	}

	void set_post_qpn(int post_qpn) {
		this->post_qpn = post_qpn;
	}

    string to_string(){
        string ans;

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
        cout << bold << yellow << "Pop()" << reset << endl;
        cout << yellow << this->to_string() << reset << endl;
    }
    string get_aim_name() {return "Pop";}
    string get_ctrl_rules(){return "null";}
};


#endif // _POP_H
