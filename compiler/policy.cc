#include "policy.h"
#include "utils/utils.h"
#include "utils/colors.h"

Policy::Policy(string input_file, int qpn_s, int qpn_r, int num, int base) {
    ifstream infile(input_file.c_str());
    assert(infile.is_open());
    cout << "reading from file " + input_file <<endl;
    // read file
    string l;
    regex r ("//.*|/\\*.*\\*/");
    while (getline(infile, l)) {
        if (l.empty()) //empty line
            continue;
        if (regex_match(l,r)) //comment line
            continue;
        this->lines.push_back(l);
        cout<<l<<endl;
    }

    //parse policy type
    if (this->lines.size() == 0) {
        throw_error("No valid statements in the policy!");
    }

    this->avail_state = qpn_s; // handle init_qpn 
    //this->base_state = qpn_s + 1; // basic QPN used for checking idx
    this->base_state = base + 1;
    this->qpn_tran_coef = qpn_r - qpn_s;
    cout << "coeff is "  << this->qpn_tran_coef << (qpn_r - qpn_s)  <<endl;
    this->fake_state = 0 + 20 * num;   // handle multi-task
                                     // used for specifying QPN key for re-entering loop
    this->end_state.set_qpn(998 - num); // end state doesn not need QPN_TRAN
    this->end_state.set_dqpn(999 - num);
    this->task_nr = num; //  multi-task
    // last state will use md.qpn(after QPN_TRAN)
}

/**
 * Extract supported operators from lines after the first line
 */
void Policy::parse() {
    string delim(30,'=');
    for (int i = 0; i < lines.size(); i ++) {
        string cur_line = trim(lines[i]);
        if (0 == cur_line.rfind(".traverse")) {
            cout << "> Processing a traverse primitive: " << blue << cur_line << reset << endl;
            Traverse *t = parse_traverse(cur_line);
            this->ops.push_back(t);
        } else if (0 == cur_line.rfind(".in")) {
            cout << "> Processing an in primitive: " << blue << cur_line << reset << endl;
            In *in = parse_in(cur_line);
            this->ops.push_back(in);
        } else if (0 == cur_line.rfind(".iter")) {
            cout << "> Processing an iter primitive: " << blue << cur_line << reset << endl;
            Iter *i = parse_iter(cur_line);
            this->ops.push_back(i);
        } else if (0 == cur_line.rfind(".values")) {
            cout << "> Processing a values primitive: " << blue << cur_line << reset << endl;
            Values *vs = parse_values(cur_line);
            this->ops.push_back(vs);
        } else if (0 == cur_line.rfind("KernelGraph")) {
            cout << "> Processing a KernelGraph primitive: " << blue << cur_line << reset << endl;
            KernelGraph *kg = parse_kernelgraph(cur_line);
            this->ops.push_back(kg);
        } else if (0 == cur_line.rfind(".assert")){
            cout << "> Processing an Assert primitive: " << blue << cur_line << reset << endl;
            Asser* asser = parse_asser(cur_line);
            this->ops.push_back(asser);
        } else if (0 == cur_line.rfind("End")) {
            cout << "> Processing the last primitive inside the above policy: " << blue << cur_line << reset << endl;
            End *ed = parse_end(cur_line);
            this->ops.push_back(ed);
        }
          else {
            cout << "> Current line: " << red << cur_line << reset <<endl;
            throw_error("Cannot recognize line the operator!");
        }
    }
}

KernelGraph* Policy::parse_kernelgraph(string line)
{
    regex reg_kg("KernelGraph\\s*\\((\\w+)\\)");
    if (!std::regex_match(line.begin(), line.end(), reg_kg)) {
        throw_error("Invalid kernelgraph statement");
    }

    smatch match;
    KernelGraph* kg = new KernelGraph();
    // extract the root
    if (regex_search(line, match, reg_kg)) {
        kg->set_root(match.str(1));
        // TODO: There are more things to do---Need to build the kernel graph.
    } else {
        throw_error("Invalid kernelgraph statement: Could not find the root");
    }

    kg->print();
    return kg;
}

End* Policy::parse_end(string line)
{
    regex reg_kg("End");
    if (!std::regex_match(line.begin(), line.end(), reg_kg)) {
        throw_error("Invalid kernelgraph statement");
    }
    End *ed = new End();
    ed->print();
    return ed;
}

Traverse* Policy::parse_traverse(string line) {
//    regex reg_t("\\.traverse\\s*\\((\\w+,\\s*&\\w+\\.\\w+,\\s*\\w+\\.\\w+)\\)");
    regex reg_t("\\.traverse\\s*\\((\\d+,\\s*\\w+,\\s*\\d+)\\)");
    if (!std::regex_match(line.begin(), line.end(), reg_t)) {
        throw_error("Invalid traverse statement");
    }

    smatch match;
    regex sreg_t("\\.traverse\\s*\\((\\w+),\\s*(\\w+),\\s*(\\w+)\\)");
    Traverse *traverse = new Traverse();
    if (regex_search(line, match, sreg_t)) {
        traverse->set_offset(match.str(1));
        traverse->set_end(match.str(2));
        traverse->set_type(match.str(3));
    } else {
        throw_error("Invalid traverse statement");
    }

    traverse->print();
    return traverse;
}

In* Policy::parse_in(string line)
{
    regex reg_in(".in\\s*\\((\\w+)\\)");
    regex reg_dec(".in\\s*\\((\\w+),\\s*@(\\w+),\\s*(\\w+)\\)");// offset,type,var
    if (!std::regex_match(line.begin(), line.end(), reg_in)
            &&!std::regex_match(line.begin(), line.end(), reg_dec)) {
        throw_error("Invalid in statement");
    }
    smatch match;
    In* in = new In();
    // extract the field
    if (regex_search(line, match, reg_in)) {
        in->set_offset(match.str(1));
        // TODO: There are more things to do---Need to build the kernel graph.
    }
    else if (regex_search(line, match, reg_dec)) {
        printf("hello!\n");
        in->set_offset(match.str(1));
        in->set_dec(true);
        in->set_type(match.str(2));
        in->set_var(match.str(3));
    } 
    else {
        throw_error("Invalid in statement: Could not find the field");
    }

    in->print();
    return in;
}

Iter* Policy::parse_iter(string line) {
    regex reg_fixed("\\.iter\\s*\\(\\w+,\\s*\\d+,\\s*\\w+\\)");
    regex reg_dynamic("\\.iter\\s*\\(\\w+,\\s*\\w+,\\s*\\w+\\)");
    if (!std::regex_match(line.begin(), line.end(), reg_dynamic)
            && !std::regex_match(line.begin(), line.end(), reg_fixed)) {
        throw_error("Invalid iter statement");
    }

    smatch match;
    regex sreg_dynamic("\\.iter\\s*\\((\\w+),\\s*(\\w+),\\s*(\\w+)\\)");
    regex sreg_fixed("\\.iter\\s*\\((\\w+),\\s*(\\d+),\\s*(\\w+)\\)");
    Iter *iter = new Iter();
    if (regex_search(line, match, sreg_fixed)){
        iter->set_offset(match.str(1));
        iter->set_sstep(match.str(2));
        iter->set_size(match.str(3));
        iter->set_dynamic(0);
    }
    else if (regex_search(line, match, sreg_dynamic)) {
        iter->set_offset(match.str(1));
        iter->set_sstep(match.str(2));
        iter->set_size(match.str(3));
        iter->set_dynamic(1);
    } 
    else {
        throw_error("Invalid iter statement");
    }

    iter->print();
    return iter;
}

Values* Policy::parse_values(string line) {
    // regex reg_check("\\.values\\s*\\(\\w+(,\\s*\\w+)*@\\s*(\\w+)\\)");
    regex reg_clone("\\.values\\s*\\(\\w+(,\\s*\\w+)*\\)");
    regex reg_fd("\\.values\\s*\\(@\\w+,\\s*\\w+\\)");
//    regex reg_vs("\\.values\\s*\\(\\w+(,\\s*\\w+)*@\\s*\\w+\\)");
// !std::regex_match(line.begin(), line.end(), reg_check) // we no longer use reg_check
    if (!std::regex_match(line.begin(), line.end(), reg_clone)
            && !std::regex_match(line.begin(), line.end(), reg_fd)) {
        throw_error("Invalid values statement");
    }

    smatch match;
    // regex sreg_check("\\.values\\s*\\((\\w+(,\\s*\\w+)*)@\\s*(\\w+)\\)");
    regex sreg_clone("\\.values\\s*\\((\\w+(,\\s*\\w+)*)\\)");
    regex sreg_fd("\\.values\\s*\\(@(\\w+),\\s*(\\w+)\\)");
    Values *vs = new Values();
    // if (regex_search(line, match, sreg_check)) {
    //     vs->set_smtbit(match.str(3));
    //     vector<string> s_fields = split(match.str(1), ",");
    //     for (string field: s_fields)
    //         vs->add_field(field);
    // }
    if(regex_search(line, match, sreg_clone)){
        //vs->set_smtbit(to_string(0);
        vector<string> s_fields = split(match.str(1), ",");
        for (string field: s_fields)
            vs->add_field(field);
    } else if(regex_search(line, match, sreg_fd)){
        vs->set_name(match.str(1));
        vs->add_field(match.str(2));
    } else {
        throw_error("Invalid values statement");
    }

    vs->print();
    return vs;
}

