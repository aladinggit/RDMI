#include <tofino/intrinsic_metadata.p4>
#include <tofino/constants.p4>
#include <tofino/primitives.p4>
#include <tofino/stateful_alu_blackbox.p4>

#include "./includes/headers.p4"
#include "./includes/parser.p4"

#define PKT_INSTANCE_TYPE_NORMAL 0
#define PKT_INSTANCE_TYPE_INGRESS_CLONE 1
#define PKT_INSTANCE_TYPE_EGRESS_CLONE 2
#define PKT_INSTANCE_TYPE_COALESCED 3
#define PKT_INSTANCE_TYPE_INGRESS_RECIRC 4
#define PKT_INSTANCE_TYPE_REPLICATION 5
#define PKT_INSTANCE_TYPE_RESUBMIT 6

#define TASKS_OFFSET 0x7A8
#define PID_TASKS 0x100
#define PID_OFFSET 0x8A8

#define IB_OPCODE_RC_WRITE_ONLY  10
#define IB_OPCODE_RC_WRITE_FIRST 6
#define IB_OPCODE_RC_READ_REQ    12
#define IB_OPCODE_RC_SEND_ONLY 4
#define IB_OPCODE_UD_SEND_ONLY 100

#define IB_OPCODE_RC_READ_RESPONSE 16



action set_egr(egress_spec) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_spec);
}

table simple_forward_tab {
    reads {
//        md.etc         : exact;
        ipv4           : valid;
        ipv4.dstAddr   : exact;
    }
    actions {
        set_egr;
    }
    size: 1024; // concurrency
}

table mac_forward_tab{
    reads{
        ipv4 : valid;
        ethernet.dstAddr : exact;
    }
    actions {
        set_egr;
    }
}


// update ipv4 checksum
field_list ipv4_checksum_list {
    ipv4.version;
    ipv4.ihl;
    ipv4.diffserv;
    ipv4.totalLen;
    ipv4.identification;
    ipv4.flags;
    ipv4.fragOffset;
    ipv4.ttl;
    ipv4.protocol;
    ipv4.srcAddr;
    ipv4.dstAddr;
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}



calculated_field ipv4.hdrChecksum  {
//    verify ipv4_checksum;
    update ipv4_checksum;
}

action nop() {
}

action _drop() {
    drop();
}

table drop_tab {
    actions {
        _drop;
    }
    default_action: _drop;
    size: 1; // concurrency
}


/**************************************
 * Code block: endian translation.
 * Start
 * *************************************/
field_list addr {
    ib_payload_8.payLoad_8;
    ib_payload_8.payLoad_7;
    ib_payload_8.payLoad_6;
    ib_payload_8.payLoad_5;
    ib_payload_8.payLoad_4;
    ib_payload_8.payLoad_3;
    ib_payload_8.payLoad_2;
    ib_payload_8.payLoad_1;
}

field_list_calculation addr_low32 {
    input {
      addr;
    }
    algorithm : identity_lsb;
    output_width: 32;
}

field_list_calculation addr_high32 {
    input {
      addr;
    }
    algorithm : identity_msb;
    output_width: 32;
}

action split_addr_low32 () {
    modify_field_with_hash_based_offset(md.aeth_addr_l, 0, addr_low32, 4294967296);
}
action split_addr_high32 () {
    modify_field_with_hash_based_offset(md.aeth_addr_h, 0, addr_high32, 4294967296);
}

table split_addr_high32_tab {
    reads {
        ib_aeth : valid;
//    	ib_bth.dqpn : exact;
        md.len : exact; 
    }
    actions{
      split_addr_high32;
    }
    size: 2; // concurrency
}

table split_addr_low32_tab {
    reads {
        ib_aeth : valid;
//        ib_bth.dqpn : exact;
        md.len : exact;
    }
    actions{
      split_addr_low32;
    }
    size: 2; // concurrency
}

// Encode the PSN rule here
/**************************************
 * Code block: Read andn update the PSN corresponding to each QPN.
 * Start
 * *************************************/
register psn {
    width   : 32;
// concurrency    instance_count  : 16;
   instance_count  : 500;
}

blackbox stateful_alu read_update_psn_alu {
    reg: psn;

    update_lo_1_value: register_lo + 1;

// somehow it cannot be directly written to header
// We have to merge ack_req with psn, otherwise switch doesn't work
    output_dst: ib_bth.psn;
    output_value: alu_lo;
}

action read_update_psn(state) {
    read_update_psn_alu.execute_stateful_alu(state);
}

table read_update_psn_tab {
    reads {
        ib_aeth : valid;
        ib_bth.dqpn : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        read_update_psn;
    }
    size: 1024; // concurrency
}

// Encode the SELF-DEFENSE rule here
/**************************************
 * Code block: Read and update the PSN corresponding to each response packet QPN
 * Start
 * *************************************/

register psn_def {
    width   : 32;
   instance_count  : 500;
}

blackbox stateful_alu read_update_psn_def_alu {
    reg: psn_def;

    update_lo_1_value: register_lo + 1;

    output_dst: md.psn_def;
    output_value: alu_lo;
}

action read_update_psn_def(state) {
    read_update_psn_def_alu.execute_stateful_alu(state);
}

table read_update_psn_def_tab {
    reads {
        ib_aeth : valid;
        ib_bth.dqpn : exact;

    }
    actions {
        read_update_psn_def;
    }
    size: 1024; // concurrency
}

action gen_spoof_alarm(){
// alarm spoofed packet
//    modify_field(ib_bth.dqpn, 9999);
    modify_field(md.qpn, 999);
//    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_spec); 
//    modify_field(ipv4.dstAddr, dstip);
    modify_field(md.spoof, 1);
    modify_field(ib_bth.tver, 1);
}

table gen_spoof_alarm_tab{
    reads {
        ib_aeth: valid;
    }
    actions {
        gen_spoof_alarm;
    }
}

// Encode the State(QPN) transition rule here
/**************************************
 * Code block: state transfering.
 * Start
 * *************************************/

// Encode the rKey caching rule here
/**************************************
 * Code block: cache and read the rkey
 * Start
 * *************************************/
register rkey {
    width   : 32;
// concurrency    instance_count  : 1;
    instance_count  :30;
}

blackbox stateful_alu cache_rkey_alu {
    reg : rkey;

// somehow it cannot be directly written to header
    output_dst : ib_reth.rkey;
    output_value : register_lo;
}

action cache_rkey() {
    cache_rkey_alu.execute_stateful_alu(0);
}

table cache_rkey_tab {
    reads {
        ib_aeth : valid;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        cache_rkey;
    }
    size: 100; // concurrency
}

