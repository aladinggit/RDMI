import socket, struct, pickle, os
from collections import defaultdict
import math
def unsigned_to_signed(val, bitwidth):
    if val >= 2**(bitwidth-1):
        return val - 2**bitwidth
    else:
        return val

# rules for writing rKey and QPN:

class simple(object):
    def __init__(self):
	pass
    def init(self):
	pass
    def read_registers(self):
        flag = p4_pd.register_flags_t(1)
        mirror.session_create(
            mirror.MirrorSessionInfo_t(
                mir_type=mirror.MirrorType_e.PD_MIRROR_TYPE_NORM,
                direction=mirror.Direction_e.PD_DIR_BOTH,
                mir_id=1,
                egr_port=0, egr_port_v=True,
                max_pkt_len=16384))
# starting message QPN needs to be itself - 1
        print("start")
#######################################################
#               rKEY reconfiguration                  #
#   replace the number(525497) with                   #
#   the rkey of the connection                        #
#######################################################
        p4_pd.register_write_rkey(0, 525497)

#######################################################
#               PSN reconfiguration                   #
#   Store the PSN inside the switch                   #
#   the sample is listed below                        #
#######################################################
        p4_pd.register_write_psn(0, unsigned_to_signed(14097393 + pow(2, 31), 32))
        p4_pd.register_write_psn(1, unsigned_to_signed(1683493 + pow(2, 31), 32))
