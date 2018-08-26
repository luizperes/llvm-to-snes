//===-- SNESTargetInfo.cpp - SNES Target Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"
namespace llvm {
Target &getTheSNESTarget() {
  static Target TheSNESTarget;
  return TheSNESTarget;
}
}

extern "C" void LLVMInitializeSNESTargetInfo() {
  llvm::RegisterTarget<llvm::Triple::snes> X(llvm::getTheSNESTarget(), "snes",
                                            "Super Nintendo 65c816");
}