// Encode header flag setting here, and read the addr to packet header
/**************************************
 * Code block: cache and read the rkey
 *      including len_udp and len_IPV4
 *      Read addr from md.aeth_addr to ib_reth.virtAddr
 * Start
 * *************************************/
action set_bth_reth_flags() {
// We have to merge ack_req with psn, otherwise switch doesn't work
//    modify_field(ib_bth.ack_req, 1);
//    modify_field(ib_bth.reserved2, 0);
    modify_field(ib_bth.migReq, 1);
    modify_field(ib_bth.padCnt, 0);
    modify_field(ib_bth.opCode, 12);
//    modify_field(, 0x40);
}

table set_bth_reth_flags_tab {
    reads {
        ib_aeth : valid;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        set_bth_reth_flags;
    }
    size: 10; // concurrency
}

action modify_header_len() {
// this cannot be combined with the previous table somehow
    modify_field(ipv4.totalLen, 60);
    modify_field(udp.hdr_length, 40);
}

table modify_header_len_tab {
    reads {
        ib_aeth : valid;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        modify_header_len;
    }
    size: 10; // concurrency
}

// We have to merge ack_req with psn, otherwise switch doesn't work
//action read_psn_to_header() {
//// this cannot be combined with the previous table somehow
//    modify_field(ib_bth.psn, md.psn);
//}
//
//table read_psn_to_header_tab {
//    reads {
//        ib_aeth : valid;
//    }
//    actions {
//        read_psn_to_header;
//    }
//}

// read header 
action read_addr_to_header() {
    modify_field(ib_reth.virtAddr_h, md.aeth_addr_h);
    modify_field(ib_reth.virtAddr_l, md.aeth_addr_l);
}

table read_addr_to_header_tab {
    reads {
        ib_aeth: valid;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        read_addr_to_header;
    }
    size: 10; // concurrency
}

// swap dst/src address in IPV4/ETHER
/**************************************
 * Code block: Swap the address in packet header
 * START
 *  *************************************/
action forward_rnic() {
    swap(ipv4.srcAddr, ipv4.dstAddr);
    swap(ethernet.dstAddr, ethernet.srcAddr);
//    modify_field(ig_intr_md_for_tm.ucast_egress_port, ig_intr_md.ingress_port);
}

table forward_rnic_tab {
    reads {
        ib_aeth : valid;
    }
    actions {
        forward_rnic;
    }
    size: 10; // concurrency
}

// packet clone to control plane
/**************************************
 * Code block: Clone the target packet, change the port to 192(control plane)
 * Start
 * *************************************/

field_list check_range_list {
    md.k1;
    md.k2;
}

action cloning() {
    clone_ingress_pkt_to_egress(1, check_range_list);
}

table cloning_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
    }
    actions {
        cloning;
    }
    size: 1024; // concurrency
}

action gen_mali_alarm(){
    modify_field(ib_bth.se, 1);
}

table gen_mali_alarm_tab{
    reads {
        ib_bth.dqpn: exact;
        md.k1: exact;
        md.k2: exact;
        eg_intr_md_from_parser_aux.clone_src: exact;       
    }
    actions {
        gen_mali_alarm;
    }
}

// if packet is cloned, change the port to control plane
action change_port_to_control() {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, 192);
}

table change_port_to_control_tab {
    reads {
        standard_metadata.instance_type: exact;
    }
    actions {
        change_port_to_control;
    }
    size: 1024; // concurrency
}



// Add new header and remove the original header
/**************************************
 * Code block: Returned header format is IB_AETH
 *      Add IB_RETH as send header
 * Start
 * *************************************/
action add_reth() {
    add_header(ib_reth);
}

table add_reth_tab {
    reads {
        ib_aeth: valid;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        add_reth;
    }
    size: 10; // concurrency
}

action remove_aeth() {
    remove_header(ib_aeth);
}

table remove_aeth_tab {
    reads {
        ib_aeth: valid;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        remove_aeth;
    }
    size: 10; // concurrency
}

action remove_payload_4() {
    remove_header(ib_payload_4);
}

action remove_payload_8() {
    remove_header(ib_payload_8);
}

action remove_payload_16() {
    remove_header(ib_payload_16);
}

action remove_payload_32() {
    remove_header(ib_payload_32);
}

table remove_payload_tab {
    reads {
        ib_aeth : valid;
        md.len : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        nop;
    	remove_payload_4;
    	remove_payload_8;
        remove_payload_16;
        remove_payload_32;
    }
    size: 20; // concurrency
}

// Physical address translation start here
/**************************************
 * Code block: Tables for translate address
 * Start
 * *************************************/
action trans_mode_1() {
    modify_field(md.trans_mode, 1);
}

action trans_mode_2() {
    modify_field(md.trans_mode, 2);
}

table trans_mode_tab{
    reads {
        ib_aeth : valid;
// temp       md.qpn : exact;
// temp       md.vmalloc_bit : exact;
// temp       md.walking_bit : exact;
        md.aeth_addr_h : ternary;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        trans_mode_1;
        trans_mode_2;
    }
    size: 30; // concurrency
}

field_list addr_l {
    md.aeth_addr_l;
}

field_list_calculation addr_high1 {
    input {
      addr_l;
    }
    algorithm : identity_msb;
    output_width: 4;
}

action split_addr_high1 () {
    modify_field_with_hash_based_offset(md.greater_4, 0, addr_high1, 16);
}

