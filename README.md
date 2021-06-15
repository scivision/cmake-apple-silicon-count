# cmake-hwloc-apple

Workarounds for Apple Silicon CPU detection with hwloc and CMake.
For MPI projects and others where physical CPU count needs to be accurately known, and where one wishes only to use the "fast" cores, 
[hwloc](https://www.open-mpi.org/projects/hwloc/)
would normally be used.
However, hwloc 
[does not yet support](https://github.com/open-mpi/hwloc/issues/454) 
Apple Silicon accurately.
Thus we developed these workarounds for CMake-based projects.

The C code was 
[originally provided](https://github.com/open-mpi/hwloc/issues/454#issuecomment-819436128)
by @fozog and we subsequently modified it.
