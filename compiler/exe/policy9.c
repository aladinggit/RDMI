/* Linux afinfo ops verification */
KernelGraph(tcp4_seq_afinfo)
.values(24, 32, 48)
.assert(0xa08031d1, 0x9fc00000)
.in(16)
.values(24, 32)
.assert(0xa08031d1, 0x9fc00000)
End