table split_addr_high1_tab {
    reads {
        ib_aeth : valid;
        md.trans_mode : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions{
        nop;
        split_addr_high1;
    }
//    size: 1;
    size: 20; // concurrency
}

// constants for linear translation
//action cache_offset_into_meta() {
//    modify_field(md.offset_1, 0xffff965a);
//    modify_field(md.offset_2, 0x40000000);
//    modify_field(md.offset_3, 0xffff965b);
//    modify_field(md.offset_4, 0x0bc00000);
//    modify_field(md.offset_5, 0xC0000000);
//}
action cache_offset_into_meta(addr1, addr2, addr3, addr4, addr5, addr6) {
    modify_field(md.offset_1, addr1);
    modify_field(md.offset_2, addr2);
    modify_field(md.offset_3, addr3);
    modify_field(md.offset_4, addr4);
    modify_field(md.offset_5, addr5);
    modify_field(md.offset_6, addr6);
}
table cache_offset_into_meta_tab {
    reads {
        ib_aeth : valid;
//        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
	    cache_offset_into_meta;
    }
//    default_action: cache_offset_into_meta;
//    size: 1024; // concurrency
    size: 10;
}

action addr_translation_phys_offset() {
//    modify_field(md.aeth_addr_h, 0x1d);
    modify_field(md.aeth_addr_h, md.offset_6);
    add_to_field(md.aeth_addr_l, md.offset_4);
}

// > 0x40000000
action addr_translation_page_offset_1() {
    subtract_from_field(md.aeth_addr_h, md.offset_1);
    subtract_from_field(md.aeth_addr_l, md.offset_2);
}

// < 0x40000000
action addr_translation_page_offset_2() {
    subtract_from_field(md.aeth_addr_h, md.offset_3);
    add_to_field(md.aeth_addr_l, md.offset_5);
}


table addr_translation_page_offset_tab {
    reads {
        ib_aeth : valid;
        md.greater_4 : range;
        md.trans_mode : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        addr_translation_page_offset_1;
    	addr_translation_page_offset_2;
        addr_translation_phys_offset;
    }
    size: 1024; // concurrency
}
// End of address translation block //


// Alternative code block
// Cache address to reg_1
/**************************************
 * Code block: Tables for cache and read address from/to register
 * Start
 * *************************************/
register process_addr_h {
    width   : 32;
// concurrency    instance_count  : 15;
    instance_count  :450;
}

blackbox stateful_alu write_process_addr_to_reg_h_alu {
    reg : process_addr_h;

    update_lo_1_value : md.aeth_addr_h;

    output_dst : md.aeth_addr_h;
    output_value : alu_lo;
}

action write_process_addr_to_reg_h(state) {
    write_process_addr_to_reg_h_alu.execute_stateful_alu(state);
}

blackbox stateful_alu read_process_addr_from_reg_h_alu {
    reg : process_addr_h;

    output_dst : md.aeth_addr_h;
    output_value : register_lo;
}

action read_process_addr_from_reg_h(state) {
    read_process_addr_from_reg_h_alu.execute_stateful_alu(state);
}

table cache_process_addr_to_reg_h_tab {
    reads {
        ib_aeth: valid;
        md.qpn: exact;
        ib_bth.dqpn : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        write_process_addr_to_reg_h;
        read_process_addr_from_reg_h;
    }
    size: 1024; // concurrency
}

// handle lower part of address
register process_addr_l {
    width   : 32;
// concurrency    instance_count  : 15;
    instance_count: 450;
}

blackbox stateful_alu write_process_addr_to_reg_l_alu {
    reg : process_addr_l;

    update_lo_1_value : md.aeth_addr_l;

    output_dst : md.aeth_addr_l;
    output_value : alu_lo;
}

action write_process_addr_to_reg_l(state) {
    write_process_addr_to_reg_l_alu.execute_stateful_alu(state);
}

blackbox stateful_alu read_process_addr_from_reg_l_alu {
    reg : process_addr_l;

    output_dst : md.aeth_addr_l;
    output_value : register_lo; // weird, shouldn't it be alu_lo?
}

action read_process_addr_from_reg_l(state) {
    read_process_addr_from_reg_l_alu.execute_stateful_alu(state);
}

blackbox stateful_alu read_update_iter_addr_l_alu {
    reg : process_addr_l;

    update_lo_1_value : register_lo + md.entry_size;

    output_dst : md.aeth_addr_l;
    output_value : alu_lo;
}

action read_update_iter_addr_l(state) {
    read_update_iter_addr_l_alu.execute_stateful_alu(state);
}

table cache_process_addr_to_reg_l_tab {
    reads {
        ib_aeth: valid;
        md.qpn: exact;
        ib_bth.dqpn : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        write_process_addr_to_reg_l;
        read_process_addr_from_reg_l;
        read_update_iter_addr_l;
    }
    size: 1024; // concurrency
}


// Modify header with different parameter depending on the current state
/**************************************
 * Code block: Modify offset and length_to_read
 *      note that we might need to subtract an offset, but 
 *      subtraction in P4 cannot use parameter as subtractor.
 *      Thus we have to encode subtractcor into a metadata first.
 * Start
 * *************************************/
action encode_mod_offset(offset) {
    modify_field(md.mod_value, offset);
}

table encode_mod_offset_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
        ib_bth.dqpn : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        encode_mod_offset;       
    }
    size: 1024; // concurrency
}

table encode_mod_offset_pre_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
        ib_bth.dqpn : exact;
    }
    actions {
        encode_mod_offset;
    }
    size: 1024; // concurrency
}

action mod_field_parameters_add(len) {
    modify_field(ib_reth.len, len);
    add_to_field(md.aeth_addr_l, md.mod_value);
}

action mod_field_parameters_subtract(len) {
    modify_field(ib_reth.len, len);
    subtract_from_field(md.aeth_addr_l, md.mod_value);
}

action mod_field_parameters_add_pre() {
    add_to_field(md.aeth_addr_l, md.mod_value);
}

action mod_field_parameters_subtract_pre() {
    subtract_from_field(md.aeth_addr_l, md.mod_value);
}

table mod_field_parameters_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
        ib_bth.dqpn : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        mod_field_parameters_add;
        mod_field_parameters_subtract;
    }
    size: 1024; // concurrency
}

table mod_field_parameters_pre_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
        ib_bth.dqpn : exact;
    }
    actions {
        mod_field_parameters_add_pre;
        mod_field_parameters_subtract_pre;
    }
    size: 1024; // concurrency
}
// End of the traversal table
/**************************************
 * Code block: End of the traversal if criteria is met, drop the packet
 * Start
 * *************************************/
// add end point
table end_of_fetching_tab {
    reads {
        ib_aeth : valid;
//        md.qpn : exact;
        ib_bth.dqpn : exact;
    }
    actions {
        nop;
        _drop;
    }
    size: 20; // concurrency
}

// debug
register ddqpn {
    width   : 32;
    instance_count : 30;
}

blackbox stateful_alu cache_ddqpn_alu {
    reg : ddqpn;

    update_lo_1_value : ib_bth.dqpn;
}

action cache_ddqpn() {
    cache_ddqpn_alu.execute_stateful_alu(0);
}

table cache_ddqpn_tab {
    reads {
        ib_aeth: valid;
    }
    actions {
        cache_ddqpn;
    }
//    default_action: cache_ddqpn;
}

register qqpn {
    width   : 32;
    instance_count : 30;
}

blackbox stateful_alu cache_qqpn_alu {
    reg : qqpn;

    update_lo_1_value : md.qpn;
}

action cache_qqpn() {
    cache_qqpn_alu.execute_stateful_alu(0);
}

table cache_qqpn_tab {
    reads {
        ib_aeth: valid;
    }
    actions {
        cache_qqpn;
    }
//    default_action: cache_qqpn;
}

// entry
register ddqpn_ent_1 {
    width   : 32;
    instance_count : 1;
}

blackbox stateful_alu cache_ddqpn_ent_1_alu {
    reg : ddqpn_ent_1;

    update_lo_1_value : ib_bth.dqpn;
}