Asser* Policy::parse_asser(string line) {
//    regex reg_t("\\.traverse\\s*\\((\\w+,\\s*&\\w+\\.\\w+,\\s*\\w+\\.\\w+)\\)");
    regex reg_t("\\.assert\\s*\\((\\s*\\w+,\\s*\\w+)\\)");
    if (!std::regex_match(line.begin(), line.end(), reg_t)) {
        throw_error("Invalid traverse statement");
    }

    smatch match;
    regex sreg_t("\\.assert\\s*\\(\\s*(\\w+),\\s*(\\w+)\\)");
    Asser *asser = new Asser();
    if (regex_search(line, match, sreg_t)) {
        asser->set_high(match.str(1));
        asser->set_low(match.str(2));
    } else {
        throw_error("Invalid assert statement");
    }

    asser->print();
    return asser;
}

void Policy::gen_pgt_walk_aim(){
    cout << "Generating page table walk AIM" << endl;
    // 4 level page table walk
    int i = 0;
    for (i = 0; i< 4; i++){
        ReadLoad * rload = new ReadLoad(0);
        rload->set_post_qpn(this->avail_state);
        this->avail_state++;
        pgt_aims.push_back(rload);
        rload->print();
    }
}

// Iter through OPs for collecting dynamic iter informations
void Policy::mark_iter(){
    cout << "Modifying iter, total " << this->ops.size() << " Checking iter" << endl;
    int seq = 0 + 3 * this->task_nr;  // isolate registers 
    for (int i = 0; i < this->ops.size(); i++){
        if (this->ops.at(i)->get_op_name() == "Iter" ){
            Iter* itr = (Iter *)(this->ops.at(i));
            //printf("i is %d, dynamic is %d, name is \n", i, itr->get_dynamic());
            if (!itr->get_dynamic()){
                cout << "checking const iter" << endl;
                itr->set_seq(seq);
                cout << "const iter is modified" << endl;
                itr->print();
                seq++;
                continue; // const entry number in iter
            }
            // check the dynamically allocated iter
            if  (this->ops.at(i-1)->get_op_name()=="Values"){
                Values* val = (Values *)(this->ops.at(i-1));
                if (val->get_name() == itr->get_sstep()){
                    val->set_regnr(seq);
                    itr->set_seq(seq);
                    cout << "dynamic iter is modified" << endl;
                    val->print();
                    itr->print();
                    seq++;
                    continue;
                }
            }
            else if (this->ops.at(i-2)->get_op_name()=="Values"){
                //cout << " enters here " << endl;
                Values* val = (Values *)(this->ops.at(i-2));
                if (val->get_name() == itr->get_sstep()){
                    //cout << " enters there " << endl;
                    val->set_regnr(seq);
                    itr->set_seq(seq);
                    cout << "dynamic iter is modified" << endl;
                    val->print();
                    itr->print();
                    seq++;
                    continue;
                }
            }
            else {
               throw_error("Cannot find iter fds");
            }
        }
    }
}

void Policy::mark_assert(){
    cout << "Checking out assert logic" << endl;
    for (int i = 0; i < this->ops.size(); i++){
        if (this->ops.at(i)->get_op_name() == "Asser" ){
            Asser* asser = (Asser *)(this->ops.at(i));
            if (i == 0){
                throw_error("Error: empty assert object!");
            }
            if (this->ops.at(i-1)->get_op_name() != "Values"){
                throw_error("Error: wrong assert logic!")
            }
            // marking assert logic
            Values* val = (Values*)(this->ops.at(i-1));
            val->set_smtbit("2");
            val->set_high(asser->get_high());
            val->set_low(asser->get_low());
        }
    }
}

// begin AIM gen
void Policy::frontend_compile(){
    cout << "Start compiling" << endl;
    for (int i = 0; i < this->ops.size(); i++){
        if (this->ops[i]->get_op_name() == "Traverse"){
            this->gen_traverse_aim((Traverse *)(this->ops[i]));
            cout<< green << "Traverse aim gen finished" << reset <<endl;
        }
        if (this->ops[i]->get_op_name() == "KernelGraph"){
            this->gen_kgraph_aim((KernelGraph *)(this->ops[i]));
            cout<< green << "Kernelgraph aim gen finished" << reset <<endl;
        }
        if (this->ops[i]->get_op_name() == "In"){
            this->gen_in_aim((In *)(this->ops[i]));
            cout<< green << "In aim gen finished" << reset <<endl;
        }
        if (this->ops[i]->get_op_name() == "Values"){
            this->gen_values_aim((Values *)(this->ops[i]));
            cout<< green << "Values aim gen finished" << reset <<endl;
        }
        if (this->ops[i]->get_op_name() == "Iter"){
            this->gen_iter_aim((Iter *)(this->ops[i]));
            cout<< green << "Iter aim gen finished" << reset <<endl;
        }
        if (this->ops[i]->get_op_name() == "End"){
            this->gen_end_aim((End *)(this->ops[i]));
            cout<< green << "End aim gen finished" << reset <<endl;
        }
    }
    // merging aims together, print out aim infos
    this->merge_aims();
}

// Adding mapping for dQPN and sQPN: md.qpn will use md.qpn.
void Policy::gen_kgraph_aim(KernelGraph *kg){
    this->cur_pos = 0; // current offset is at 0
    Init* in = new Init();
    //this->cur_end = in; // current exiting point is drop
    this->head_aims.push_back(in); // the first object is for dropping
    
    // set init qpn used for qpn transfer
    in->set_init_qpn(this->qpn_tran(this->avail_state)); // QPN_TRAN

    // set the init_state 
    // in->set_prev_qpn(this->avail_state);

    // the state for connecting kgraph is avail_state
    this->last_state.clear();
    this->last_state.push_back(this->qpn_tran(this->avail_state)); // QPN_TRAN
    this->avail_state++; // init_state occupied by kgraph

    // end state remains the same
}

void Policy::gen_traverse_aim(Traverse *tra){


    int next_qpn, load_rec_qpn, new_qpn;
    int fake_state; // fake state md.QPN for re-enter Move and push
    fake_state = this->fake_state;
    this->fake_state++;

    next_qpn = this->avail_state;
    this->avail_state++;
    load_rec_qpn = this->avail_state;
    this->avail_state++;
    new_qpn = this->avail_state; // qpn for push, avail state remains 

    // generate const move
    ConstMove* cmove = new ConstMove(stoi(tra->get_type()) - this->cur_pos); // move base to next

    // set new cur_pos to the real point  // debug
    this->cur_pos = stoi(tra->get_type()); // where the next is pointing to

    // generate rmove for reading next ptr
    // no need to set QPN tran here given JMP
    ReadMove* rmove = new ReadMove(stoi(tra->get_offset()) - this->cur_pos); // debug
    rmove->set_post_qpn(next_qpn); // set avail state as QPN of rmove
    rmove->add_prev_qpn(fake_state); // use fake state as previous QPN
    // this->avail_state++;

    // generate pop state
    Pop* pop = new Pop();
    pop->add_prev_qpn(fake_state); // use fake state as previous QPN
    pop->set_post_qpn(next_qpn); // use the Move() qpn as the next QPN

    // generate additional load for recirculation
    ReadLoad* rload = new ReadLoad(0); // const load used for recirculation
                                          // no offset
    rload->set_post_qpn(load_rec_qpn);
    rload->add_prev_qpn(this->qpn_tran(next_qpn)); // jmp from Move(next) // QPN_TRAN
    rload->set_tran_qpn(this->end_state.get_qpn()); // the next state of load is the end state
    rload->set_tran_dqpn(this->end_state.get_dqpn()); // same for setting next state

    // generate push 
    Push* push = new Push();

    // put QPN from previous state to move($) and push
    int last_state_count, last_state, last_state_num;
    last_state_num = this->last_state.size();
    for(last_state_count = 0; last_state_count < last_state_num; last_state_count++){
        last_state = this->last_state.at(last_state_count);
        cmove->add_prev_qpn(last_state);
        push->add_prev_qpn(last_state);
    }
    push->add_prev_qpn(this->qpn_tran(next_qpn)); // push from Move(next) // QPN_TRAN
    cmove->set_post_qpn(new_qpn); // cmove into new state
    push->set_post_qpn(new_qpn); // push into new state

    // adding Move into last state
    this->last_state.push_back(this->qpn_tran(next_qpn)); // QPN_TRAN

    // generate negjmp
    NegJump* njump = new NegJump(tra->get_high(), tra->get_low()); // set ending offset
    njump->set_post_qpn(this->qpn_tran(next_qpn)); // prev aim is Move(next), make it post_qpn for invoking
                                                    // QPN_TRAN
    njump->set_true_post_qpn(new_qpn); // jump to push
    njump->set_false_post_qpn(load_rec_qpn); // jump to load recirc
    njump->set_fake_qpn(fake_state); // generate rule for direct tran for jmp

    // add rule for prev end state // cur_end is no longer needed
    //Aim* prev = this->cur_end;
    //prev->add_prev_qpn(load_rec_qpn); // going out of traverse loop

    // set rmove as the cur_end_aim inside the body
    //    this->cur_end = rmove; // no longer needed, replaced with end_state
    this->end_state.set_qpn(fake_state);
    this->end_state.set_dqpn(0);

    // put all aim into aim set
    this->head_aims.push_back(cmove);
    this->head_aims.push_back(push);
    this->tail_aims.push(rload);
    this->tail_aims.push(njump);
    this->tail_aims.push(rmove);
    this->tail_aims.push(pop);

    // end of tra aim gen
}
 
