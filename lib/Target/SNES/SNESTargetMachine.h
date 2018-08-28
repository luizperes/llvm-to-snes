//===-- SNESTargetMachine.h - Define TargetMachine for SNES -------*- C++ -*-===//
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

#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"

#include "SNESFrameLowering.h"
#include "SNESISelLowering.h"
#include "SNESInstrInfo.h"
#include "SNESSelectionDAGInfo.h"
#include "SNESSubtarget.h"

namespace llvm {

/// A generic SNES implementation.
class SNESTargetMachine : public LLVMTargetMachine {
public:
  SNESTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                   StringRef FS, const TargetOptions &Options, Optional<Reloc::Model> RM,
                   CodeModel::Model CM, CodeGenOpt::Level OL);

  const SNESSubtarget *getSubtargetImpl() const;
  const SNESSubtarget *getSubtargetImpl(const Function &) const override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return this->TLOF.get();
  }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  bool isMachineVerifierClean() const override {
    return false;
  }

private:
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  SNESSubtarget SubTarget;
};

} // end namespace llvm

#endif // LLVM_SNES_TARGET_MACHINE_H