action cache_ddqpn_ent_1() {
    cache_ddqpn_ent_1_alu.execute_stateful_alu(0);
}

table cache_ddqpn_ent_1_tab {
    reads {
        ib_aeth: valid;
    }
    actions {
        cache_ddqpn_ent_1;
    }
//    default_action: cache_ddqpn_ent_1;
}

register qqpn_ent_1 {
    width   : 32;
    instance_count : 1;
}

blackbox stateful_alu cache_qqpn_ent_1_alu {
    reg : qqpn_ent_1;

    update_lo_1_value : md.qpn;
}

action cache_qqpn_ent_1() {
    cache_qqpn_ent_1_alu.execute_stateful_alu(0);
}

table cache_qqpn_ent_1_tab {
    reads {
        ib_aeth: valid;
    }
    actions {
        cache_qqpn_ent_1;
    }
//    default_action: cache_qqpn_ent_1;
}

register ddqpn_ent_2 {
    width   : 32;
    instance_count : 1;
}

blackbox stateful_alu cache_ddqpn_ent_2_alu {
    reg : ddqpn_ent_2;

    update_lo_1_value : ib_bth.dqpn;
}

action cache_ddqpn_ent_2() {
    cache_ddqpn_ent_2_alu.execute_stateful_alu(0);
}

table cache_ddqpn_ent_2_tab {
    reads {
        ib_aeth: valid;
    }
    actions {
        cache_ddqpn_ent_2;
    }
//    default_action: cache_ddqpn_ent_2;
}

register qqpn_ent_2 {
    width   : 32;
    instance_count : 1;
}

blackbox stateful_alu cache_qqpn_ent_2_alu {
    reg : qqpn_ent_2;

    update_lo_1_value : md.qpn;
}

action cache_qqpn_ent_2() {
    cache_qqpn_ent_2_alu.execute_stateful_alu(0);
}

table cache_qqpn_ent_2_tab {
    reads {
        ib_aeth: valid;
    }
    actions {
        cache_qqpn_ent_2;
    }
//    default_action: cache_qqpn_ent_2;
}

register ddqpn_ent_3 {
    width   : 32;
    instance_count : 1;
}

blackbox stateful_alu cache_ddqpn_ent_3_alu {
    reg : ddqpn_ent_3;

    update_lo_1_value : ib_bth.dqpn;
}

action cache_ddqpn_ent_3() {
    cache_ddqpn_ent_3_alu.execute_stateful_alu(0);
}

table cache_ddqpn_ent_3_tab {
    reads {
        ib_aeth: valid;
    }
    actions {
        cache_ddqpn_ent_3;
    }
//    default_action: cache_ddqpn_ent_3;
}

register qqpn_ent_3 {
    width   : 32;
    instance_count : 1;
}

blackbox stateful_alu cache_qqpn_ent_3_alu {
    reg : qqpn_ent_3;

    update_lo_1_value : md.qpn;
}

action cache_qqpn_ent_3() {
    cache_qqpn_ent_3_alu.execute_stateful_alu(0);
}

table cache_qqpn_ent_3_tab {
    reads {
        ib_aeth: valid;
    }
    actions {
        cache_qqpn_ent_3;
    }
//    default_action: cache_qqpn_ent_3;
}

// page table walk unit
// register for caching the current dQPN
register dqpn_page_walk {
    width   : 32;
// concurrency    instance_count  : 1;
    instance_count : 30;
}

blackbox stateful_alu cache_dqpn_page_walk_alu {
    reg : dqpn_page_walk;

    update_lo_1_value : ib_bth.dqpn;
}

action cache_dqpn_page_walk(idx) {
    cache_dqpn_page_walk_alu.execute_stateful_alu(idx);
}

blackbox stateful_alu read_dqpn_page_walk_alu {
    reg : dqpn_page_walk;

// update to header
    output_dst : ib_bth.dqpn;
    output_value : register_lo;
}

action read_dqpn_page_walk(idx) {
    read_dqpn_page_walk_alu.execute_stateful_alu(idx);
}

table cache_dqpn_page_walk_tab {
    reads {
        ib_aeth: valid;
        md.qpn: exact;
        md.vmalloc_bit: exact;
        md.walking_bit: exact;
    }
    actions {
        cache_dqpn_page_walk;
        read_dqpn_page_walk;
    }
    size: 1024; // concurrency
}

// register for caching the current QPN
register qpn_page_walk {
    width   : 32;
//    instance_count  : 1;
    instance_count : 30;
}

blackbox stateful_alu cache_qpn_page_walk_alu {
    reg : qpn_page_walk;

    update_lo_1_value : md.qpn;
}

action cache_qpn_page_walk(idx) {
    cache_qpn_page_walk_alu.execute_stateful_alu(idx);
}

blackbox stateful_alu read_qpn_page_walk_alu {
    reg : qpn_page_walk;

// update to header
    output_dst : md.qpn;
    output_value : register_lo;
}

action read_qpn_page_walk(idx) {
    read_qpn_page_walk_alu.execute_stateful_alu(idx);
}

table cache_qpn_page_walk_tab {
    reads {
        ib_aeth: valid;
        md.qpn: exact;
        md.vmalloc_bit: exact;
        md.walking_bit: exact;
    }
    actions {
        cache_qpn_page_walk;
        read_qpn_page_walk;
    }
    size: 1024; // concurrency
}


// register for caching the address
register process_page_addr_h {
    width   : 32;
// concurrency    instance_count  : 1;
    instance_count : 30;
}

blackbox stateful_alu cache_process_page_addr_to_reg_h_alu {
    reg : process_page_addr_h;

    update_lo_1_value : md.aeth_addr_h;

    output_dst : md.cached_addr_h;
    output_value : alu_lo;
}

action cache_process_page_addr_to_reg_h(idx) {
    cache_process_page_addr_to_reg_h_alu.execute_stateful_alu(idx);
}

blackbox stateful_alu read_process_page_addr_to_reg_h_alu {
    reg : process_page_addr_h;

    output_dst : md.cached_addr_h;
    output_value : register_lo;
}

action read_process_page_addr_to_reg_h(idx) {
    read_process_page_addr_to_reg_h_alu.execute_stateful_alu(idx);
}

table cache_process_page_addr_to_reg_h_tab {
    reads {
        ib_aeth: valid;
        md.walking_bit: exact;
        md.qpn: exact;
        eg_intr_md_from_parser_aux.clone_src: exact; // clone
    }
    actions {
        cache_process_page_addr_to_reg_h;
        read_process_page_addr_to_reg_h;
    }
    size: 1024; // concurrency
}

