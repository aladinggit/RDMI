import socket, struct, pickle, os
from collections import defaultdict
import math
import time
def unsigned_to_signed(val, bitwidth):
    if val >= 2**(bitwidth-1):
        return val - 2**bitwidth
    else:
        return val



# rules for writing rKey and QPN:
#   from ds01, write 3 sq-psn sequentially with signed number

class query(object):
    def __init__(self):
	pass
    def init(self):
	pass
    def read_registers(self):
        flag = p4_pd.register_flags_t(1)
        print("time stamp")
        i1 = 0         
        i2 = 0
        i3 = 0
        i4 = 0
        i5 = 0
        i6 = 0
        i7 = 0
        i8 = 0
        i9 = 0
        i10 = 0
        i11 = 0
        i12 = 0
        for ic in range(3000):
            print(ic, "round")
            k1 = p4_pd.register_read_ts_start(0, flag)
            k2 = p4_pd.register_read_ts_start(1, flag)
            k3 = p4_pd.register_read_ts_start(2, flag)
            k4 = p4_pd.register_read_ts_start(3, flag)
            k5 = p4_pd.register_read_ts_start(4, flag)
            k6 = p4_pd.register_read_ts_start(5, flag)
            k7 = p4_pd.register_read_ts_start(6, flag)
            k8 = p4_pd.register_read_ts_start(7, flag)
            k9 = p4_pd.register_read_ts_start(13, flag)
            k10 = p4_pd.register_read_ts_start(14, flag)
            k11 = p4_pd.register_read_ts_start(15, flag)
            k12 = p4_pd.register_read_ts_start(16, flag)
            k13 = p4_pd.register_read_ts_start(17, flag)
            k14 = p4_pd.register_read_ts_start(18, flag)
            k15 = p4_pd.register_read_ts_start(19, flag)
            if k1[0] == i1:
                print("en1 down")
            if k2[0] == i2:
                print("en2 down")
            if k3[0] == i3:
                print("en3 down")
            if k4[0] == i4:
                print("en4 down")
            if k5[0] == i5:
                print("en5 down")
            if k6[0] == i6:
                print("en6 down")
            if k7[0] == i7:
                print("en7 down")
            if k8[0] == i8:
                print("en8 down")
            if k9[0] == i9:
                print("en9 down")
            if k10[0] == i10:
                print("en10 down")
            if k11[0] == i11:
                print("en11 down")
            time.sleep(0.1)
            i1 = k1[0]
            i2 = k2[0]
            i3 = k3[0]
            i4 = k4[0]
            i5 = k5[0]
            i6 = k6[0]
            i7 = k7[0]
            i8 = k8[0]
            i9 = k9[0]
            i10 = k10[0]
            i11 = k11[0]
            i12 = k12[0]
            i13 = k13[0]
            i14 = k14[0]
            i15 = k15[0]
