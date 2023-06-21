/* Fetch opened file name */
KernelGraph(init_task)
//.in(1960)
.traverse(1960, 0xffffffffa1013c28, 1960)
.values(2216)
.in(2704)
.in(32)
.values(@num, 0)
.in(8)
// Iterate the array fd that contains file pointers
.iter(0, num, 8)
.in(0)
.in(24)
// Fetch the name of each file
.values(56)
End
