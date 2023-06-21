pd-master
pd simple_forward_tab add_entry set_egr ipv4_valid 1 ipv4_dstAddr 192.168.1.1 action_egress_spec 48
pd simple_forward_tab add_entry set_egr ipv4_valid 1 ipv4_dstAddr 192.168.1.9 action_egress_spec 15
pd simple_forward_tab add_entry set_egr ipv4_valid 1 ipv4_dstAddr 192.168.1.3 action_egress_spec 49
exit
pd simple_forward_tab add_entry set_egr md_etc 1 ipv4_valid 1 ipv4_dstAddr 192.168.1.1 action_egress_spec 48
pd simple_forward_tab add_entry set_egr md_etc 1 ipv4_valid 1 ipv4_dstAddr 192.168.1.9 action_egress_spec 15
exit
