//===-- SNESCallingConv.td - Calling Conventions SNES ----*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for the SNES architecture.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// SNES v8 32-bit.
//===----------------------------------------------------------------------===//

def CCSNESRet : CallingConv<[
  // DoesNothing
]>;

def CCSNES : CallingConv<[
  CCIfType<[i8], CCAssignToStack<1, 1>>,
  CCIfType<[i16], CCAssignToStack<2, 1>>
]>;


def CSR_NoRegs : CalleeSavedRegs<(add)>;
