//===-- SNESTargetMachine.cpp - Define TargetMachine for SNES ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the SNES specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#include "SNESTargetMachine.h"

#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"

#include "SNES.h"
#include "SNESTargetObjectFile.h"
#include "MCTargetDesc/SNESMCTargetDesc.h"

namespace llvm {

static const char *SNESDataLayout = "e-p:16:8:8-i1:8:8-i8:8:8-i16:8:8-n8:16";

/// Processes a CPU name.
static StringRef getCPU(StringRef CPU) {
  if (CPU.empty() || CPU == "generic") {
    return "snes2";
  }

  return CPU;
}

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  return RM.hasValue() ? *RM : Reloc::Static;
}

SNESTargetMachine::SNESTargetMachine(const Target &T, const Triple &TT,
                                   StringRef CPU, StringRef FS,
                                   const TargetOptions &Options,
                                   Optional<Reloc::Model> RM, CodeModel::Model CM,
                                   CodeGenOpt::Level OL)
    : LLVMTargetMachine(
          T, SNESDataLayout, TT,
          getCPU(CPU), FS, Options, getEffectiveRelocModel(RM), CM, OL),
      SubTarget(TT, getCPU(CPU), FS, *this) {
  this->TLOF = make_unique<SNESTargetObjectFile>();
  initAsmInfo();
}

namespace {
/// SNES Code Generator Pass Configuration Options.
class SNESPassConfig : public TargetPassConfig {
public:
  SNESPassConfig(SNESTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  SNESTargetMachine &getSNESTargetMachine() const {
    return getTM<SNESTargetMachine>();
  }

  bool addInstSelector() override;
  void addPreSched2() override;
  void addPreEmitPass() override;
  void addPreRegAlloc() override;
};
} // namespace

TargetPassConfig *SNESTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new SNESPassConfig(*this, PM);
}

extern "C" void LLVMInitializeSNESTarget() {
  // Register the target.
  RegisterTargetMachine<SNESTargetMachine> X(getTheSNESTarget());

  auto &PR = *PassRegistry::getPassRegistry();
  initializeSNESExpandPseudoPass(PR);
  initializeSNESInstrumentFunctionsPass(PR);
  initializeSNESRelaxMemPass(PR);
}

const SNESSubtarget *SNESTargetMachine::getSubtargetImpl() const {
  return &SubTarget;
}

const SNESSubtarget *SNESTargetMachine::getSubtargetImpl(const Function &) const {
  return &SubTarget;
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

bool SNESPassConfig::addInstSelector() {
  // Install an instruction selector.
  addPass(createSNESISelDag(getSNESTargetMachine(), getOptLevel()));
  // Create the frame analyzer pass used by the PEI pass.
  addPass(createSNESFrameAnalyzerPass());

  return false;
}

void SNESPassConfig::addPreRegAlloc() {
  // Create the dynalloc SP save/restore pass to handle variable sized allocas.
  addPass(createSNESDynAllocaSRPass());
}

void SNESPassConfig::addPreSched2() {
  addPass(createSNESRelaxMemPass());
  addPass(createSNESExpandPseudoPass());
}

void SNESPassConfig::addPreEmitPass() {
  // Must run branch selection immediately preceding the asm printer.
  addPass(&BranchRelaxationPassID);
}

} // end of namespace llvm
