//===-- SNESMCAsmInfo.h - SNES asm properties ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the SNESMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_ASM_INFO_H
#define LLVM_SNES_ASM_INFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {

class Triple;

/// Specifies the format of SNES assembly files.
class SNESMCAsmInfo : public MCAsmInfo {
public:
  explicit SNESMCAsmInfo(const Triple &TT);
};

} // end namespace llvm

#endif // LLVM_SNES_ASM_INFO_H
