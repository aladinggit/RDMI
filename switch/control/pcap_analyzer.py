#!/usr/bin/env python
import sys
import struct
import os
from scapy.all import *
from scapy.all import sniff, sendp, hexdump, get_if_list, get_if_hwaddr
from scapy.all import Packet, IPOption
from scapy.all import ShortField, IntField, LongField, BitField, FieldListField, FieldLenField
from scapy.all import IP, TCP, UDP, Raw
from scapy.layers.inet import _IPOption_HDR

IPV4_PROTOCOL = 0x0800



class BTH(Packet):
    name = "BTH"
    fields_desc = [
        BitField("opcode", 0, 8),
        BitField("solicited", 0, 1),
        BitField("migreq", 0, 1),
        BitField("padcount", 0, 2),
        BitField("version", 0, 4),
        XShortField("pkey", 0xffff),
        BitField("fecn", 0, 1),
        BitField("becn", 0, 1),
        BitField("resv6", 0, 6),
        BitField("dqpn", 0, 24),
        BitField("ackreq", 0, 1),
        BitField("resv7", 0, 7),
        BitField("psn", 0, 24),
	]

class RETH(Packet):
    name = "RETH"
    fields_desc = [
        BitField("virtAddr_h", 0, 32),
        BitField("virtAddr_l", 0, 32),
        BitField("rkey", 0, 32),
        BitField("len", 0, 32),
    ]


class AETH_2(Packet):
    name = "AETH_2"
    fields_desc = [
        XByteField("syndrome", 0),
        XBitField("msn", 0, 24),
        BitField("payLoad", 0, 32),
    ]



bind_layers(BTH, AETH_2, opcode = 16)
bind_layers(BTH, RETH, opcode = 12)
bind_layers(UDP, BTH, dport=4791)
#bind_layers(UDP, BTH, dport=20)

def handle_pkt(pkt):
    if UDP in pkt and pkt[UDP].dport == 4791:
        print "got a packet"
        pkt.show2()
    #    hexdump(pkt)
        sys.stdout.flush()
def get_packet_batch_count(pcap_path):
    scapy_cap = rdpcap(pcap_path, 100000)
    type1 = []
    type3 = []
    tset = {0, 1} 
    this_batch = 0
    current_len = 0
    count = 0
    base = 0.25
    for packet in scapy_cap:
        if 1:
        #if packet["BTH"].dqpn == 0xc87 or packet["BTH"].dqpn == 0xc85:
            print('time %.9f' % packet.time)
            
            ###idn = '{:08x}'.format(packet["AETH_2"].payLoad)
            #print (idn)
            ###idn = idn[4:6] + idn[2:4] + idn[0: 2] 
            ###print(idn)
            ###print "pid is ",  int(idn, 16)
            #            print("pid is ", (hex(packet["AETH_2"].payLoad)[4:6]+
#                    hex(packet["AETH_2"].payLoad)[2:4]))
            #tset.add(packet["AETH_2"].payLoad)
        #if packet["BTH"].dqpn == 0x12f:
            #print(hex(packet["AETH_2"].payLoad), "type2")
        if packet["BTH"].dqpn == 0x10e:
            num = '{:08x}'.format(packet["AETH_2"].payLoad)
            num = num[4:6] + num[2:4] + num[0: 2]
            print("file number is ", int(num, 16))
            #print("nr file", str(int(hex(packet["AETH_2"].payLoad)[4:6], 16))+
            #        str(int(hex(packet["AETH_2"].payLoad)[2:4], 16)))
        if packet["BTH"].dqpn == 0x112:
            name = '{:08x}'.format(packet["AETH_2"].payLoad)
            #name = name[6:8] + name[4:6] + name[2:4] + name[0: 2]
            #print(name)
            print("file name is ", chr(int(name[0:2], 16))
                    + chr(int(name[2:4],16)) + chr(int(name[4:6], 16)) 
                    + chr(int(name[6:8], 16)))
            #print(chr(int((hex(packet["AETH_2"].payLoad)[2:4]), 16)))
            #print(chr(int((hex(packet["AETH_2"].payLoad)[2:4]), 16)) +
            #        chr(int((hex(packet["AETH_2"].payLoad)[4:6]), 16)) +
            #        chr(int((hex(packet["AETH_2"].payLoad)[6:8]), 16)) +
            #        chr(int((hex(packet["AETH_2"].payLoad)[8:10]), 16))) 
                 
    return tset

def check_sequential(pcap_path):
    scapy_cap = rdpcap(pcap_path, 50000)

    idd = 0
    idd2 = 0
    toggle = 0
    for packet in scapy_cap:
        if packet["BTH"].opcode == 0x10:
            #print("ip", packet["IP"].id)
            if toggle == 0:
                idd = packet["IP"].id - 1
                toggle = 1
            idd2 = packet["IP"].id
            if idd2 != idd + 1:
                print("error  ", idd2, packet["BTH"].dqpn)
            idd = idd2
            

def main():
    get_packet_batch_count("/home/hongyi/tcpdump/spoof_afinfo_con_break6.pcap")
#    global show
    #set1 = get_packet_batch_count("/home/hongyi/tcpdump/dos_link_fs_multi_int_3.pcap");
    #set2 = get_packet_batch_count("/home/hongyi/tcpdump/dos_link_fs_multi_int_2.pcap");
#    check_sequential("/home/hongyi/tcpdump/dos_link_fs_multi_int_3.pcap")
#    for item in set2:
#        if item in set1:
#            continue
#        else:
#            print(hex(item))
#    iface = "bf_pci0" 
#    print "sniffing on %s" % iface
#    sys.stdout.flush()
#    sniff(iface = iface,
#          prn = lambda x: handle_pkt(x))



if __name__ == '__main__':
    main()
