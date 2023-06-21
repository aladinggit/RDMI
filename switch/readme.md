# Switch

This directory contains the master P4 program as well as control rules and triggers of the introspection.

## directory description

The ``master`` directory contains a master program of the RDMI. Master program can be configured by different configuration files generated from the compiler for enforcing policies. The ``control`` directory contains the program used for triggering.

## Experiment setup

Switch: Wedge 100BF-32X Tofino switch.
Barefoot SDE: bf-sde-8.4.0
Host OS: Ubuntu 18.0

## Run the code

### Step 1: Build and run the P4 program:
```
cd ~/bf-sde-8.4.0
source set_sde.bash // setup enviroment
./p4_build.sh master/master.p4 // compilation
./run_switchd.sh -p master
```

### Step 2: Enable all ports of the switch
```
ucli
pm
port-add -/- 25G NONE
port-enb -/-
```

### Step 3: config the control plane rule
```
# Run in another terminal in the switch

cd ~/bf-sde-8.4.0
source set_sde.bash
./run_bfshell.sh -f bfshell/setup_init.cmd // Enable the forwarding logic

# Use the metadata generated after setting up the RDMA connection(../connection)

./run_bfshell.sh -f bfshell/setup_qpn_ts.cmd // Enable the static runtime logic
./run_bfshell.sh -f code_gen.cmd // code_gen.cmd is the cmd file compiled from the policy to install the switch with introspection logic

# Next, change the rkey as well as encode the PSN inside the simple.py file(details check connection setup requirement as well as master/bfshell/simple.py). Then run:

~/tools/run_pd_rpc.py -p master bfshell/setup2.py  

```

### Step 4: Issue the trigger to start the introspection
```
# This part can be replaced with a packet generator.

cd control
make send

# Send the packet with corresponding policy number that you want to invoke

# The Policy_num below is corresponding to different starting address which can be modified and referenced inside send.cpp
sudo ./send dst_IP src_IP Policy_num 12345 0x1234 0 server_side_rQPN

#For example:
sudo ./send 192.168.1.9 192.168.1.1 5 12345 0x1234 0 5523 248

#To get the result, one can start a daemon for listening the packet in server side or forward
such packet to a remote logging server by configuring the switch control plane's forwarding rule. tcpdump can be used for tracking packets.

sudo tcpdump -i ens3f1 -s 50000 -w res.pcap // ens3f1 is the interface of the connection

# Note that the RDMA itself is bypassing the kernel. One can still config the NIC to sniff the packets using below command:
sudo ethtool --set-priv-flags ens3f1 sniffer on

```
