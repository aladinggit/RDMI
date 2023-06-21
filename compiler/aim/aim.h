#ifndef _AIM_H
#define _AIM_H

#include <string>

#include "../utils/colors.h"

using namespace std;

class Aim {
private:
	friend class Policy;

public:
	Aim(){};
	virtual string to_string()= 0;
	virtual void print() = 0;
	virtual string get_aim_name() = 0;
//    virtual void update_qpn(int prev_qpn, int post_qpn) = 0;
    virtual string get_ctrl_rules() = 0;
};


#endif // _AIM_H