register process_page_addr_l {
    width   : 32;
// concurrency    instance_count  : 1;
    instance_count : 30;
}

blackbox stateful_alu cache_process_page_addr_to_reg_l_alu {
    reg : process_page_addr_l;

    update_lo_1_value : md.aeth_addr_l;

    output_dst : md.cached_addr_l;
    output_value : alu_lo;
}

action cache_process_page_addr_to_reg_l(idx) {
    cache_process_page_addr_to_reg_l_alu.execute_stateful_alu(idx);
}

blackbox stateful_alu read_process_page_addr_to_reg_l_alu {
    reg : process_page_addr_l;

    output_dst : md.cached_addr_l;
    output_value : register_lo;
}

action read_process_page_addr_to_reg_l(idx) {
    read_process_page_addr_to_reg_l_alu.execute_stateful_alu(idx);
}

table cache_process_page_addr_to_reg_l_tab {
    reads {
        ib_aeth: valid;
        md.walking_bit: exact;
        md.qpn: exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        cache_process_page_addr_to_reg_l;
        read_process_page_addr_to_reg_l;
    }
    size: 1024; // concurrency
}

// start calculating offset(index)
// the address of global pgd table should be a configuration
action calc_pgd_offset_1(addr_l, addr_h) {
//    modify_field(md.aeth_addr_l, 0xacc0a000);
//    modify_field(md.aeth_addr_h, 0x1d);
    modify_field(md.aeth_addr_l, addr_l);
    modify_field(md.aeth_addr_h, addr_h);
}

action calc_pgd_offset_2() {
    shift_right(md.cached_addr_h, md.cached_addr_h, 4);
}

action calc_pgd_offset_3() {
    bit_and(md.page_offset, md.cached_addr_h, 0x00000ff8);
}

action calc_pud_offset_1() {
    shift_left(md.cached_addr_h, md.cached_addr_h, 5);
    shift_right(md.cached_addr_l, md.cached_addr_l, 27);
}

action calc_pud_offset_2() {
    add(md.page_offset, md.cached_addr_h, md.cached_addr_l);
}

action calc_pud_offset_3() {
    bit_and(md.page_offset, md.page_offset, 0x00000ff8);
}

action calc_pmd_offset_1() {
    shift_right(md.cached_addr_l, md.cached_addr_l, 18);
}

action calc_pmd_offset_3() {
    bit_and(md.page_offset, md.cached_addr_l, 0x00000ff8);
}

action calc_pte_offset_1() {
    shift_right(md.cached_addr_l, md.cached_addr_l, 9);
}

action calc_pte_offset_3() {
    bit_and(md.page_offset, md.cached_addr_l, 0x00000ff8);
}

action calc_page_offset_3() {
    bit_and(md.page_offset, md.cached_addr_l, 0x00000fff);
}

table add_offset_1_tab {
    reads {
        ib_aeth : valid;
        md.walking_bit : exact;
        md.qpn : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        nop;
        calc_pgd_offset_1;
        calc_pud_offset_1;
        calc_pmd_offset_1;
        calc_pte_offset_1;
    }
    size: 1024; // concurrency
}

table add_offset_2_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        nop;
        calc_pud_offset_2;
        calc_pgd_offset_2;
    }
    size: 1024; // concurrency
}

table add_offset_3_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
        md.walking_bit: exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        nop;
        calc_page_offset_3;
        calc_pgd_offset_3;
        calc_pud_offset_3;
        calc_pmd_offset_3;
        calc_pte_offset_3;
    }
    size: 1024; // concurrency
}

action mask_base_addr() {
    bit_and(md.aeth_addr_h, md.aeth_addr_h, 0x00003fff);
    bit_and(md.aeth_addr_l, md.aeth_addr_l, 0xfffff000);
}

table mask_base_addr_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
        md.walking_bit: exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        mask_base_addr;
        nop;
    }
    size: 1024; // concurrency
}

// put base address and offset index together
action make_up_addr() {
    add_to_field(md.aeth_addr_l, md.page_offset);
//    modify_field(ib_reth.virtAddr_h, md.aeth_addr_h);
}

table make_up_addr_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
        md.walking_bit : exact;
        eg_intr_md_from_parser_aux.clone_src: exact;

    }
    actions {
        make_up_addr;
        nop;
    }
    size: 1024; // concurrency
}

// adding checking logic
/* For range checking, first we need to pull out address in 16 bits metadata */
field_list addr_rh {
    ib_payload_8.payLoad_4;
    ib_payload_8.payLoad_3;
    ib_payload_8.payLoad_2;
    ib_payload_8.payLoad_1;
}

field_list_calculation addr_low16 {
    input {
      addr_rh;
    }
    algorithm : identity_lsb;
    output_width: 16;
}

field_list_calculation addr_high16 {
    input {
      addr_rh;
    }
    algorithm : identity_msb;
    output_width: 16;
}

action split_addr_low_16 () {
    modify_field_with_hash_based_offset(md.addr_l_16, 0, addr_low16, 65536);
}

table split_addr_low16_tab {
    reads {
        ib_aeth: valid;
//        md.qpn : exact;
    }
    actions{
        split_addr_low_16;
    }
//    size: 16; // concurrency
    size: 10; // concurrency
}

action split_addr_high_16 () {
    modify_field_with_hash_based_offset(md.addr_h_16, 0, addr_high16, 65536);
}

table split_addr_high16_tab {
    reads {
        ib_aeth: valid;
//        md.qpn : exact;
    }
    actions{
        split_addr_high_16;
    }
//    size: 16; // concurrency
    size: 10; // concurrency
}
/* After splitting, apply two range check tables*/
action mark_range_k1(){
    modify_field(md.k1, 1);
}

action mark_range_k2(){
    modify_field(md.k2, 1);
}

table exact_match_tab{
    reads {
        ib_aeth : valid;
        md.qpn: exact;
        md.addr_h_16: exact;
        md.addr_l_16: range;
    }
    actions{
        mark_range_k1;
        _drop;
    }
// concurrency    size: 1024;
// too large    size: 8182; // concurrency
    size: 1024;
}

field_list range_field{
//    md.k1;
//    md.k2;
    md.qpn;
}

action gen_range_digest(){  // generate range digest
    generate_digest(0, range_field);
}

//// temparary mechanism for replacing digest
//register check_range{
//    width: 8;
//// concurrency    instance_count: 1;
//    instance_count : 30;
//}
//
//blackbox stateful_alu update_range_alu {
//    reg: check_range;
//
//    update_lo_1_value: register_lo + 1;
//}
//
//action gen_range_digest() {
//    update_range_alu.execute_stateful_alu(0);
//}

