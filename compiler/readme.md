# Compiler

This directory contains the implementation of the compiler. It also includes the policy dsl used for encoding the introspection logic.

``raw_policy/policy_pool.dsl`` contains 11 policies used for introspection.

The front_parser parses the raw policy and extracts data structure and further translate them into specific offsets.
``datastruct.json`` contains data structure layout used in those policies. 

To compile the compiler program
```
make
```
This command will generate an executable named RDMI.

To compile the dsl into configuration files:
```
# Copy the policy inside policy_pool into raw_policy/sample.dsl

python front_parser.py // generate the dsl with concrete offsets encoded.

# To further translate the dsl into specific switch entries, copy the dsl with specific offsets into exe/policy0.c

./RDMI QPN_l QPN_r 1 // QPN_l denotes the starting local QPN and QPN_r denotes the starting remote QPN in server side.
                     // QPN meta data information will be available after the connection is established.
```
 The code will be generated into ``gencode`` directory.

To install multiple policies into the switch:
```
# put each policy to add on into exe/policy1.c, exe/policy2.c ...

./RDMI QPN_1 QPN_2 NUM // Num denotes for the number of policies to be installed.
```