void Policy::gen_iter_aim(Iter *itr){
    int dynamic = itr->get_dynamic(); // const iter or dynamic iter

    int load_rec_qpn, new_qpn;
    int fake_state; // fake state md.QPN for re-enter Move($) and push(iter loop)
    fake_state = this->fake_state;
    this->fake_state++;

    // cout << "debug point 1" << endl;

    load_rec_qpn = this->avail_state;
    this->avail_state++;
    new_qpn = this->avail_state; // qpn for push, move and load, avail state remains

    ConstLoad* cload = new ConstLoad(itr->get_seq());
    if (dynamic == 0){
        cload->set_post_qpn(new_qpn);
        cload->set_value(stoi(itr->get_sstep()));
        cload->set_seq(itr->get_seq()); // set position for storing max_fds
        // need to allocate prev_qpn later
    }

    // generate pop state
    Pop* pop = new Pop();
    pop->add_prev_qpn(fake_state); // use fake state as previous QPN
    pop->set_post_qpn(new_qpn); // only one pop QPN?
    // cout << "debug point 2" << endl;


    // generate move($)
    ConstMove* cmove_array = new ConstMove(stoi(itr->get_size())); // move to next array entry
    // debug 
    cmove_array->add_prev_qpn(fake_state);
    // cmove_array->print();
    cmove_array->set_post_qpn(new_qpn);

    // generate const move to array header
    ConstMove* cmove = new ConstMove(stoi(itr->get_offset()) - this->cur_pos); // move base to next

    // generate push
    Push* push = new Push();

    // cout << "debug point 3" << endl;


    // put QPN from previous state to move($) and push
    int last_state_count, last_state, last_state_num;
    last_state_num = this->last_state.size();
    for(last_state_count = 0; last_state_count < last_state_num; last_state_count++){
        last_state = this->last_state.at(last_state_count);
        cmove->add_prev_qpn(last_state);
        push->add_prev_qpn(last_state);
        if (dynamic == 0){
            cload->add_prev_qpn(last_state);
        }
    }
    push->add_prev_qpn(fake_state); // push from iter body
    cmove->set_post_qpn(new_qpn); // cmove into new state
    push->set_post_qpn(new_qpn); // push into new state

    // adding fake_state into last state
    this->last_state.push_back(fake_state);

    // generate additional load for recirculation
    ReadLoad* rload = new ReadLoad(0); // const load used for recirculation
                                          // no offset
    rload->set_post_qpn(load_rec_qpn);
    rload->add_prev_qpn(fake_state); // jmp from Move(next)
    rload->set_tran_qpn(this->end_state.get_qpn()); // the next state of load is the end state
    rload->set_tran_dqpn(this->end_state.get_dqpn()); // same for setting next state

    // generate decjmp
    DecJump* djump = new DecJump(itr->get_seq()); // set ending offset
    djump->set_post_qpn(fake_state); // prev qpn set is the fake_state
    djump->set_true_post_qpn(new_qpn); // jump to push
    djump->set_false_post_qpn(load_rec_qpn); // jump to load recirc

    // redirect body move and load to end state
    this->end_state.set_qpn(fake_state);
    this->end_state.set_dqpn(0);

    if (dynamic == 0){
        this->head_aims.push_back(cload);
    }
    this->head_aims.push_back(cmove);
    this->head_aims.push_back(push);
    this->tail_aims.push(rload);
    this->tail_aims.push(djump);
    this->tail_aims.push(cmove_array);
    this->tail_aims.push(pop);
}


void Policy::gen_in_aim(In *in){
    int mov_qpn;

    mov_qpn = this->avail_state;
    this->avail_state++;

    // generate move
    ReadMove* rmove = new ReadMove(stoi(in->get_offset())-this->cur_pos);
    // set qpn to transit to
    // rmove->set_qpn_next();
    
    // set exit point for the move aim
    int end_qpn, end_dqpn;
    end_qpn = this->end_state.get_qpn();
    end_dqpn = this->end_state.get_dqpn();
    rmove->set_qpn_null(end_qpn);
    rmove->set_dqpn_null(end_dqpn);

    // set previous and post qpn for move
    int last_state_count, last_state, last_state_num;
    last_state_num = this->last_state.size();
    for(last_state_count = 0; last_state_count < last_state_num; last_state_count++){
        last_state = this->last_state.at(last_state_count);
        rmove->add_prev_qpn(last_state);
    }
    this->last_state.clear();
    this->last_state.push_back(this->qpn_tran(mov_qpn)); // push move into next aim's last aim // QPN_TRAN

    rmove->set_post_qpn(mov_qpn); // qpn for executing move

    // set cur_pos depending on decoration
    if (in->get_dec()){
        this->cur_pos = stoi(in->get_var());
    }
    else{
        this->cur_pos = 0;
    }

    head_aims.push_back(rmove);
}

void Policy::gen_values_aim(Values *val){
    int val_num = val->get_num();
    int iter_store = val->get_regnr() == (-1)? 0: 1; // check whether this value should be stored 
                                                    // in register or not
    int range_check = stoi(val->get_smtbit()) == (2)? 1: 0; // check whether the value needs to be 
                                                            // asserted or not
    // cout << bold << red << "debug: range check is " << range_check << reset << endl;
    int rd_qpn = this->avail_state;
    this->avail_state++;
    // generate read load
    if (iter_store == 1){ // store the address into register
        ReadLoad* rload = new ReadLoad(stoi(val->fields.at(0))-this->cur_pos);
        rload->set_post_qpn(rd_qpn);
        int last_state_count, last_state, last_state_num;
        last_state_num = this->last_state.size();
        for(last_state_count = 0; last_state_count < last_state_num; last_state_count++){
            last_state = this->last_state.at(last_state_count);
            rload->add_prev_qpn(last_state);
        } // set previous state for move
        this->last_state.clear();
        this->last_state.push_back(this->qpn_tran(rd_qpn)); // QPN_TRAN

        // set register to store address
        rload->set_reg_index(val->get_regnr());

        // no additional transferring needed execpt there is no 
        this->head_aims.push_back(rload);
    }

    else{ // not a internal array traversal variable
        int count = 0;
        for (count = 0; count < val_num; count++){
            rd_qpn = this->avail_state -1; // sequential load primitives concatenated together // debug
            this->avail_state ++;
            // cout << red << rd_qpn << reset << endl;
            ReadLoad* rload = new ReadLoad(stoi(val->fields.at(count))-this->cur_pos);
            rload->set_post_qpn(rd_qpn);
            if (range_check){
                rload->set_range_check(1); // mark as range check readload
                rload->set_high(val->get_high());
                rload->set_low(val->get_low());
            }

            // assign previous qpn one by one
            int last_state_count, last_state, last_state_num;
            last_state_num = this->last_state.size();
            for(last_state_count = 0; last_state_count < last_state_num; last_state_count++){
                last_state = this->last_state.at(last_state_count);
                rload->add_prev_qpn(last_state);
            } // set previous state for move
            this->last_state.clear();
            this->last_state.push_back(this->qpn_tran(rd_qpn));

            this->head_aims.push_back(rload);
        }
        this->avail_state--; // patch values
    }
}

void Policy::gen_end_aim(End* ed){
    // assign exit of the last load onto nearest exiting point
    Aim* last = head_aims.back();
    if (last->get_aim_name() != "ReadLoad"){
        throw_error("policy didn't terminate properly");
    }
    else {
        ((ReadLoad*)last)->set_tran_qpn(this->end_state.get_qpn());
        ((ReadLoad*)last)->set_tran_dqpn(this->end_state.get_dqpn());
    }

}

// begin code gen

// PC transition helper function
string gen_direct_transfer_tab(int qpn, int end_bit, int iter_entry, int dqpn){
    string str = "pd direct_transfer_tab add_entry mod_dqpn ib_aeth_valid 1 md_qpn " + to_string(qpn) + " md_end_bit " 
        + to_string(end_bit) + " md_iter_entry " + to_string(iter_entry) + " action_dqpn " 
        + to_string(dqpn) + '\n';
    return str;
}

string gen_check_null_tab(int qpn, int p_qpn, int dqpn){
    string str = "pd check_null_tab add_entry mod_qpn_dqpn ib_aeth_valid 1 md_qpn " + to_string(qpn) + 
        " md_aeth_addr_h 0x0 md_aeth_addr_l 0x0 action_qpn " +
        to_string(p_qpn) + " action_dqpn " + to_string(dqpn) + '\n';
    return str;
}

string gen_check_traverse_end_tab(int qpn, string addr_h, string addr_l){
    string str = "pd check_traverse_end_tab add_entry set_end_bit ib_aeth_valid 1 md_qpn " + to_string(qpn) + 
        " md_aeth_addr_h 0x" + addr_h + " md_aeth_addr_l 0x" + addr_l + '\n';
    return str;
}

