//===-- SNESMCAsmInfo.cpp - SNES asm properties -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the SNESMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "SNESMCAsmInfo.h"

#include "llvm/ADT/Triple.h"

namespace llvm {

SNESMCAsmInfo::SNESMCAsmInfo(const Triple &TT) {
  CodePointerSize = 2;
  CalleeSaveStackSlotSize = 2;
  CommentString = ";";
  PrivateGlobalPrefix = ".L";
  UsesELFSectionDirectiveForBSS = true;
  UseIntegratedAssembler = true;
}

} // end of namespace llvm
