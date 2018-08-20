//===-- SNESFrameLowering.h - Define frame lowering for SNES --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_FRAMELOWERING_H
#define LLVM_SNES_FRAMELOWERING_H

#include "SNES.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {

class SNESSubtarget;
class SNESFrameLowering : public TargetFrameLowering {
public:
  explicit SNESFrameLowering(const SNESSubtarget &ST);
};

} // End llvm namespace

#endif // LLVM_SNES_FRAMELOWERING_H