string gen_read_update_max_entry_tab(int qpn, int idx, int act){
    string str;
    switch(act){
        case 1: 
            str = "pd read_update_max_entry_tab add_entry read_const_length ib_aeth_valid 1 md_qpn " + to_string(qpn) +
                " action_idx " + to_string(idx) + '\n';
            break;
        case 2:
            str = "pd read_update_max_entry_tab add_entry read_max_entry ib_aeth_valid 1 md_qpn " + to_string(qpn) +
                " action_idx " + to_string(idx) + '\n';
            break;
        case 3:
            str = "pd read_update_max_entry_tab add_entry update_max_entry ib_aeth_valid 1 md_qpn " + to_string(qpn) +
                " action_idx " + to_string(idx) + '\n';
            break;
    }
    return str;
}

string gen_end_transfer_tab(int qpn, int p_qpn, int dqpn){
    string str = "pd end_transfer_tab add_entry mod_qpn_dqpn ib_aeth_valid 1 md_qpn " + to_string(qpn) 
        + " action_qpn " + to_string(p_qpn) + " action_dqpn " + to_string(dqpn) + '\n';
    return str;
}

// cloning tab helper function
string gen_cloning_tab(int qpn){
    string str = "pd cloning_tab add_entry cloning ib_aeth_valid 1 md_qpn " + to_string(qpn) + '\n';
    return str;
}

int Policy::find_next_post_qpn(int i){
    int j = 0;
    int post_qpn = 0;
    for (j = i+1; j < this->all_aims.size(); j++){
        if (this->all_aims[j]->get_aim_name() == "ReadMove"){
            post_qpn = ((ReadMove *)(this->all_aims[j]))->get_post_qpn();
            break;
        }
        if (this->all_aims[j]->get_aim_name() == "ReadLoad"){
            post_qpn = ((ReadLoad *)(this->all_aims[j]))->get_post_qpn();
            break;
        }
        if (this->all_aims[j]->get_aim_name() == "DecJump" or (this->all_aims[j]->get_aim_name() == "NegJump")){
            post_qpn = -1; // use end transfer tab
            break;
        }
    }
    return post_qpn;
}

// PC transtition main function
string Policy::gen_pc_tran(){
    cout << "Start generating QPN transition rule" << endl;
    string trans;
    for (int i = 0; i < this->all_aims.size(); i++){
        Aim* it = this->all_aims[i];
        if (it->get_aim_name() == "Init"){
            // generate first transition from init to next move/load
            int post_qpn = this->find_next_post_qpn(i);
            // if (post == -1)
            trans += gen_end_transfer_tab(((Init*)it)->get_init_qpn(), ((Init*)it)->get_init_qpn(), post_qpn);
        }
        if (it->get_aim_name() == "ReadLoad"){
            if (((ReadLoad *)it)->get_tran_qpn() != -1){ // last load statement, use end trans
                trans += gen_end_transfer_tab(this->qpn_tran(((ReadLoad *)it)->get_post_qpn()), 
                    ((ReadLoad *)it)->get_tran_qpn(), ((ReadLoad *)it)->get_tran_dqpn()); // QPN_TRAN
            }
            else {
                int post_qpn = this->find_next_post_qpn(i); // Jmp will be covered in the first case
                trans += gen_end_transfer_tab(this->qpn_tran(((ReadLoad*)it)->get_post_qpn()), 
                     this->qpn_tran(((ReadLoad*)it)->get_post_qpn()), post_qpn); // QPN_TRAN
            }
        }
        if (it->get_aim_name() == "ReadMove"){
            if (((ReadMove *)it)->get_qpn_null() != -1){ // not last Move before Jmp
                trans += gen_check_null_tab(this->qpn_tran(((ReadMove *)it)->get_post_qpn()), // QPN_TRAN 
                    ((ReadMove *)it)->get_qpn_null(), ((ReadMove *)it)->get_dqpn_null()); // checking the NULL criteria
                // if base is not Null
                int post_qpn = this->find_next_post_qpn(i); // Jmp will not be covered
                trans += gen_direct_transfer_tab(this->qpn_tran(((ReadMove *)it)->get_post_qpn()),
                     0, 0, post_qpn); // QPN_TRAN
            }
            else {
                // indicate this is a Traverse Mov, no action
            }
        }
        if (it->get_aim_name() == "NegJump"){ // No need to use QPN_TRAN
            trans += gen_check_traverse_end_tab(((NegJump *)it)->get_post_qpn(), ((NegJump *)it)->get_addr_h(), 
                ((NegJump *)it)->get_addr_l());
            trans += gen_direct_transfer_tab(((NegJump *)it)->get_post_qpn(), 0, 0,
                ((NegJump *)it)->get_true_post_qpn());  // traverse ends here
            trans += gen_direct_transfer_tab(((NegJump *)it)->get_post_qpn(), 1, 0,
                ((NegJump *)it)->get_false_post_qpn());  // traverse is not ended
            trans += gen_direct_transfer_tab(((NegJump *)it)->get_fake_qpn(), 0, 0, 
                this->qpn_rtran(((NegJump *)it)->get_post_qpn())); // last trans table transfer from fake state to move state
                                                  // QPN_TRAN
        }
        if (it->get_aim_name() == "DecJump"){ // No need to use QPN_TRAN
            trans += gen_read_update_max_entry_tab(((DecJump *)it)->get_post_qpn(), ((DecJump *)it)->get_reg_idx(),
                3); // update max entry. If reg_lo != 1, md.iter_end = 2; if reg_lo == 1, md.iter_end = 1;
            trans += gen_direct_transfer_tab(((DecJump *)it)->get_post_qpn(), 0, 1, 
                ((DecJump *)it)->get_false_post_qpn());
            trans += gen_direct_transfer_tab(((DecJump *)it)->get_post_qpn(), 0, 2, 
                ((DecJump *)it)->get_true_post_qpn());
        }
    }
    return trans;
}

string Policy::backend_compile(){
    cout << "Start backend compiling" << endl;
    string code;
    for (int i = 0; i < this->all_aims.size(); i++){
        if (this->all_aims[i]->get_aim_name() == "Init"){
            code += this->gen_init_code((Init *)(this->all_aims[i]));
            cout<< blue << "Initialization code gen finished" << reset <<endl;
        }
        if (this->all_aims[i]->get_aim_name() == "ConstLoad"){
            code += this->gen_constload_code((ConstLoad *)(this->all_aims[i]));
            cout<< blue << "Const load(Load($)) code gen finished" << reset <<endl;
        }
        if (this->all_aims[i]->get_aim_name() == "ConstMove"){
            code += this->gen_constmove_code((ConstMove *)(this->all_aims[i]));
            cout<< blue << "Const move(Move($)) code gen finished" << reset <<endl;
        }
        if (this->all_aims[i]->get_aim_name() == "ReadLoad"){
            code += this->gen_readload_code((ReadLoad *)(this->all_aims[i]));
            cout<< blue << "Read load(Load()) code gen finished" << reset <<endl;
        }
        if (this->all_aims[i]->get_aim_name() == "ReadMove"){
            code += this->gen_readmove_code((ReadMove *)(this->all_aims[i]));
            cout<< blue << "Read move(Move())) code gen finished" << reset <<endl;
        }
        if (this->all_aims[i]->get_aim_name() == "Push"){
            code += this->gen_push_code((Push *)(this->all_aims[i]));
            cout<< blue << "Push code gen finished" << reset <<endl;
        }
        if (this->all_aims[i]->get_aim_name() == "Pop"){
            code += this->gen_pop_code((Pop *)(this->all_aims[i]));
            cout<< blue << "Pop code gen finished" << reset <<endl;
        }
        if (this->all_aims[i]->get_aim_name() == "DecJump"){
            code += this->gen_decjump_code((DecJump *)(this->all_aims[i]));
            cout<< blue << "DecJump(R0--, L1, L2) code gen finished" << reset <<endl;
        }
        if (this->all_aims[i]->get_aim_name() == "NegJump"){
            code += this->gen_negjump_code((NegJump *)(this->all_aims[i]));
            cout<< blue << "NegJump(base == Addr, L1, L2) code gen finished" << reset <<endl;
        }
    }
    return code;
}

// end of fetching helper function
string gen_end_of_fetching_tab(int qpn){
    string str;
    str += "pd end_of_fetching_tab add_entry _drop ib_aeth_valid 1 ib_bth_dqpn " + to_string(qpn) + '\n';
    return str;
}

// Cache/Read/Modiify base address helper function
string gen_cache_process_addr_to_reg_h_tab(int qpn, int dqpn, int idx, int act){ // act is the action NO
    string str;
    switch(act){
        case 1: // read: read_process_addr_from_reg_h
            str += "pd cache_process_addr_to_reg_h_tab add_entry read_process_addr_from_reg_h ib_aeth_valid 1 md_qpn "
                + to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) 
                + " eg_intr_md_from_parser_aux_clone_src 0"  // handling egress clone
                + " action_state " + to_string(idx) + '\n';
            break;
        case 2: // write: write_process_addr_to_reg_h
            str += "pd cache_process_addr_to_reg_h_tab add_entry write_process_addr_to_reg_h ib_aeth_valid 1 md_qpn "
                + to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) 
                + " eg_intr_md_from_parser_aux_clone_src 0"  // handling egress clone
                + " action_state " + to_string(idx) + '\n';
            break;
    }
    return str;
}

