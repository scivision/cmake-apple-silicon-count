# cmake-hwloc-apple

Workarounds for Apple Silicon CPU detection with hwloc and CMake.
For MPI projects and others where physical CPU count needs to be accurately known, and where one wishes only to use the "fast" cores, 
[hwloc](https://www.open-mpi.org/projects/hwloc/)
would normally be used instead.

The C code was 
[originally provided](https://github.com/open-mpi/hwloc/issues/454#issuecomment-819436128)
by @fozog and we subsequently modified it.

In our real projects, we handle other architectures similarly using hwloc.
To keep this example minimal, we only show MacOS handling.

## Usage

```sh
cmake -B build
```

If on Apple Silicon CPU, this will print the number of "fast" CPU cores detected.