table gen_range_digest_tab{
    reads {
        ib_aeth : valid;
        md.qpn: exact;
        md.k1: exact;
        md.k2: exact;
    }
    actions {
        gen_range_digest;
        nop;
    }
    size: 1024; // concurrency
}

table range_match_tab{
    reads {
        ib_aeth : valid;
        md.qpn: exact;
        md.addr_h_16: range;
    }
    actions{
        mark_range_k2;
        _drop;
    }
// concurrency   size: 1024;
// too large    size: 8092; // concurrency
// works    size: 1024;
    size: 1024;
}
/* Count logic starts here */
register count_reg_1 {
    width   : 32;
    instance_count  : 1;
}

blackbox stateful_alu update_count_1_alu {
    reg: count_reg_1;

    update_lo_1_value: register_lo + 1;

    output_dst: md.count_1;
    output_value: alu_lo;
}

action update_count_1() {
    update_count_1_alu.execute_stateful_alu(0);
}

blackbox stateful_alu read_count_1_alu {
    reg: count_reg_1;

    output_dst: md.count_1;
    output_value: register_lo;
}

action read_count_1() {
    read_count_1_alu.execute_stateful_alu(0);
}

table update_count_1_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
    }
    actions {
        update_count_1;
        read_count_1;
    }
    default_action: read_count_1;
}

register count_reg_2 {
    width   : 32;
    instance_count  : 1;
}

blackbox stateful_alu update_count_2_alu {
    reg: count_reg_2;

    update_lo_1_value: register_lo + 1;

    output_dst: md.count_2;
    output_value: alu_lo;
}

action update_count_2() {
    update_count_2_alu.execute_stateful_alu(0);
}

blackbox stateful_alu read_count_2_alu {
    reg: count_reg_2;

    output_dst: md.count_2;
    output_value: register_lo;
}

action read_count_2() {
    read_count_2_alu.execute_stateful_alu(0);
}

table update_count_2_tab {
    reads {
        ib_aeth : valid;
        md.qpn : exact;
    }
    actions {
        update_count_2;
        read_count_2;
    }
    default_action: read_count_2;
}

/* compare count_1 and count_2 */
action compare_count(){
    bit_xor(md.xor_count, md.count_1, md.count_2);
}

table compare_count_tab {
    reads {
        ib_aeth : valid;
    }
    actions {
        compare_count;
    }
    default_action: compare_count;
}

action set_rescount(){
    modify_field(md.res_count, 1);
}

table set_res_count_tab {
    reads {
        ib_aeth : valid;
        md.xor_count: exact;
    }
    actions {
        set_rescount;
        nop;
    }
    default_action: set_rescount;
}

/* Generate digest for comparing count*/
field_list count_field{
    md.count_1;
    md.count_2;
}

//action gen_count_digest(){
//    generate_digest(0, count_field);
//}

register check_count{
    width: 8;
    instance_count: 1;
}

blackbox stateful_alu update_count_alu {
    reg: check_count;

    update_lo_1_value: register_lo + 1;
}

action gen_count_digest() {
    update_count_alu.execute_stateful_alu(0);
}

table gen_count_digest_tab{
    reads {
        ib_aeth : valid;
        md.qpn: exact;
        ib_bth.dqpn: exact;
        md.res_count: exact;
    }
    actions {
        gen_count_digest;
        nop;
    }   
}

// Begin state transition table logic

/* table_1*/
// used for traverse end criteria
action set_end_bit(){
    modify_field(md.end_bit, 1);
}

table check_traverse_end_tab{
    reads {
        ib_aeth: valid;
        md.qpn: exact;
        md.aeth_addr_h: exact;
        md.aeth_addr_l: exact;
    }
    actions {
        set_end_bit;
    }
    size: 1024; // concurrency
}

/* table_2*/
// used for check null pointer
//
action set_null_bit(){
    modify_field(md.null_bit, 1);
}

action mod_qpn_dqpn(qpn, dqpn){
    modify_field(md.qpn, qpn);
    modify_field(ib_bth.dqpn, dqpn);
}

table check_null_tab{
    reads {
        ib_aeth: valid;
        md.qpn: exact;
        md.aeth_addr_h: exact;
        md.aeth_addr_l: exact;
    }
    actions {
        mod_qpn_dqpn;
//        set_null_bit;
    }
    size: 1024; // concurrency
}

table move_transfer_tab{
    reads {
        md.qpn: exact;
        md.null_bit: exact;
    }
    actions {
        mod_qpn_dqpn;
    }
    size: 1024;
}

table end_transfer_tab{
    reads {
        ib_aeth: valid;
        md.qpn: exact;
    }
    actions {
        mod_qpn_dqpn;
    }
    size: 1024; // concurrency
}

// used for checking iter's ending criteria
register max_entry{
    width: 32;
// concurrency    instance_count: 8;
    instance_count: 200;
}

// write the max_fd into the register
blackbox stateful_alu read_max_entry_alu {
    reg: max_entry;

    update_lo_1_value: md.aeth_addr_l;
}

action read_max_entry(idx) {
    read_max_entry_alu.execute_stateful_alu(idx);
}

blackbox stateful_alu read_const_length_alu {
    reg: max_entry;

    update_lo_1_value: md.max_len;
}

action read_const_length(idx) {
    read_const_length_alu.execute_stateful_alu(idx);
}

blackbox stateful_alu update_max_entry_alu {
    reg: max_entry;

    condition_lo: register_lo != 1;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: register_lo - 1;

    output_dst: md.iter_entry;
    output_value: predicate;
}

action update_max_entry(idx){
    update_max_entry_alu.execute_stateful_alu(idx);
}

table read_update_max_entry_tab{
    reads {
        ib_aeth: valid;
        md.qpn: exact;
    }
    actions {
        read_const_length;
        read_max_entry;
        update_max_entry;
    }
    size: 1024; // concurrency
}

/* table_3*/
/* For every returning packet, do state transfer first*/
action mod_dqpn(dqpn){
    modify_field(ib_bth.dqpn, dqpn);
}

table direct_transfer_tab{
    reads {
        ib_aeth: valid;
        md.qpn: exact;
//        md.null_bit: exact; // no check on check null
        md.end_bit: exact;
        md.iter_entry: exact;
    }
    actions {
        mod_dqpn;
    }
    size: 1024; // concurrency
}

action cache_size_into_md(entry_size){
    modify_field(md.entry_size, entry_size);
//    modify_field(md.max_len, max_len);
}

table cache_size_into_md_tab{
    reads {
        md.qpn: exact;
    }
    actions {
        cache_size_into_md; 
    }
    size: 1024; // concurrency
}

action cache_len_into_md(max_len){
//    modify_field(md.entry_size, entry_size);
    modify_field(md.max_len, max_len);
}