string gen_cache_process_addr_to_reg_l_tab(int qpn, int dqpn, int idx, int act){ // act is the action NO
    string str;
    switch(act){
        case 1: // read: read_process_addr_from_reg_l
            str += "pd cache_process_addr_to_reg_l_tab add_entry read_process_addr_from_reg_l ib_aeth_valid 1 md_qpn "
                + to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) 
                + " eg_intr_md_from_parser_aux_clone_src 0"  // handling egress clone
                + " action_state " + to_string(idx) + '\n';
            break;
        case 2: // write: write_process_addr_to_reg_l
            str += "pd cache_process_addr_to_reg_l_tab add_entry write_process_addr_to_reg_l ib_aeth_valid 1 md_qpn "
                + to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) 
                + " eg_intr_md_from_parser_aux_clone_src 0"  // handling egress clone
                + " action_state " + to_string(idx) + '\n';
            break;
        case 3: // modify: read_update_iter_addr_l
            str += "pd cache_process_addr_to_reg_l_tab add_entry read_update_iter_addr_l ib_aeth_valid 1 md_qpn "
                + to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) 
                + " eg_intr_md_from_parser_aux_clone_src 0"  // handling egress clone
                + " action_state " + to_string(idx) + '\n';
            break;
    }
    return str;
}

// Cache array size/length tab helper function
string gen_cache_size_into_md_tab(int qpn, int size){
    string str;
    str += "pd cache_size_into_md_tab add_entry cache_size_into_md md_qpn " + to_string(qpn) + 
        " action_entry_size " + to_string(size) + '\n';
    return str;
}
string gen_cache_len_into_md_tab(int qpn, int len){
    string str;
    str += "pd cache_len_into_md_tab add_entry cache_len_into_md md_qpn " + to_string(qpn) +
        " action_max_len " + to_string(len) + '\n';
    return str;
}
// Mod offset pre table helper function
string gen_mod_field_parameters_pre_tab(int qpn, int dqpn, int offset){
    string str;
    if (offset < 0){
        str += "pd encode_mod_offset_pre_tab add_entry encode_mod_offset ib_aeth_valid 1 md_qpn " + 
            to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) + " action_offset " + to_string(-offset) + '\n';
        str += "pd mod_field_parameters_pre_tab add_entry mod_field_parameters_subtract ib_aeth_valid 1 md_qpn " +
            to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) + '\n';
    }
    else {
        str += "pd encode_mod_offset_pre_tab add_entry encode_mod_offset ib_aeth_valid 1 md_qpn " + 
            to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) + " action_offset " + to_string(offset) + '\n';
        str += "pd mod_field_parameters_pre_tab add_entry mod_field_parameters_add ib_aeth_valid 1 md_qpn " +
            to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) + '\n';
    }
    return str;
}

// PSN table mapping helper function
string gen_read_update_psn_tab(int dqpn, int idx){
    string str;
    str += "pd read_update_psn_tab add_entry read_update_psn ib_aeth_valid 1 ib_bth_dqpn " + 
        to_string(dqpn) + " eg_intr_md_from_parser_aux_clone_src 0" + " action_state " + to_string(idx) + '\n';
    return str;
}

// PSN table for defense spoof injection
string gen_read_update_psn_def_tab(int dqpn, int idx){
    string str;
    // This can be enabled for defending spoofing disable
    //str += "pd read_update_psn_def_tab add_entry read_update_psn_def ib_aeth_valid 1 ib_bth_dqpn " +
        to_string(dqpn) + " action_state " + to_string(idx) + '\n';
    return str;
}


// mod offset helper function
string gen_mod_field_parameters_tab(int qpn, int dqpn, int offset){
    string str;
    if (offset < 0){
        str += "pd encode_mod_offset_tab add_entry encode_mod_offset ib_aeth_valid 1 md_qpn " + 
            to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) + " eg_intr_md_from_parser_aux_clone_src 0" +
             " action_offset " + to_string(-offset) + '\n';
        str += "pd mod_field_parameters_tab add_entry mod_field_parameters_subtract ib_aeth_valid 1 md_qpn " +
            to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) + " eg_intr_md_from_parser_aux_clone_src 0"
            + " action_len " + to_string(8) + '\n'; // set len as 8 for now
    }
    else {
        str += "pd encode_mod_offset_tab add_entry encode_mod_offset ib_aeth_valid 1 md_qpn " + 
            to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) + " eg_intr_md_from_parser_aux_clone_src 0" +
             " action_offset " + to_string(offset) + '\n';
        str += "pd mod_field_parameters_tab add_entry mod_field_parameters_add ib_aeth_valid 1 md_qpn " +
            to_string(qpn) + " ib_bth_dqpn " + to_string(dqpn) + " eg_intr_md_from_parser_aux_clone_src 0"
            + " action_len " + to_string(8) + '\n'; // set len as 8 for now
    }
    return str;
}

// Move must cache address: find next post QPN and JMP
// Load/Move will only read after Load --> 
//      norm load: find post; end load cmove(push): no action; end load rmove(read); end load empty: no action.
// Cmove after pop will result in modify base, otherwise mod_para_pre. Cmove needs to load constant as well.
// Push/Pop semantics:
string Policy::gen_base_operation(){
    cout << "Start generating base regitser operation rules" << endl;
    this->base_idx = 0 + 15 * this->task_nr; // initilizing base array // multi_task
    this->stack_top = -1 + 15 * this->task_nr; // init stack_depth = 0 // multi_task
    string str;
    for (int i = 0; i < this->all_aims.size(); i++){
        Aim* it = this->all_aims[i];
        if (it->get_aim_name() == "Init"){ // similar with Move(). Init will not change base_idx
            int post_qpn = this->find_next_post_qpn(i);
            str += gen_cache_process_addr_to_reg_h_tab(((Init*)it)->get_init_qpn(), 
                post_qpn, this->stack_top + 1, 2); // write reg
            str += gen_cache_process_addr_to_reg_l_tab(((Init*)it)->get_init_qpn(), 
                post_qpn, this->stack_top + 1, 2); // write reg
        }
        if (it->get_aim_name() == "ReadMove"){
            this->base_idx = this->stack_top + 1; // store new address to stack_top + 1
                                                  // base after rmove() will be read from there.
            if (((ReadMove *)it)->get_qpn_null() != -1){ // not last Move before Jmp 
                int post_qpn = this->find_next_post_qpn(i); // Jmp will not be covered
                str += gen_cache_process_addr_to_reg_h_tab(this->qpn_tran(((ReadMove *)it)->get_post_qpn()), 
                    post_qpn, this->stack_top + 1, 2); // write reg_h // QPN_TRAN
                str += gen_cache_process_addr_to_reg_l_tab(this->qpn_tran(((ReadMove *)it)->get_post_qpn()), 
                    post_qpn, this->stack_top + 1, 2); // write reg_l // QPN_TRAN
            }   
            else{ // this is a jmp after move
                // find next jmp
                int j;
                for (j = i+1; j < this->all_aims.size(); j++){
                    if (this->all_aims[j]->get_aim_name() == "NegJump"){
                        NegJump * njump = (NegJump *)(this->all_aims[j]);
                        str += gen_cache_process_addr_to_reg_h_tab(this->qpn_tran(((ReadMove *)it)->get_post_qpn()), 
                            njump->get_true_post_qpn(), this->stack_top + 1, 2); // write reg_h // QPN_TRAN
                        str += gen_cache_process_addr_to_reg_l_tab(this->qpn_tran(((ReadMove *)it)->get_post_qpn()),
                            njump->get_true_post_qpn(), this->stack_top + 1, 2); // write reg_l // QPN_TRAN
                        //str += "hello\n"; //debug
                        str += gen_cache_process_addr_to_reg_h_tab(this->qpn_tran(((ReadMove *)it)->get_post_qpn()),
                            njump->get_false_post_qpn(), this->stack_top + 1, 2); // write reg_h // QPN_TRAN
                        str += gen_cache_process_addr_to_reg_l_tab(this->qpn_tran(((ReadMove *)it)->get_post_qpn()),
                            njump->get_false_post_qpn(), this->stack_top + 1, 2); // write reg_l // QPN_TRAN
                        break;
                    }
                }
            } 
        } // end of rmove
        if (it->get_aim_name() == "ReadLoad"){
            if (((ReadLoad *)it)->get_tran_qpn() != -1){ // last load statement, use end trans
                int  j;
                for (j = i+1; j < this->all_aims.size(); j++){
                    if (this->all_aims[j]->get_aim_name() == "ConstMove"){
                        // if cmove, no action. Empty is included
                        break;
                    }
                    if (this->all_aims[j]->get_aim_name() == "ReadMove"){
                        // if rmove, read. Should use fake qpn
                        // int post_qpn = this->find_next_post_qpn(i);
                        ReadMove * rmove = (ReadMove *)(this->all_aims[j]);
                        int post_qpn = rmove->get_post_qpn();
                        int ssize = rmove->get_prev_qpn_size();
                        if (ssize != 1){
                            throw_error("Move in traverse should only have 1 QPN!");
                        }
                        int prev_qpn = rmove->get_prev_qpn().at(0);
                        
                        str += gen_cache_process_addr_to_reg_h_tab(prev_qpn, post_qpn, this->stack_top, 1);   // temperor fix: pop indication  // more fix
                        str += gen_cache_process_addr_to_reg_l_tab(prev_qpn, post_qpn, this->stack_top, 1);  // temperor fix: pop indication  // more fix
                        break;
                    }
                }
            }
            else { // this load is not the last primitive
                if (this->all_aims[i+1]->get_aim_name() != "ConstMove"){
                    int post_qpn = this->find_next_post_qpn(i); // Jmp will be covered in the first case
                    str += gen_cache_process_addr_to_reg_h_tab(this->qpn_tran(((ReadLoad*)it)->get_post_qpn()), post_qpn, 
                    this->base_idx, 1); // read reg // QPN_TRAN
                    str += gen_cache_process_addr_to_reg_l_tab(this->qpn_tran(((ReadLoad*)it)->get_post_qpn()), post_qpn, 
                    this->base_idx, 1); // read reg // QPN_TRAN
                }
            }
        } // end of rload
        if (it->get_aim_name() == "ConstMove"){
            if (i == 0){
                throw_error("ERROR: ConstMove is placed at top!");
            }
            if (this->all_aims[i-1]->get_aim_name() == "Pop"){
                //int count;
                // for (count = 0; count < ((ConstMove *)it)->get_prev_qpn().size(); count++){
                //     str += gen_cache_process_addr_to_reg_h_tab(((ConstMove *)it)->get_prev_qpn().at(count), 
                //         ((ConstMove *)it)->get_post_qpn(), this->base_idx, 1);  // read reg
                //     str += gen_cache_process_addr_to_reg_l_tab(((ConstMove *)it)->get_prev_qpn().at(count), 
                //         ((ConstMove *)it)->get_post_qpn(), this->base_idx, 3); // mod reg
                //     // gen load constant tab
                //     str += gen_cache_parameter_into_md_tab((((ConstMove *)it)->get_prev_qpn().at(count)), 
                //         ((ConstMove *)it)->get_offset(), 0); // max_len is fine
                // }
                ConstMove * cmove = (ConstMove *)it;
                int ssize = cmove->get_prev_qpns_size();
                if (ssize != 1){
                    throw_error("there should be only 1 ending QPN in iter!");
                }
                int j;
                for (j = i+1; j < this->all_aims.size(); j++){
                    if (this->all_aims[j]->get_aim_name() == "DecJump"){
                        DecJump * djump = (DecJump *)(this->all_aims[j]);
                        str += gen_cache_process_addr_to_reg_h_tab(((ConstMove *)it)->get_prev_qpn().at(0), 
                            djump->get_false_post_qpn(), this->base_idx, 1); // read reg
                        str += gen_cache_process_addr_to_reg_l_tab(((ConstMove *)it)->get_prev_qpn().at(0), 
                            djump->get_false_post_qpn(), this->base_idx, 1); // read reg
                        str += gen_cache_process_addr_to_reg_h_tab(((ConstMove *)it)->get_prev_qpn().at(0), 
                            djump->get_true_post_qpn(), this->base_idx, 1); // read reg
                        str += gen_cache_process_addr_to_reg_l_tab(((ConstMove *)it)->get_prev_qpn().at(0), 
                            djump->get_true_post_qpn(), this->base_idx, 3); // mod reg
                        str += gen_cache_size_into_md_tab((((ConstMove *)it)->get_prev_qpn().at(0)), 
                         ((ConstMove *)it)->get_offset()); // max_len is fine
                    }
                    break;
                }
            }
            // patching, if load is infront of constmove, modify register directly.
            else if (this->all_aims[i-1]->get_aim_name() == "ReadLoad"){
                // handle special situation, semantically there must be a move already
                str += gen_cache_process_addr_to_reg_h_tab(((ConstMove *)it)->get_prev_qpn().at(0), 
                    ((ConstMove *)it)->get_post_qpn(), this->base_idx, 1); // read reg
                str += gen_cache_process_addr_to_reg_l_tab(((ConstMove *)it)->get_prev_qpn().at(0), 
                    ((ConstMove *)it)->get_post_qpn(), this->base_idx, 3);
                // gen rule for encoding offset
                str += gen_cache_size_into_md_tab(((ConstMove *)it)->get_prev_qpn().at(0), 
                    ((ConstMove *)it)->get_offset());
            }
            else { // add mod_para table for cmove
                int count;
                for (count = 0; count < ((ConstMove *)it)->get_prev_qpn().size(); count++){
                    str += gen_mod_field_parameters_pre_tab(((ConstMove *)it)->get_prev_qpn().at(count), 
                        ((ConstMove *)it)->get_post_qpn(), ((ConstMove *)it)->get_offset()); // direct add offset
                }
            }
        } // end of cmove
        if (it->get_aim_name() == "Push"){
            this->stack_top++; // base_idx remains the same
        }
        if (it->get_aim_name() == "Pop"){
            this->stack_top--;
            if (this->stack_top <= -2){
                throw_error("Invalid pop behavior!");
            }
            base_idx = this->stack_top + 1;
        }
    }
    return str;
}

