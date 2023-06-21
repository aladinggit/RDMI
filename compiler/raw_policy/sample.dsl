kgraph(sys_call_table)
.iterate(this, 500, ptr)
.values(this)
.assert(0xffffffffa08031d1<this<0xffffffff9fc00000)