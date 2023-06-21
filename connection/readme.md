# Connection

This directory is used for establishing RDMA connection needed for RDMI.

Disable iCRC: We get rid of recomputing iCRC by disabling the iCRC check temporarily on our RNICs.

The NIC driver installation is specified [here](https://enterprise-support.nvidia.com/s/article/howto-install-mlnx-ofed-driver). RDMI requires to enable physical address memory region. The link [here](https://enterprise-support.nvidia.com/s/article/howto-compile-mlnx-ofed-drivers--160---mlnx-ofa-kernel-example-x) specifies how to recompile the driver with flags for achieving this.

## To compile the program
```
make rdmas // compile the server side program
make rdmac // compile the client side program

```

## To establish the connection

Assume server(introspected side) use IP 192.168.1.9, client(remote side) use IP 192.168.1.1

The detailed parameter specification can be found inside the source program

In the below command, -a 192.168.1.9 specifies the server side IP. -n 10 specifies the number of connection to establish,
User can modify the number and create a number of queue pair needed in their experiments.
```
sudo ./rdmatry_server -a 192.168.1.9 -n 10 -m 1 -M 1000000000 -d 0  // run on introspected side
./rdmatry_client -a 192.168.1.9 -n 10 -M 1000000000 -r 10000000 -c 1 -t 99999999999 -p 1 // run on remote side
```

## Meta data of the connection information

Connection metadata will be further used by other components in RDMI. We provided some tools for checking and parsing
the metadata.

``obj_addr_key.txt`` will contain the rkey of the connection.

To check the PSN information and encode them in switch configuration file(please check ../switch/master/bfshell/simple.py for specific encoding instructions).

parse.py will help to generate the format used for encoding PSN informations.
```
./rdma res show qp > read.txt
python parse.py // This script will generate the PSN information for each QPN inside new.txt. User can insert
                // the generated configurations and insert them into corresponding position inside ../switch/master/bfshell/simple.py
```