//// cache timestamp
//string gen_read_update_ts_start_tab(int qpn, int idx){
//    string str;
//    str += "pd read_update_ts_start_tab add_entry read_update_ts_start ib_aeth_valid 1 ib_bth_dqpn " + to_string(qpn) +
//        " action_idx " + to_string(idx) + '\n';
////    str += "pd read_update_ts_start_tab add_entry read_update_ts_start ib_aeth_valid 1 md_qpn " + to_string(qpn) + " md_sign 0" + '\n';
//    return str;
//}
string gen_read_update_ts_start_tab(int qpn){
    string str;
    str += "pd read_update_ts_start_tab add_entry read_update_ts_start ib_aeth_valid 1 ib_bth_dqpn " + to_string(qpn) + '\n';
//    str += "pd read_update_ts_start_tab add_entry read_update_ts_start ib_aeth_valid 1 md_qpn " + to_string(qpn) + " md_sign 0" + '\n';
    return str;
}

string gen_read_update_toggle_start_tab(int qpn){
    string str;
    str += "pd read_update_toggle_start_tab add_entry read_update_toggle_start ib_aeth_valid 1 md_qpn " + to_string(qpn) + '\n';
    return str;
}


string Policy::gen_psn_mapping(){
    cout << "Start generating PSN mapping" << endl;
    string code;
    for (int i = 0; i < this->all_aims.size(); i++){
        if (this->all_aims[i]->get_aim_name() == "ReadLoad"){
            ReadLoad * rload = (ReadLoad *)(this->all_aims[i]);
            code += gen_read_update_psn_tab(rload->get_post_qpn(), rload->get_post_qpn() - this->base_state);
            code += gen_read_update_psn_def_tab(this->qpn_tran(rload->get_post_qpn()), rload->get_post_qpn() - this->base_state);
            // caching timestapm
            // temperary disable
            //code += gen_read_update_ts_start_tab(this->qpn_tran(rload->get_post_qpn()), rload->get_post_qpn() - this->base_state);
        }
        if (this->all_aims[i]->get_aim_name() == "ReadMove"){
            ReadMove * rmove = (ReadMove *)(this->all_aims[i]);
            code += gen_read_update_psn_tab(rmove->get_post_qpn(), rmove->get_post_qpn() - this->base_state);
            code += gen_read_update_psn_def_tab(this->qpn_tran(rmove->get_post_qpn()), rmove->get_post_qpn() - this->base_state);
            // temperary disable
            //code += gen_read_update_ts_start_tab(this->qpn_tran(rmove->get_post_qpn()), rmove->get_post_qpn() - this->base_state);
        }
    }
    return code;
}

string Policy::gen_offset_encoding(){
    cout << "Start encoding offsets" << endl;
    string code;
    for (int i = 0; i < this->all_aims.size(); i++){
        if (this->all_aims[i]->get_aim_name() == "ReadLoad"){
            ReadLoad * rload = (ReadLoad *)(this->all_aims[i]);
            int j = 0; 
            for (j = 0;  j < rload->get_prev_qpn_size(); j++){
                code += gen_mod_field_parameters_tab(rload->get_prev_qpn().at(j), rload->get_post_qpn(), 
                    rload->get_offset());
            }
        }
        if (this->all_aims[i]->get_aim_name() == "ReadMove"){
            ReadMove * rmove = (ReadMove *)(this->all_aims[i]);
            int j = 0; 
            for (j = 0;  j < rmove->get_prev_qpn_size(); j++){
                code += gen_mod_field_parameters_tab(rmove->get_prev_qpn().at(j), rmove->get_post_qpn(), 
                    rmove->get_offset());
            }
        }
    }
    return code;
}

string Policy::gen_load_max(){
    cout << "Start encoding max entry loading" << endl;
    string str;
    for (int i = 0; i < this->all_aims.size(); i++){
        if (this->all_aims[i]->get_aim_name() == "ReadLoad"){
            ReadLoad * rload = (ReadLoad *)(this->all_aims[i]);
            if (rload->get_reg_index() == -1){
                // no action
            }
            else { // use rload post QPN as key for loading
                str += gen_read_update_max_entry_tab(this->qpn_tran(rload->get_post_qpn()), 
                    rload->get_reg_index(), 2); // load aeth // QPN_TRAN
            }
        }
        if (this->all_aims[i]->get_aim_name() == "ConstLoad"){ // 1. update md.max_len; 2. store into cload->seq
            ConstLoad * cload = (ConstLoad *)(this->all_aims[i]);
            int j = 0;
            for(j = 0; j< cload->get_prev_qpn().size(); j++){
            str += gen_cache_len_into_md_tab(cload->get_prev_qpn().at(j), cload->get_value()); // size can be 0
            str += gen_read_update_max_entry_tab(cload->get_prev_qpn().at(j), cload->get_seq(), 1); // const length
            }
        }
    }
    return str;
}

