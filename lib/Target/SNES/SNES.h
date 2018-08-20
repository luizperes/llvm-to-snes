//===-- SNES.h - Top-level interface for SNES representation --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// SNES back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_H
#define LLVM_SNES_H

// TODO: check how to use mctargetdesc
// include "MCTargetDesc/SNESTargetDesc.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class FunctionPass;
  class SNESTargetMachine;
  
  FunctionPass *createISelDag(SNESTargetMachine &TM);
} // end of llvm namespace

#endif //LLVM_SNES_H 
