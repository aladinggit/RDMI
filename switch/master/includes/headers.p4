/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Template headers.p4 file for basic_switching
// Edit this file as needed for your P4 program

// Here's an ethernet header to get started.

header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        etherType : 16;
    }
}

header ethernet_t ethernet;

header_type vlan_tag_t {
    fields {
        pcp       : 3;
        cfi       : 1;
        vid       : 12;
        etherType : 16;
    }
}

header vlan_tag_t vlan;

header_type ipv4_t {
    fields {
        version        : 4;
        ihl            : 4;
        diffserv       : 8;
        totalLen       : 16;
        identification : 16;
        flags          : 3;
        fragOffset     : 13;
        ttl            : 8;
        protocol       : 8;
        hdrChecksum    : 16;
        srcAddr        : 32;
        dstAddr        : 32;
    }
}

header ipv4_t ipv4;

header_type udp_t {
    fields {
        srcPort    : 16;
        dstPort    : 16;
        hdr_length : 16;
        checksum   : 16;
    }
}

header udp_t udp;

header_type ib_bth_t {
   fields {
      opCode    : 8;
      se        : 1;
      migReq    : 1;
      padCnt    : 2;
      tver      : 4;
      p_key     : 16;
//      reserved  : 8;
// merge reserved bits with dqpn to form a 32bits qpn field
      dqpn      : 32;
//      ack_req   : 1;
//      reserved2 : 7;
//      psn_h     : 8;
// We have to merge ack_req with psn, otherwise switch doesn't work
      psn     : 32;
   }
}

//@pragma pa_atomic ingress ib_bth.psn
//@pragma pa_no_tagalong ingress ib_bth.reserved2
//@pragma pa_mutually_exclusive ingress ib_bth.psn ib_bth.reserved2
//@pragma pa_solitary ingress ib_bth.psn
//@pragma pa_atomic ingress ib_bth.reserved2
header ib_bth_t ib_bth;

header_type ib_reth_t {
   fields {
      virtAddr_h : 32;
      virtAddr_l : 32;
      rkey       : 32;
      len        : 32;
   }
}

header ib_reth_t ib_reth;

header_type ib_deth_t {
   fields {
      qKey     : 32;
      reserved : 8;
      srcQp    : 24;
   }
}

header ib_deth_t ib_deth;

header_type ib_mad_t {
   fields {
      base_version  : 8;
      mgmt_class    : 8;
      class_version : 8;
      mad_method    : 8;
      status        : 16;
      specific      : 16;
      trans_id      : 64;
      attr_id       : 16;
      reserved      : 16;
      modifier      : 32;
   }
}

header ib_mad_t ib_mad;

header_type ib_payload_4_t {
    fields {
        payLoad_1 : 8;
        payLoad_2 : 8;
        payLoad_3 : 8;
        payLoad_4 : 8;
    }   
}
header ib_payload_4_t ib_payload_4;

header_type ib_payload_8_t {
    fields {
        payLoad_1 : 8;
	    payLoad_2 : 8;
    	payLoad_3 : 8;
        payLoad_4 : 8;
        payLoad_5 : 8;
        payLoad_6 : 8;
        payLoad_7 : 8;
        payLoad_8 : 8;
    }
}
header ib_payload_8_t ib_payload_8;

header_type ib_payload_16_t {
    fields {
        payLoad : 128;
    }
}
header ib_payload_16_t ib_payload_16;

header_type ib_payload_32_t {
    fields {
        payLoad_1 : 128;
        payLoad_2 : 128;
    }
}
header ib_payload_32_t ib_payload_32;

header_type ib_aeth_t {
    fields {
        ack : 8;
        MSN : 24;
    }
}
header ib_aeth_t ib_aeth;

//header_type ib_icrc_t {
//    fields {
//        icrc_v : 32;
//    }
//}
//header ib_icrc_t ib_icrc;

header_type md_t {
   fields {
      offset_1: 32;
      offset_2: 32;
      offset_3: 32;
      offset_4: 32;
      offset_5: 32;
//      rkey : 32;
      aeth_addr_l : 32;
      aeth_addr_h : 32;
//      psn: 32;
      len : 8;
//      pid_seq : 8;
//      pid_seq_1 : 8; // recording round
//      pid_seq_2 : 8;
      val_reg64 : 7;
      greater_4: 4;
      trans_mode: 2;
//      end_sig : 1;
      null: 1;
//      reth_mapbit : 1;
//      aeth_mapbit : 1;
      etc : 1;
      mod_value : 32;
      iter_entry: 4;
// now 32 bits
      qpn: 32;
// metadata for physical addr translation
      page_offset: 32;
      cached_addr_l: 32;
      cached_addr_h: 32;
      addr_l_16: 16;
      addr_h_16: 16;
      count_1: 32;
      count_2: 32;
      xor_count: 32;
      res_count: 1;
      k1: 1;
      k2: 1;
      null_bit: 1;
      end_bit: 1;
      max_len : 32;
      entry_size : 32;
// vmalloc allocation bits
      vmalloc_bit: 1;
      walking_bit: 1;
      test_cnt: 16;
      cnt: 16;
      addr_type : 2;
      tstamp: 32;
      sign: 8;
      psn_def: 32;
      spoof: 2;
   }
}

metadata md_t md;
@pragma pa_container ingress md.rdma_len 10 11
@pragma pa_container ingress md.rdma_end_l 12 13