string Policy::gen_init_code(Init * in){
    in->set_post_qpn(999 - this->task_nr);
    string str = gen_end_of_fetching_tab(in->get_post_qpn()); // set dropping table
    // By default, kgraph is the first base(like a In()/Move())
    // which means address should be put into reg

    // cache time stamp in switch
    // removed for now
      str += gen_read_update_ts_start_tab(in->get_init_qpn());
//    str += gen_read_update_toggle_start_tab(in->get_init_qpn());

    return str;
}

string Policy::gen_constload_code(ConstLoad * cload){ 
    string str;
    return str;
}


// range match helper function
string gen_range_match_tab(int qpn, int range_1, int range_2){
    string str;
    str += "pd range_match_tab add_entry mark_range_k2 ib_aeth_valid 1 md_qpn " + to_string(qpn) + 
        " md_addr_h_16_start " + to_string(range_1) + " md_addr_h_16_end " + to_string(range_2) + " priority 0\n";
    return str;
}

string gen_exact_match_tab(int qpn, string addr_h, string range_1, string range_2){
    string str;
    str += "pd exact_match_tab add_entry mark_range_k1 ib_aeth_valid 1 md_qpn " + to_string(qpn) + 
        " md_addr_h_16 0x" + addr_h + " md_addr_l_16_start 0x" + range_1 + " md_addr_l_16_end 0x" + range_2 +
        " priority 0\n";  
    return str;
}

string gen_gen_mali_alarm_tab(int dqpn){
    string str;
    str += "pd gen_mali_alarm_tab add_entry gen_mali_alarm ib_bth_dqpn " + to_string(dqpn) + " md_k1 0 md_k2 0 " + 
        "eg_intr_md_from_parser_aux_clone_src 1\n";
    return str;
}

// string gen_gen_range_digest_tab(int qpn){
//     string str;
//     str += "pd gen_range_digest_tab add_entry gen_range_digest ib_aeth_valid 1 md_qpn " + to_string(qpn) +
//         " md_k1 0 md_k2 0\n";
//     return str;
// }

string Policy::gen_readload_code(ReadLoad * rload){
    // return/log the readload result with clone tab
    // range check the result if rload->get_range_check == 1
    string str;
    str += gen_cloning_tab(this->qpn_tran(rload->get_post_qpn()));
    if (rload->get_range_check() == 1){ // need to range check the result
        // todo need to add range check content
        string high_prev = rload->get_high_prev();
        string high_post = rload->get_high_post();
        string low_prev = rload->get_low_prev();
        string low_post = rload->get_low_post();
        str += gen_exact_match_tab(this->qpn_tran(rload->get_post_qpn()), high_prev, "0", high_post);
        str += gen_exact_match_tab(this->qpn_tran(rload->get_post_qpn()), low_prev, low_post, "0xffff");
        str += gen_range_match_tab(this->qpn_tran(rload->get_post_qpn()), stoi(low_prev, 0, 16) + 1, 
            stoi(high_prev, 0, 16) - 1);
        // str += gen_gen_range_digest_tab(this->qpn_tran(rload->get_post_qpn()));
        str += gen_gen_mali_alarm_tab(this->qpn_tran(rload->get_post_qpn()));
    }
    return str;
}

string Policy::gen_decjump_code(DecJump * djump){
    string str;
    return str;
}

string Policy::gen_negjump_code(NegJump * njump){
    string str;
    return str;
}

string Policy::gen_pop_code(Pop * pop){
    string str;
    return str;
}

string Policy::gen_push_code(Push * push){
    string str;
    return str;
}

string Policy::gen_readmove_code(ReadMove * rmove){
    string str;
    return str;
}

string Policy::gen_constmove_code(ConstMove * cmove){
    string str;
    return str;
}

// page table walk helper function
// gen page table walk address check table
string gen_mark_vmalloc_bit_p1_tab(int qpn){
    string str;
    str += "pd mark_vmalloc_bit_p1_tab add_entry mark_addr_type ib_aeth_valid 1 md_qpn " + to_string(qpn) +
        " md_aeth_addr_h 0xffffffff md_aeth_addr_h_mask 0xffffffff priority 10 action_tp 1\n";
    str += "pd mark_vmalloc_bit_p1_tab add_entry mark_addr_type ib_aeth_valid 1 md_qpn " + to_string(qpn) +
        " md_aeth_addr_h 0xffff0000 md_aeth_addr_h_mask 0xffff0000 priority 100 action_tp 2\n";
    return str;
}
string gen_mark_vmalloc_bit_p2_tab(){ // stored in qpn_ts
    string str;
    return str;
}

// gen sig page table walk end table
string gen_mark_walking_bit_tab(int dqpn){
    string str;
    str += "pd mark_walking_bit_tab add_entry mark_walking_bit ib_aeth_valid 1 ib_bth_dqpn " +
        to_string(dqpn) + '\n';
    return str;
}

//gen pgt transfer table
string gen_pgt_transfer_tab(int qpn, int dqpn, int vmalloc_bit){
    string str;
    str += "pd pgt_transfer_tab add_entry pgt_mod_dqpn ib_aeth_valid 1 md_qpn " + to_string(qpn) + " md_vmalloc_bit " +
        to_string(vmalloc_bit) + " action_dqpn " + to_string(dqpn) + " action_qpn " + to_string(dqpn) + '\n';
    return str;
}

// gen QPN/dQPN caching table
string gen_cache_dqpn_page_walk_tab(int qpn, int vmalloc_bit, int walking_bit, int idx, int act){
    string str;
    switch(act){
        case 1: // read
            str += "pd cache_dqpn_page_walk_tab add_entry read_dqpn_page_walk ib_aeth_valid 1 md_qpn " +
                to_string(qpn) + " md_vmalloc_bit " + to_string(vmalloc_bit) + " md_walking_bit " +
                to_string(walking_bit) + " action_idx " + to_string(idx) + '\n';
            break;
        case 2: // cache
            str += "pd cache_dqpn_page_walk_tab add_entry cache_dqpn_page_walk ib_aeth_valid 1 md_qpn " +
                to_string(qpn) + " md_vmalloc_bit " + to_string(vmalloc_bit) + " md_walking_bit " +
                to_string(walking_bit) + " action_idx " + to_string(idx) + '\n';
            break;
    }
    return str;
}
string gen_cache_qpn_page_walk_tab(int qpn, int vmalloc_bit, int walking_bit, int idx, int act){
    string str;
    switch(act){
        case 1: // read
            str += "pd cache_qpn_page_walk_tab add_entry read_qpn_page_walk ib_aeth_valid 1 md_qpn " +
                to_string(qpn) + " md_vmalloc_bit " + to_string(vmalloc_bit) + " md_walking_bit " +
                to_string(walking_bit) + " action_idx " + to_string(idx) + '\n';
            break;
        case 2: // cache
            str += "pd cache_qpn_page_walk_tab add_entry cache_qpn_page_walk ib_aeth_valid 1 md_qpn " +
                to_string(qpn) + " md_vmalloc_bit " + to_string(vmalloc_bit) + " md_walking_bit " +
                to_string(walking_bit) + " action_idx " + to_string(idx) + '\n';
            break;
    }
    return str;
}
// gen base addr caching table
string gen_cache_process_page_addr_to_reg_h_tab(int qpn, int walking_bit, int idx,int act){
    string str;
    switch(act){
        case 1: // read
            str += "pd cache_process_page_addr_to_reg_h_tab add_entry read_process_page_addr_to_reg_h ib_aeth_valid 1 md_walking_bit "
                + to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0" 
                  " action_idx " + to_string(idx) + '\n'; 
            break;
        case 2: // cache
            str += "pd cache_process_page_addr_to_reg_h_tab add_entry cache_process_page_addr_to_reg_h ib_aeth_valid 1 md_walking_bit "
                + to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0"
                + " action_idx " + to_string(idx) + '\n';  
            break;
    }
    return str;
}
string gen_cache_process_page_addr_to_reg_l_tab(int qpn, int walking_bit, int idx, int act){
    string str;
    switch(act){
        case 1: // read
            str += "pd cache_process_page_addr_to_reg_l_tab add_entry read_process_page_addr_to_reg_l ib_aeth_valid 1 md_walking_bit "
                + to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0" 
                  " action_idx " + to_string(idx) + '\n';
            break;
        case 2:
            str += "pd cache_process_page_addr_to_reg_l_tab add_entry cache_process_page_addr_to_reg_l ib_aeth_valid 1 md_walking_bit "
            + to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0" 
                  " action_idx " + to_string(idx) + '\n';
            break;
    }
    return str;
}
// gen 4 level page table walk table
string gen_add_offset_1_tab(int qpn, int walking_bit, int act){
    string str;
    switch(act){
        case 1:
            str += "pd add_offset_1_tab add_entry calc_pgd_offset_1 ib_aeth_valid 1 md_walking_bit " + 
                to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0"
                + " action_addr_l 0xacc0a000 action_addr_h 0x1d\n";
            break;
        case 2: 
            str += "pd add_offset_1_tab add_entry calc_pud_offset_1 ib_aeth_valid 1 md_walking_bit " + 
                to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 3:
            str += "pd add_offset_1_tab add_entry calc_pmd_offset_1 ib_aeth_valid 1 md_walking_bit " + 
                to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 4:
            str += "pd add_offset_1_tab add_entry calc_pte_offset_1 ib_aeth_valid 1 md_walking_bit " + 
                to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 5:
            str += "pd add_offset_1_tab add_entry nop ib_aeth_valid 1 md_walking_bit " + 
                to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
    }
    return str;
}
string gen_add_offset_2_tab(int qpn, int act){
    string str;
    switch(act){
        case 1:
            str += "pd add_offset_2_tab add_entry calc_pgd_offset_2 ib_aeth_valid 1 md_qpn " 
                + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 2: 
            str += "pd add_offset_2_tab add_entry calc_pud_offset_2 ib_aeth_valid 1 md_qpn " 
                + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 3:
            break;
    }
    return str;
}
string gen_add_offset_3_tab(int qpn, int walking_bit, int act){
    string str;
    switch(act){
        case 1:
            str += "pd add_offset_3_tab add_entry calc_pgd_offset_3 ib_aeth_valid 1 md_qpn " + to_string(qpn) +
                + " md_walking_bit " + to_string(walking_bit) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 2: 
            str += "pd add_offset_3_tab add_entry calc_pud_offset_3 ib_aeth_valid 1 md_qpn " + to_string(qpn) +
                + " md_walking_bit " + to_string(walking_bit) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 3:
            str += "pd add_offset_3_tab add_entry calc_pmd_offset_3 ib_aeth_valid 1 md_qpn " + to_string(qpn) +
                + " md_walking_bit " + to_string(walking_bit) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 4:
            str += "pd add_offset_3_tab add_entry calc_pte_offset_3 ib_aeth_valid 1 md_qpn " + to_string(qpn) +
                + " md_walking_bit " + to_string(walking_bit) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 5:
            str += "pd add_offset_3_tab add_entry calc_page_offset_3 ib_aeth_valid 1 md_qpn " + to_string(qpn) +
                + " md_walking_bit " + to_string(walking_bit) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
        case 6:
            str += "pd add_offset_3_tab add_entry nop ib_aeth_valid 1 md_walking_bit " + 
                to_string(walking_bit) + " md_qpn " + to_string(qpn) + " eg_intr_md_from_parser_aux_clone_src 0\n";
            break;
    }
    return str;
}
string gen_mask_base_addr_tab(int qpn, int walking_bit){
    string str;
    str += "pd mask_base_addr_tab add_entry mask_base_addr ib_aeth_valid 1 md_qpn " + to_string(qpn) + " md_walking_bit " + 
        to_string(walking_bit) + " eg_intr_md_from_parser_aux_clone_src 0\n";
    return str;
}
string gen_make_up_addr_tab(int qpn, int walking_bit){
    string str;
    str += "pd make_up_addr_tab add_entry make_up_addr ib_aeth_valid 1 md_qpn " + to_string(qpn) + " md_walking_bit " + 
        to_string(walking_bit) + " eg_intr_md_from_parser_aux_clone_src 0\n";
    return str;
}


