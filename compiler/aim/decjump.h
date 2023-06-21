#ifndef _DECJUMP_H
#define _DECJUMP_H

#include <string>

#include "aim.h"

using namespace std;

class DecJump : public Aim  {
private:
	friend class Policy;

	int reg_index = -1;
    int prev_qpn = -1; // not used in our case?
	int post_qpn = -1;
    int true_post_qpn = -1;
    int false_post_qpn = -1;
	int label1 = 0;
	int label2 = 0;

public:
	DecJump(int reg_index){
		this->reg_index = reg_index;
	};

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

    int get_reg_idx(){return this->reg_index;}
	int get_true_post_qpn(){ return this->true_post_qpn;}
	int get_false_post_qpn(){ return this->false_post_qpn;}

    string to_string(){
        string ans;

        ans += "register to dec is " + std::to_string(this->reg_index);
        ans += '\n';

        ans += "for the end checking: Stop criteria dQPN is " + std::to_string(this->true_post_qpn) +
                    " Non-stop dQPN is " +  std::to_string(this->false_post_qpn);
        ans += '\n';


        ans += "qpn to execute JMP is " + std::to_string(this->post_qpn);
        ans += '\n';

        return ans;
    }

	void print(){
        cout << bold << yellow << "DecJump(R0--, L1, L2) " << reset << endl;
        cout << yellow << this->to_string() << reset << endl;
    }

    string get_aim_name() {return "DecJump";}
    string get_ctrl_rules(){return "null";}
};


#endif // _DECJUMP_H
