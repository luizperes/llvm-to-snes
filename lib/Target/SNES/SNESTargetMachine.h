//===-- SNESTargetMachine.h - Define TargetMachine for SNES (65c816) ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the SNES specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_TARGET_MACHINE_H
#define LLVM_SNES_TARGET_MACHINE_H

#include "SNESInstrInfo.h"
#include "SNESSubtarget.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class SNESTargetMachine : public LLVMTargetMachine {
private:
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  SNESSubtarget Subtarget;
public:
  SNESTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                    StringRef FS, const TargetOptions &Options,
                    Optional<Reloc::Model> RM, CodeModel::Model CM,
                    CodeGenOpt::Level OL);
  ~SNESTargetMachine() override;

  const SNESSubtarget *getSubtargetImpl() const { return &Subtarget; }
  const SNESSubtarget *getSubtargetImpl(const Function &) const override {
    return &Subtarget;
  }

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }

  bool isMachineVerifierClean() const override {
    return false;
  }
};

} // end of llvm namespace

#endif // LLVM_SNES_TARGET_MACHINE_H
