#ifndef _NEGJUMP_H
#define _NEGJUMP_H

#include <string>

#include "aim.h"

using namespace std;

class NegJump : public Aim  {
private:
	friend class Policy;

	string addr_h;
    string addr_l;
	int prev_qpn = -1; // not used in our case?
    int post_qpn = -1;
	int true_post_qpn = -1;
	int false_post_qpn = -1;
	int fake_state = -1;

	int label1 = 0;
	int label2 = 0;

public:
	NegJump(string addr_h, string addr_l){
		this->addr_h = addr_h;
        this->addr_l = addr_l;
	};

	void set_fake_qpn(int qpn) {
		this->fake_state = qpn;
	}


	void set_prev_qpn(int prev_qpn) {
		this->prev_qpn = prev_qpn;
	}

	int get_prev_qpn() {
		return this->prev_qpn;
	}

	void set_post_qpn(int post_qpn) {
		this->post_qpn = post_qpn;
	}

	int get_post_qpn() {
		return this->post_qpn;
	}

	void set_true_post_qpn(int post_qpn) {
		this->true_post_qpn = post_qpn;
	}

	void set_false_post_qpn(int post_qpn) {
		this->false_post_qpn = post_qpn;
	}

	void udpate_label(int label1, int label2) {
		this->label1 = label1;
		this->label2 = label2;
	}

	string get_addr_h(){ return this->addr_h;}
	string get_addr_l(){ return this->addr_l;}
	int get_true_post_qpn(){ return this->true_post_qpn;}
	int get_false_post_qpn(){ return this->false_post_qpn;}
	int get_fake_qpn() {return this->fake_state;}


    string to_string(){
        string ans;

        ans += "addr to stop traversal is " + this->addr_h + this->addr_l;
        ans += '\n';

        ans += "for the end checking: Stop criteria dQPN is " + std::to_string(this->true_post_qpn) +
                    " Non-stop dQPN is " +  std::to_string(this->false_post_qpn);
        ans += '\n';


        ans += "qpn to execute JMP is " + std::to_string(this->post_qpn);
        ans += '\n';

        return ans;
    }

	void print(){
        cout << bold << yellow << "NegJump(base == addr, L1, L2)" << reset << endl;
        cout << yellow << this->to_string() << reset << endl;
    }
    string get_aim_name() {return "NegJump";}
    string get_ctrl_rules(){return "null";}
};


#endif // _NEGJUMP_H
