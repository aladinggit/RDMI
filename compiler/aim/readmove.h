#ifndef _READMOVE_H
#define _READMOVE_H

#include <string>

#include "aim.h"

using namespace std;

class ReadMove : public Aim  {
private:
	friend class Policy;
	int offset;
    vector<int> prev_qpns;
    int post_qpn = -1;
	int qpn_null = -1; // handle null ptr redirection
    int dqpn_null = -1;
public:
	ReadMove(int offset){
		this->offset = offset;
	};

    void set_qpn_null(int label) {
        this->qpn_null = label;
    }

    // void set_qpn_next(int qpn){
    //     this->qpn_next = qpn;
    // }

    void set_dqpn_null(int label) {
        this->dqpn_null = label;
    }

    void add_prev_qpn(int prev_qpn) {
        this->prev_qpns.push_back(prev_qpn);
    }

    void set_post_qpn(int post_qpn) {
        this->post_qpn = post_qpn;
    }

    void set_offset(int offset){
        this->offset = offset;
    }

    string to_string(){
        string ans;

        ans += "offset is " + std::to_string(this->offset);
        ans += '\n';

        int num = 0;
        for (num = 0; num < prev_qpns.size(); num++){
            ans += "previous qpn is " + std::to_string(this->prev_qpns.at(num));
            ans += '\n';
        }

        ans += "qpn to execute is " + std::to_string(this->post_qpn);
        ans += '\n';

        ans += "upon null, redirect to: md.QPN " + std::to_string(this->qpn_null)
                     + " dqpn " + std::to_string(this->dqpn_null);
        ans += "\n";

        return ans;
    }
    int get_post_qpn(){ return this->post_qpn; }
    int get_qpn_null(){ return this->qpn_null; }
    int get_dqpn_null(){ return this->dqpn_null;}
    int get_prev_qpn_size(){ return this->prev_qpns.size();}
    int get_offset(){return this->offset;}
    vector<int> get_prev_qpn() {return this->prev_qpns;}
    // int get_qpn_tran() { return this->qpn_next; }


	void print(){
        cout << bold << yellow << "Read Move(offset)" << reset << endl;
        cout << yellow << this->to_string() << reset << endl;
    }
    string get_aim_name() {return "ReadMove";}
    string get_ctrl_rules(){return "null";}
};


#endif // _READMOVE_H