string Policy::gen_readmove_pgt_walk_code(){ // only target for Move
    string str; 
    cout << "Start encoding page table walk rule" << endl;
    // go through every readmove first
    for (int i = 0; i < this->all_aims.size(); i++){
        if (this->all_aims[i]->get_aim_name() == "ReadMove"){
            ReadMove * rmove = (ReadMove *)(this->all_aims[i]);
            // 2 situations: 1, end move; 2, normal move
            // we don't care about dqpn
            str += gen_mark_vmalloc_bit_p1_tab(qpn_tran(rmove->get_post_qpn())); // check whether is vmalloc or not
            str += gen_cache_dqpn_page_walk_tab(qpn_tran(rmove->get_post_qpn()), 1, 0, this->task_nr, 2); // cache into reg
            str += gen_cache_qpn_page_walk_tab(qpn_tran(rmove->get_post_qpn()), 1, 0, this->task_nr, 2); // cache into reg
            // transit to pgt walk qpn
            str += gen_pgt_transfer_tab(qpn_tran(rmove->get_post_qpn()), ((ReadLoad *)(this->pgt_aims.at(0)))->get_post_qpn(), 1);
            // handle last round pte
            str += gen_add_offset_3_tab(qpn_tran(rmove->get_post_qpn()), 1, 5);
            str += gen_mask_base_addr_tab(qpn_tran(rmove->get_post_qpn()), 1);
            str += gen_make_up_addr_tab(qpn_tran(rmove->get_post_qpn()), 1);
            str += gen_cache_process_page_addr_to_reg_h_tab(qpn_tran(rmove->get_post_qpn()), 1, this->task_nr, 1); // cache
            str += gen_cache_process_page_addr_to_reg_l_tab(qpn_tran(rmove->get_post_qpn()), 1, this->task_nr, 1); // cache
        }
    }
    return str;
}

// go through every pgt_aims

string Policy::gen_pgt_aims_code(){
    string str;
    for (int i = 0; i < this->pgt_aims.size(); i++){
        ReadLoad * rload = (ReadLoad *)(this->pgt_aims.at(i));
        str += gen_read_update_psn_tab(rload->get_post_qpn(), rload->get_post_qpn()-this->base_state);
        str += gen_read_update_psn_def_tab(this->qpn_tran(rload->get_post_qpn()), rload->get_post_qpn()-this->base_state);
        // cache timestamp
        // temperary disable
        //str += gen_read_update_ts_start_tab(this->qpn_tran(rload->get_post_qpn()), rload->get_post_qpn()-this->base_state);
        str += gen_mod_field_parameters_tab(rload->get_post_qpn(), rload->get_post_qpn(), 0);

        if (i == 0){ // pgd walk
            str += gen_cache_process_page_addr_to_reg_h_tab(rload->get_post_qpn(), 0, this->task_nr, 2); // cache
            str += gen_cache_process_page_addr_to_reg_l_tab(rload->get_post_qpn(), 0, this->task_nr, 2); // cache
            str += gen_add_offset_1_tab(rload->get_post_qpn(), 0, 1); 
            str += gen_add_offset_2_tab(rload->get_post_qpn(), 1);
            str += gen_add_offset_3_tab(rload->get_post_qpn(), 0, 1); 
            str += gen_make_up_addr_tab(rload->get_post_qpn(), 0);
            str += gen_pgt_transfer_tab(qpn_tran(rload->get_post_qpn()), ((ReadLoad *)(this->pgt_aims.at(1)))->get_post_qpn(), 0);
        }
        if (i == 1){ // pud walk
            str += gen_cache_process_page_addr_to_reg_h_tab(rload->get_post_qpn(), 0, this->task_nr, 1); // read
            str += gen_cache_process_page_addr_to_reg_l_tab(rload->get_post_qpn(), 0, this->task_nr, 1); // read
            str += gen_add_offset_1_tab(rload->get_post_qpn(), 0, 2); 
            str += gen_add_offset_2_tab(rload->get_post_qpn(), 2);
            str += gen_add_offset_3_tab(rload->get_post_qpn(), 0, 2); 
            str += gen_mask_base_addr_tab(rload->get_post_qpn(), 0);
            str += gen_make_up_addr_tab(rload->get_post_qpn(), 0);
            str += gen_pgt_transfer_tab(qpn_tran(rload->get_post_qpn()), ((ReadLoad *)(this->pgt_aims.at(2)))->get_post_qpn(), 0);
        }
        if (i == 2){ // pmd walk
            str += gen_cache_process_page_addr_to_reg_h_tab(rload->get_post_qpn(), 0, this->task_nr, 1); // read
            str += gen_cache_process_page_addr_to_reg_l_tab(rload->get_post_qpn(), 0, this->task_nr, 1); // read
            str += gen_add_offset_1_tab(rload->get_post_qpn(), 0, 3); 
            str += gen_add_offset_3_tab(rload->get_post_qpn(), 0, 3); 
            str += gen_mask_base_addr_tab(rload->get_post_qpn(), 0);
            str += gen_make_up_addr_tab(rload->get_post_qpn(), 0);
            str += gen_pgt_transfer_tab(qpn_tran(rload->get_post_qpn()), ((ReadLoad *)(this->pgt_aims.at(3)))->get_post_qpn(), 0);
        }
        if (i == 3){ // pte walk
            str += gen_cache_process_page_addr_to_reg_h_tab(rload->get_post_qpn(), 0, this->task_nr, 1); // read
            str += gen_cache_process_page_addr_to_reg_l_tab(rload->get_post_qpn(), 0, this->task_nr, 1); // read
            str += gen_add_offset_1_tab(rload->get_post_qpn(), 0, 4); 
            str += gen_add_offset_3_tab(rload->get_post_qpn(), 0, 4); 
            str += gen_mask_base_addr_tab(rload->get_post_qpn(), 0);
            str += gen_make_up_addr_tab(rload->get_post_qpn(), 0);
            // mark walking
            str += gen_mark_walking_bit_tab(qpn_tran(rload->get_post_qpn()));
            str += gen_cache_dqpn_page_walk_tab(qpn_tran(rload->get_post_qpn()), 0, 1, this->task_nr, 1); // cache into reg
            str += gen_cache_qpn_page_walk_tab(qpn_tran(rload->get_post_qpn()), 0, 1, this->task_nr, 1); // cache into reg
        }

    }
    return str;
}
