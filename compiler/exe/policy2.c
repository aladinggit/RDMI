/* Linux /proc verification */
KernelGraph(proc_root)
.in(40)
.values(8, 16, 24, 32, 40, 48, 56)
.assert(0xa08031d1, 0x9fc00000)
End
