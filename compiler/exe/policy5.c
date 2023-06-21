/* Syscall verification */
KernelGraph(sys_call_table)
.iter(0, 500, 8)
.values(0)
.assert(0xa08031d1, 0x9fc00000)
End