table cache_len_into_md_tab{
    reads {
        md.qpn: exact;
    }
    actions {
        cache_len_into_md;
    }
    size: 1024; // concurrency
}

// modify QPN to page table walk QPN
action pgt_mod_dqpn(dqpn, qpn){
    modify_field(md.qpn, qpn);
    modify_field(ib_bth.dqpn, dqpn);
}

table pgt_transfer_tab{
    reads {
        ib_aeth: valid;
        md.qpn: exact;
        md.vmalloc_bit: exact;
    }
    actions {
        pgt_mod_dqpn;
    }
    size: 1024; // concurrency
}


// check if page table walk should be involved
action mark_vmalloc_bit() {
    modify_field(md.vmalloc_bit, 1);
}

table mark_vmalloc_bit_p2_tab{
    reads {
        ib_aeth : valid;
//        md.qpn: exact;
//        md.aeth_addr_h : ternary;
        md.addr_type: exact;
        ib_payload_8.payLoad_4: range;
        ib_payload_8.payLoad_6: range;
    }
    actions {
        mark_vmalloc_bit;
    }
    size: 1024; // concurrency
}

action mark_addr_type(tp){
    modify_field(md.addr_type, tp);
}

table mark_vmalloc_bit_p1_tab{
    reads {
        ib_aeth : valid;
        md.qpn: exact;
        md.aeth_addr_h : ternary;
    }
    actions {
        mark_addr_type;
    }
    size: 1024; // concurrency
}



// mark end of page table walk bit, when QPN == V4, marked end of QPN
// TODO: Adding walking bit logic.
action mark_walking_bit(){
    modify_field(md.walking_bit, 1);
}

table mark_walking_bit_tab{
    reads {
        ib_aeth: valid;
        ib_bth.dqpn: exact;
    }
    actions {
        mark_walking_bit;
    }
    size: 1024; // concurrency
}

// for testing and end the pipeline
register cnt{
    width: 16;
    instance_count: 1;
}

blackbox stateful_alu update_cnt_alu {
    reg: cnt;

    update_lo_1_value: register_lo + 1;
    output_dst: md.test_cnt;
    output_value: alu_lo;
}

action update_cnt() {
    update_cnt_alu.execute_stateful_alu(0);
}

table update_cnt_tab{
    reads{
        ib_aeth: valid;
    }
    actions{
        update_cnt;
    }
    size: 1024; // concurrency
}

action mod_cnt(cnt){
    modify_field(md.cnt, cnt);
}

table mod_cnt_table{
    reads{
        ib_aeth: valid;
    }
    actions{
        mod_cnt;
    }
}

table debug_tab{
    reads {
        md.test_cnt : exact;
    }
    actions {
        _drop;
    }
}

// clone related table
action mod_res_udp(entry) {
    modify_field(udp.dstPort, entry);
}

table mod_res_udp_tab{
    reads {
//        ib_aeth: valid;
        eg_intr_md_from_parser_aux.clone_src: exact;
    }
    actions {
        mod_res_udp;
    }
    size: 10;
}

action remove_clone_aeth() {
    remove_header(ib_aeth);
}

table remove_clone_aeth_tab {
    reads {
        ib_aeth: valid;
        eg_intr_md_from_parser_aux.clone_src: exact;
    }
    actions {
        remove_clone_aeth;
    }
}

action add_aeth() {
    add_header(ib_aeth);
}

table add_aeth_tab {
    reads {
//        ib_aeth: valid;
        eg_intr_md_from_parser_aux.clone_src: exact;
    }
    actions {
        add_aeth;
    }
}

action debug_iter(){
    modify_field(ib_bth.tver, md.iter_entry);
}

table debug_iter_tab{
    reads {
        ib_aeth: valid;
    }
    actions {
        debug_iter;
    }
}

/* obtain timestamps */

// Use high 32-bit tstamp of the full 48 bits
field_list tstamp {
    _ingress_global_tstamp_;
}

field_list_calculation tstamp_high32 {
    input {
        tstamp;
    }
    algorithm : identity_lsb;
    output_width : 32;
}

action split_tstamp_high32 () {
    modify_field_with_hash_based_offset(md.tstamp, 0, tstamp_high32, 0x100000000);
}

table split_tstamp_high32_tab {
    actions{
        split_tstamp_high32;
    }
    default_action : split_tstamp_high32;
    size : 1;
}

register ts_start{
    width: 32;
    instance_count: 500;
}

blackbox stateful_alu read_update_ts_start_alu {
    reg : ts_start;

    condition_lo: true;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: md.tstamp;
}

// for defense
//action read_update_ts_start(idx){
//    read_update_ts_start_alu.execute_stateful_alu(idx);
//}

// for time measure
action read_update_ts_start(){
    read_update_ts_start_alu.execute_stateful_alu(0);
}

table read_update_ts_start_tab{
    reads {
        ib_aeth: valid;
//        md.qpn : exact;
        ib_bth.dqpn: exact;
//        md.sign: exact;
    }
    actions {
        read_update_ts_start;
    }
    size: 1024;
}

register ts_end{
    width: 32;
    instance_count: 1;
}

blackbox stateful_alu read_update_ts_end_alu {
    reg : ts_end;

    condition_lo: true;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: md.tstamp;
}

action read_update_ts_end(){
    read_update_ts_end_alu.execute_stateful_alu(0);
}

table read_update_ts_end_tab{
    reads{
        ib_aeth : valid;
    }
    actions {
        read_update_ts_end;
    }
}

register toggle_start{
    width : 8;
    instance_count : 1;
}

blackbox stateful_alu read_update_toggle_start_alu {
    reg : toggle_start;

    condition_lo: true;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: 1;

    output_dst: md.sign;
    output_value: register_lo;
}

action read_update_toggle_start(){
    read_update_toggle_start_alu.execute_stateful_alu(0);
}

table read_update_toggle_start_tab{
    reads {
        ib_aeth: valid;
        md.qpn : exact;
    }
    actions {
        read_update_toggle_start;
    }
    size: 1024;
}

action magic_rev(){
    swap(ethernet.dstAddr, ethernet.srcAddr);
//    modify_field(ig_intr_md_for_tm.ucast_egress_port, portin);
//    modify_field(ig_intr_md.ingress_port, portout);    
}

table magic_rev_tab{
    reads {
        ib_aeth: valid;
        udp.srcPort : exact;
    }
    actions{
        magic_rev;
    }
    size: 10;
}

action magic_set(eth){
    modify_field(ethernet.dstAddr, eth);
}
table magic_set_tab{
    reads {
        ib_aeth: valid;
//        udp.srcPort : exact;
    }
    actions{
        magic_set;
    }
    size: 10;
}


