#ifndef _PARSER_H
#define _PARSER_H

#include <string>
#include <vector>
#include <iostream>

#include "./operators/op.h"
#include "./utils/colors.h"

#include "./operators/kernel.h"
#include "./operators/traverse.h"
#include "./operators/in.h"
#include "./operators/iter.h"
#include "./operators/values.h"
#include "./operators/in.h"
#include "./operators/end.h"
#include "./operators/asser.h" // adding assert logic

// aim header file
#include "./aim/aim.h"
#include "./aim/constmove.h"
#include "./aim/init.h"
#include "./aim/pop.h"
#include "./aim/readload.h"
#include "./aim/constload.h"
#include "./aim/decjump.h"
#include "./aim/negjump.h"
#include "./aim/push.h"
#include "./aim/readmove.h"


#include <stack>
#include "./end_state.h"

using namespace std;

#ifndef throw_error
#define throw_error(msg) throw std::runtime_error(string(__FILE__)+":"+std::to_string(__LINE__)+" --> "+msg);
#endif

class Policy {
private:
    vector<string> lines;
    vector<Op*> ops;
    vector<Aim*> head_aims;
    stack<Aim*> tail_aims;
    vector<Aim*> all_aims;

public:

    //    Aim* cur_end; // current ending aim

    vector<Aim*> pgt_aims;
    string last_primitive_name;
    vector<int> last_state;
    int avail_state, reversed_avail_state, state_idx, base_state;
    int stack_top = -1, base_idx = 0;
    end_st end_state;
    end_st drop_state;
    int iter_num;
    int qpn_tran_coef;
    int fake_state; // used for marking the fake state, i.e. r_prev_state
    int cur_pos;
    int task_nr = 0;

    int policy_num; // used to specify the number of policy inside DSL.

	Policy(){
    };
	Policy(string input_file, int qpn_s, int qpn_t, int num, int base);

	void parse();
    void frontend_compile(); // frontend
    string backend_compile(); // backend

// OP gen
	KernelGraph* parse_kernelgraph(string line);
	Traverse* parse_traverse(string line);
	In* parse_in(string line);
	Iter* parse_iter(string line);
	Values* parse_values(string line);
    End* parse_end(string line);
    Asser* parse_asser(string line);

    int qpn_tran(int qpn){return qpn + qpn_tran_coef;} // from 3000 to 300
    int qpn_rtran(int qpn){return qpn - qpn_tran_coef;} // reverse, from 300 to 3000

// AIM gen
    void gen_kgraph_aim(KernelGraph *);
    void gen_traverse_aim(Traverse *);
    void gen_iter_aim(Iter *);
    void gen_in_aim(In *in);
    void gen_values_aim(Values *);
    void gen_end_aim(End *);

// Code gen
    int find_next_post_qpn(int);
    string gen_pgt_aims_code(void);
    string gen_readmove_pgt_walk_code(void); // generate page table walk rule
    string gen_load_max(void); // generate max entry loading rules
    string gen_offset_encoding(void); // generate offset encoding rules
    string gen_pc_tran(void); // generate PC transition rules
    string gen_psn_mapping(void); // generate PSN mapping rules
    string gen_base_operation(void); // generate base address operations
    string gen_init_code(Init *);
    string gen_constload_code(ConstLoad *);
    string gen_readload_code(ReadLoad *);
    string gen_decjump_code(DecJump *);
    string gen_negjump_code(NegJump *);
    string gen_pop_code(Pop *);
    string gen_push_code(Push *);
    string gen_readmove_code(ReadMove *);
    string gen_constmove_code(ConstMove *);

    void merge_aims(){
        // merge head
        for (int i = 0; i < this->head_aims.size(); i++){
            this->all_aims.push_back(this->head_aims.at(i));
            this->head_aims.at(i)->print();
        }
        // merge tail
        while (!this->tail_aims.empty()) {
            // TODO: fix this.
            this->all_aims.push_back(this->tail_aims.top());
            this->tail_aims.top()->print();
            this->tail_aims.pop();
        }
    };    
//    string compile_traverse(Traverse* tt);
//    string compile_value(Values* vv);
//    string compile_iter(Iter* ii);
////    string compile_out(Out* oo);
//    string compile_end(End* ed);
//    string compile_kernelgraph(KernelGraph *kk);
////    string compile_check(Check *cc);

	string to_strings() {
		string s;
		for (string l: lines)
			s += l + "\n";
		return s;
	};

	void print() {cout << this->to_strings() << endl; }
//	void print_stmt();
    void mark_iter(void); // used for checking the dynamic iter.
    void mark_assert(void); // used  for checking the assert logic
    void gen_pgt_walk_aim(void);


	int get_num_stmts() { return ops.size(); }
	vector<Op*> get_stmts() {return ops;}
};



#endif
