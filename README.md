# LLVM IR to the SNES plataform

> This project aims to transform LLVM IR into WLA DX (ASM 65c816) which would be then further assembled to a SNES ROM. If you are lost and don't know barely anything from this project you could follow the links below to further information on the SNES plataform.

Obs.: All files for this project can be found inside the folder ./lib/Target/SNES/

* [SNES 65816 Reference](https://wiki.superfamicom.org/65816-reference)
* [LLVM Backend API](https://llvm.org/docs/WritingAnLLVMBackend.html)

The wiki's homepage for general information

* [Superfamicom's wiki](https://wiki.superfamicom.org/)

# How to test it

Run: `cmake -DLLVM_TARGETS_TO_BUILD=X86 . --build . -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=SNES`