table magic_forward_tab {
    reads {
        ib_aeth        : valid;
        ipv4           : valid;
        ipv4.dstAddr   : exact;
        md.spoof       : exact;
    }
    actions {
        set_egr;
    }
    size: 1024; // concurrency
}


// ingress pipeline logic:
// 1. Endian translation
// 2. Adress type: <1>linear; <2>vmalloc
// 3. If vmalloc: cache addr; cache qpn; modify qpn to special qpn; address translation
//          qpn --> v1, v2, v3, v4. Reread qpn

control ingress {
// defense
//    apply(read_update_psn_def_tab);
//    if (ib_bth.psn != md.psn_def){
//        apply(gen_spoof_alarm_tab);
//    }
// for measuring time 
    apply(split_tstamp_high32_tab);
//    apply(mod_cnt_table);
// for non-RDMI packet: simple forwarding
    apply(simple_forward_tab);
// handling ib_test non_ip packet
    apply(mac_forward_tab);
// apply magic table for 1st packet
    apply(magic_rev_tab);
//    apply(magic_set_tab);
// debug    apply(update_cnt_tab);
//    if (md.test_cnt == md.cnt){
//        apply(drop_tab);
//    }
// debug    apply(debug_tab);

// debug    apply(cache_ddqpn_ent_1_tab);
// debug    apply(cache_qqpn_ent_1_tab);
    apply(mark_walking_bit_tab);
//    if (md.qpn == 285){
//        apply(drop_tab);
//    }
//    if (md.walking_bit == 1){
//        apply(drop_tab);
//    }
    
//    apply(cache_parameter_into_md_tab); // TODO: merge table
    apply(cache_len_into_md_tab);

// Reverse the endian and store the address into md.aeth_addr
// For every packet with payload len == 8
    apply(split_addr_high32_tab);
    apply(split_addr_low32_tab);

// for measuring time
    apply(read_update_ts_start_tab);
    apply(read_update_ts_end_tab);
// If the packet needs to be cloned or not? If so Clone_i_to_e
    apply(cloning_tab);

// Split address to 2 16 bits metadata for range checking
    apply(split_addr_low16_tab);
    apply(split_addr_high16_tab);

// Address type checking happens here
    apply(mark_vmalloc_bit_p1_tab);
    apply(mark_vmalloc_bit_p2_tab);

// range checking logic
    apply(exact_match_tab); // Check if address is fallen to range border. Mark md.k1.
    apply(range_match_tab); // Check if address is at middle of the border. Mark md.k2.
//    apply(gen_range_digest_tab);

// count logic: not needed anymore?
//    apply(update_count_1_tab);
//    apply(update_count_2_tab);
//    apply(compare_count_tab);
//    apply(set_res_count_tab);

// 1. only the first address with ffffffffc* will be cached
//      rest will be read.
//    apply(walk_qpn_dqpn_tab);

//    apply(finish_walk_tab);

// debug    apply(cache_qqpn_tab);
// debug    apply(cache_ddqpn_tab);
// High level sequence:
//  1. Check input, set flag.
//  2. Use the flag for transferring state
//  3. Depend(and only depend) on the state to choose actions and parameters
// packet content checking table, which will be placed in the same stage
    apply(end_transfer_tab);  // transfer from end load
    apply(check_null_tab);  // result in end_bit
//    apply(move_transfer_tab);
    // check null tab mast be put before the two iter tab
    apply(read_update_max_entry_tab);
    apply(check_traverse_end_tab);
// state transtion table placed here, transit old qpn to new qpn
    apply(direct_transfer_tab); 
// debug    apply(cache_ddqpn_ent_2_tab);
// debug    apply(cache_qqpn_ent_2_tab);
// Checking input
//    apply(check_null_pointer_tab);

//    apply(debug_iter_tab);

//    apply(gen_count_digest_tab);
// Check whether the end criteria is met or not, if so, drop
    apply(end_of_fetching_tab);

// cache the qpn and dqpn here
    apply(cache_dqpn_page_walk_tab); // the sequence cannot be reversed!
    apply(cache_qpn_page_walk_tab);
//    apply(cache_ddqpn_ent_2_tab);
//    apply(cache_qqpn_ent_2_tab);
// Encode page table transition rule:
// If vmalloc_bit == 1: need walking. QPN_o --> V1, V1 --> V2, V2 --> V3, V3 --> V4
//      Packet with QPN==V4 needs to change back to QPN_o by reading out reg_qpn
//      Packet with QPN==V4 needs to be specially marked
    apply(pgt_transfer_tab);


//    apply(change_port_to_control_tab);

    apply(forward_rnic_tab);
// move to egress
//    apply(cache_size_into_md_tab);
    apply(encode_mod_offset_pre_tab);
    apply(mod_field_parameters_pre_tab);
    apply(magic_forward_tab);
// packet gen
    apply(magic_set_tab);
//    apply(cache_ddqpn_ent_3_tab);
//    apply(cache_qqpn_ent_3_tab);
}



control egress {
    // dealing cloned packet
    // apply(remove_clone_aeth_tab);

    apply(cache_size_into_md_tab);

//    apply(cache_ddqpn_ent_3_tab);
//    apply(cache_qqpn_ent_3_tab);

    apply(cache_process_page_addr_to_reg_h_tab); // Specify in which round the address is returned
    apply(cache_process_page_addr_to_reg_l_tab);

    apply(add_offset_1_tab);
    apply(add_offset_2_tab);
    apply(add_offset_3_tab);
    apply(mask_base_addr_tab);
    apply(make_up_addr_tab);


// Cache/read address to/from register here
    apply(cache_process_addr_to_reg_h_tab);
    apply(cache_process_addr_to_reg_l_tab);

// Address translation: We are using 3 tables because we
//      are storing address in 32bits
    apply(cache_offset_into_meta_tab);
    apply(trans_mode_tab);
    apply(split_addr_high1_tab); // put it to front
    apply(addr_translation_page_offset_tab);

// Add the reth header used for sending request
    apply(add_reth_tab);

// Modify the offset/length
    apply(encode_mod_offset_tab);
    apply(mod_field_parameters_tab);

// Put rkey inside the packet, put and update the PSN information
    apply(cache_rkey_tab);
    apply(read_update_psn_tab);

// Ready for sending out, modify flags, src/dst addr, port in header
    apply(set_bth_reth_flags_tab); // to ingress
    apply(modify_header_len_tab); // to ingress
    apply(read_addr_to_header_tab);

// Remove the aeth and Payload header
    apply(remove_payload_tab);
    apply(remove_aeth_tab);

    // handling cloned packet
//    apply(add_aeth_tab);
//    apply(mod_res_udp_tab);
    apply(gen_mali_alarm_tab);
}
