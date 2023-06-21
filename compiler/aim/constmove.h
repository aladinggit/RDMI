#ifndef _CONSTMOVE_H
#define _CONSTMOVE_H

#include <string>

#include "aim.h"

using namespace std;

class ConstMove : public Aim  {
private:
	friend class Policy;
	int offset;
    vector<int> prev_qpns;	
    int post_qpn = -1;

public:
	ConstMove(int offset){
		this->offset = offset;
	};

    void add_prev_qpn(int prev_qpn) {
        this->prev_qpns.push_back(prev_qpn);
    }

    void set_post_qpn(int post_qpn) {
        this->post_qpn = post_qpn;
    }

    int get_post_qpn() { return this->post_qpn;}
    vector<int> get_prev_qpn() { return this->prev_qpns;}

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

        return ans;
    }

	void print(){
        cout << bold << yellow << "Const Move($offset)" << reset << endl;
        cout << yellow << this->to_string() << reset << endl;

        
    
    }
	string get_aim_name() {return "ConstMove";}
    string get_ctrl_rules(){return "rules";}
    int get_offset() {return offset;}
    int get_prev_qpns_size() { return this->prev_qpns.size();}
};


#endif // _CONSTMOVE_H
