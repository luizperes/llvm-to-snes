//===-- SNESCallingConv.td - Calling Conventions for SNES ----*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for SNES architecture.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// SNES Return Value Calling Convention
//===----------------------------------------------------------------------===//

def RetCC_SNES : CallingConv
<[
  // i8 is returned in AL.
  CCIfType<[i8], CCAssignToReg<[AL]>>,

  // i16 are returned in R25:R24, R23:R22, R21:R20 and R19:R18.
  CCIfType<[i16], CCAssignToReg<[A]>>
]>;

//===----------------------------------------------------------------------===//
// SNES Argument Calling Conventions
//===----------------------------------------------------------------------===//

// The calling conventions are implemented in custom C++ code

// Calling convention for variadic functions.
def ArgCC_SNES_Vararg : CallingConv
<[
  // i16 are always passed through the stack with an alignment of 1.
  CCAssignToStack<2, 1>
]>;

//===----------------------------------------------------------------------===//
// Callee-saved register lists.
//===----------------------------------------------------------------------===//

def CSR_Normal : CalleeSavedRegs<(add A)>;
def CSR_Interrupts : CalleeSavedRegs<(add A)>;
