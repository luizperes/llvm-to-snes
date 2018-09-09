# LLVM IR to the SNES plataform

> This project aims to transform LLVM IR into WLA DX (ASM 65c816) which would then assemble this output and generate a SNES ROM. Please read the links below to further information on SNES and LLVM plataforms.

* [SNES 65816 Reference](https://wiki.superfamicom.org/65816-reference)
* [LLVM Backend API](https://llvm.org/docs/WritingAnLLVMBackend.html)

The wiki's homepage for general information on SNES architecture (65c816)

* [Superfamicom's wiki](https://wiki.superfamicom.org/)

# How to test it
Install it locally:
  - `cd </path/to/llvm-to-snes/>`
  - `makedir build-snes && cd build-snes`
  - `cmake -DLLVM_TARGETS_TO_BUILD=X86 . --build . -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SNES`
  - `make install` _// it might take a while_

Run: _// please read observations below_
  - `clang your_filename.c -S -emit-llvm -o your_filename.ll`
  - `<path/to/llvm-to-snes>/build-snes/bin/llc -march=snes <your_filename>.(ll|bc)`

###### Obs.1
You may test it with any language that compiles to llvm, or you might write it yourself by hand, as long as the file has LLVM extensions `.ll` or `.bc`.

###### Obs.2
SNES (65c816) has a 16-bit architecture and this project up-to-date does not support 32 or 64 bit. A good example for a file test would be:

```C
// sum_file.c
short sum(short a, short b) {
  return a + b;
}
```

###### Obs.3
You may find C files for testing under `./test/Examples/SNES`.

###### Obs.4
The files for this project can be found inside the folder `./lib/Target/SNES/`. We have forked the whole LLVM because we have plans of using Docker containers in the future for such tasks.