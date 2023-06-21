/* check keylogger  */
KernelGraph(keyboard_notifier_block)
.in(8)
.traverse(8, 0x0000000000000000, 0)
.values(0)
.assert(0xa08031d1, 0x9fc00000)
End
