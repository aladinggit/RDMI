parser start {
    return parse_ethernet;
}

#define ETHERTYPE_VLAN 0x8100
#define ETHERTYPE_IPV4 0x0800

parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        ETHERTYPE_VLAN : parse_vlan;
        ETHERTYPE_IPV4 : parse_ipv4;
        default: ingress;
    }
}

#define IP_PROTOCOLS_UDP 17

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol) {
        IP_PROTOCOLS_UDP : parse_udp;
        default: ingress;
    }
}

parser parse_vlan {
    extract(vlan);
    return select(vlan.etherType) {
        // ETHERTYPE_VLAN : parse_vlan;
        ETHERTYPE_IPV4 : parse_ipv4;
        default: ingress;
    }
}

#define UDP_PORT_IB 4791
#define UDP_PORT_LOG 12345
#define UDP_PORT_CTRL 12347
#define UDP_PORT_TMP_CRC 12348
#define UDP_PORT_AUTH_CTRL 12349

parser parse_udp {
    extract(udp);
    return select(udp.dstPort) {
       UDP_PORT_IB        : parse_ib_bth;
       default: ingress;
    }
}

//parser parse_tmp_crc {
//    extract(tmp_crc);
//    return parse_ib_bth;
//}

#define IB_OPCODE_RC_WRITE_ONLY  10
#define IB_OPCODE_RC_WRITE_FIRST 6
#define IB_OPCODE_RC_READ_REQ    12
#define IB_OPCODE_RC_SEND_ONLY 4
#define IB_OPCODE_UD_SEND_ONLY 100

#define IB_OPCODE_RC_READ_RESPONSE 16
#define IB_OPCODE_RC_ACK 17


parser parse_ib_bth {
    extract(ib_bth);
    set_metadata(md.qpn, ib_bth.dqpn);
    return select (ib_bth.opCode) {
      IB_OPCODE_RC_READ_REQ    : parse_ib_reth;
      IB_OPCODE_RC_READ_RESPONSE : parse_ib_aeth;
      IB_OPCODE_UD_SEND_ONLY   : parse_ack_malform_deth;
      IB_OPCODE_RC_ACK         : parse_ack_malform_deth;
      IB_OPCODE_RC_SEND_ONLY   : parse_ack_malform_deth;
      default: parse_ack_malform_deth;
    }
}

parser parse_ack_malform_deth {
    set_metadata(md.etc, 1);
    return ingress;
}

parser parse_ib_reth {
    extract(ib_reth);
    set_metadata(md.etc, 1);
//    set_metadata(md.reth_mapbit, 1);
//    set_metadata(md.process_addr_h, ib_reth.virtAddr_h);
//    set_metadata(md.process_addr_l, ib_reth.virtAddr_l);
//    set_metadata(md.pid_seq, 0);
//    set_metadata(md.rkey, ib_reth.rkey);
    return ingress;
}

// Use UDP packet length to calculate the payload of RDMA information
parser parse_ib_aeth {
    extract(ib_aeth);
//    set_metadata(md.aeth_mapbit, 1);
    return select(udp.hdr_length) {
        28: ingress;
        32: parse_ib_payload_4;
        36: parse_ib_payload_8;
        44: parse_ib_payload_16;
        60: parse_ib_payload_32;
    default: ingress;
    }
}


parser parse_ib_payload_4 {
    set_metadata(md.len, 4);
    extract(ib_payload_4);
    return ingress;
}

parser parse_ib_payload_8 {
    set_metadata(md.len, 8);
    extract(ib_payload_8);
    return ingress;
//    return parse_icrc;
}

parser parse_ib_payload_16 {
    set_metadata(md.len, 16);
    extract(ib_payload_16);
    return ingress;
}

parser parse_ib_payload_32 {
    set_metadata(md.len, 32);
    extract(ib_payload_32);
    return ingress;
}

//parser parse_icrc {
//    extract(ib_icrc);
//    return ingress;
//}
//parser parse_ib_deth {
//    extract(ib_deth);
//    set_metadata(md.deth_mapbit, 1);
//    return ingress;
//}
//
//parser parse_ib_mad {
//    extract(ib_mad);
//    return ingress;
//}
//
//parser parse_log_header {
//    extract(log_header);
//    return ingress;
//}
//
//parser parse_ctrl {
//    extract(ctrl);
//    return ingress;
//}
//
//parser parse_authCtrl {
//    extract(authCtrl);
//    return ingress;
//}
