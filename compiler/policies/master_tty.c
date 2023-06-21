// Check Linux tty ops
KernelGraph(tty_drivers)
.in(0, @0, 168)
.traverse(168, 0xffffffffa1188520, 168)
.values(@num, 52)
.in(128)
.iter(0, num, 8)
.in(0)
.in(88)
.in(0)
.values(104, 128)
.assert(0xa08031d1, 0x9fc00000)
End
