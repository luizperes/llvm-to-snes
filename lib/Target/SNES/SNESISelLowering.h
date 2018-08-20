//===-- SNESISelLowering.h - SNES DAG Lowering Interface ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that SNES uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_ISELLOWERING_H
#define LLVM_SNES_ISELLOWERING_H

#include "SNES.h"
#include "llvm/Target/TargetLowering.h"

namespace llvm {
  class SNESSubtarget;

  class SNESTargetLowering : public TargetLowering {
    const SNESSubtarget *Subtarget;
  public:
    SNESTargetLowering(const TargetMachine &TM, const SparcSNEStarget &STI) {}
  };
} // end namespace llvm

#endif // LLVM_SNES_ISELLOWERING_H
