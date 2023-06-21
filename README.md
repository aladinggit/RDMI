# Remote Direct Memory Introspection

## Overview

RDMI develops a defense system targetting for memory introspection, leveraging
programmable data planes and RDMA NICs. The RDMI compiler compiles the policies specified
in domain specific language into lower level configurations. The master P4 switch program 
takes in the configurations and enforce the introspection policies for different security tasks.
This repo contains implementation of the system. Please refer to each ``readme`` under those subdirectories for more
informations.

## Compiler

The ``compiler`` directory contains the implementation of the compiler. It also includes the policy dsl used for 
encoding the introspection logic. 

## Switch

The ``switch`` directory contains the master P4 program as well as control rules and triggers of the introspection. 

## Connection

The ``connection`` directory contains the connection setup program for establish connections. 

## Experimental workflows:

1. Establish the RDMA connections(refer to ``connection``).
2. Compile the policy and generate the corresponding configuration files(refer to ``compiler``).
3. Configure the switch and run the program(refer to ``switch``).

## Reference implementations

Some of the implementation used in this repo is based on existing open-source
project, including [redmark](https://github.com/spcl/redmark.git),
[Pythia](https://github.com/WukLab/Pythia.git), [SCADET](https://github.com/sabbaghm/SCADET.git),
[Bedrock](https://github.com/alex1230608/Bedrock.git) and some examples codes provided in Tofino switch SDE.

## License
The code is released under the [MIT License](https://opensource.org/license/mit/).
