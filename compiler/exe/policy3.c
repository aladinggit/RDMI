/* Check netfilter hook functions */
KernelGraph(init_net)
// const iteration
.iter(3592, 104, 8) 
.in(0)
.values(@num, 0)
.iter(8, num, 16)
.values(0)
.assert(0xa08031d1, 0x9fc00000)
End
