#include <sys/time.h>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include "policy.h"

using namespace std;



int policy1(string path, int qpn_s, int qpn_r, int i, int base) {
//	string path("./policies/policy1.c");
	Policy *d = new Policy(path, qpn_s, qpn_r, i, base);
	d->parse();
    d->mark_iter();
    d->mark_assert();
	d->frontend_compile();
    d->gen_pgt_walk_aim();
    string trans_rule = "pd-master\n";
    trans_rule += d->backend_compile();
    trans_rule += '\n';
    trans_rule += d->gen_pc_tran();
    trans_rule += '\n';
    trans_rule += d->gen_base_operation();
    trans_rule += '\n';
    trans_rule += d->gen_psn_mapping();
    trans_rule += '\n';
    trans_rule += d->gen_offset_encoding();
    trans_rule += '\n';
    trans_rule += d->gen_load_max();
    trans_rule += '\n';
    trans_rule += d->gen_readmove_pgt_walk_code();
    trans_rule += '\n';
    trans_rule += d->gen_pgt_aims_code();
    trans_rule += "exit";
    ofstream file;
    string pp = "./gencode/code_gen" + to_string(i) + ".cmd";
    file.open(pp);
    file << trans_rule << endl;
    file.close();
    return d->avail_state;
}

int main (int argc, char *argv[]) {
    printf("begin compiling: ./RDMI 3000 300 10");
    if(argc != 4){
        cout << "the num of param is 4!! dqpn, qpn, policy_num" << endl;
        exit(0);
    }
    int num = (stoi)(argv[3]);
    auto start = chrono::steady_clock::now();
    string path = "./policies/policy";
    // calculate qpn_tran_coef
    int qpn_tran_coef = (stoi)(argv[2]) - stoi(argv[1]);
//    path = "./policies/policy0.c";
    path = "./exe/policy0.c";
    int new_avail_state = policy1(path, stoi(argv[1]), (stoi)(argv[2]), 0, stoi(argv[1])); // 0th policy;
    cout << "the 2 coeffs are " << (int)(*argv[1]) << "   " << (int)(*argv[2]) << endl;

    string control_rule;
    control_rule += "0 th policy's state is " + to_string(stoi(argv[1])) + " and " + to_string(stoi(argv[2])) + '\n';

    for (int i = 1; i < num; i++){
//        path = "./policies/policy" + to_string(i) + ".c";
        path = "./exe/policy" + to_string(i) + ".c";
        cout << bold << blue << i << "th policy's state is " << new_avail_state << " and " <<
                 new_avail_state + qpn_tran_coef  << reset << endl;
        control_rule += to_string(i) + " th policy's state is " + to_string(new_avail_state) + " and "+ to_string(new_avail_state + qpn_tran_coef) + '\n';
        new_avail_state = policy1(path, new_avail_state, new_avail_state + 
            qpn_tran_coef, i, stoi(argv[1])); // ith policy
    }

	auto end = chrono::steady_clock::now();
	cout << "Elapsed time in seconds: "
	<< chrono::duration_cast<chrono::milliseconds>(end - start).count()
	<< " milliseconds" << endl;

    ofstream fil;    
    string pat = "./gencode/summary";
    fil.open(pat);
    fil << control_rule << endl;
    fil.close();
	return 0;
}
