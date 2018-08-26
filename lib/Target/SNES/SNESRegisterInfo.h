//===-- SNESRegisterInfo.h - SNES Register Information Impl ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the SNES implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_REGISTERINFO_H
#define LLVM_SNES_REGISTERINFO_H

#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "SNESGenRegisterInfo.inc"

namespace llvm {

class SNESSubtarget;

struct SNESRegisterInfo : public SNESGenRegisterInfo {
  SNESRegisterInfo();
};

} // end namespace llvm

#endif // LLVM_SNES_REGISTERINFO_H
