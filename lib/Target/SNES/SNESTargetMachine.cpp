//===-- SNESTargetMachine.cpp - Define TargetMachine for SNES (65c816) -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "SNESTargetMachine.h"
#include "SNES.h"
#include "SNESTargetObjectFile.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

static const char *SNESDataLayout = "e-p:16:8:8-i1:8:8-i8:8:8-i16:8:8-n8:16";

extern "C" void LLVMInitializeSNESTarget() {
  // Register the target.
  RegisterTargetMachine<SNESTargetMachine> X(getTheSNESTarget());
}

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (!RM.hasValue())
    return Reloc::Static;
  return *RM;
}

/// Processes a CPU name.
static StringRef getCPU(StringRef CPU) {
  if (CPU.empty() || CPU == "generic") {
    return "snes";
  }

  return CPU;
}

SNESTargetMachine::SNESTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     Optional<Reloc::Model> RM,
                                     CodeModel::Model CM,
                                     CodeGenOpt::Level OL)
    : LLVMTargetMachine(T, SNESDataLayout, TT, getCPU(CPU), FS, Options,
                        getEffectiveRelocModel(RM), CM, OL),
      Subtarget(TT, getCpu(CPU), FS, *this) {
  this->TLOF = make_unique<AVRTargetObjectFile>();
  initAsmInfo();
}

SNESTargetMachine::~SNESTargetMachine() {}

// TODO: check what is needed from code below
/*
namespace {
/// Spar Code Generator Pass Configuration Options.
class SparcPassConfig : public TargetPassConfig {
public:
  SparcPassConfig(SparcTargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  SparcTargetMachine &getSparcTargetMachine() const {
    return getTM<SparcTargetMachine>();
  }

  void addIRPasses() override;
  bool addInstSelector() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *SparcTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new SparcPassConfig(*this, PM);
}

void SparcPassConfig::addIRPasses() {
  addPass(createAtomicExpandPass());

  TargetPassConfig::addIRPasses();
}

bool SparcPassConfig::addInstSelector() {
  addPass(createSparcISelDag(getSparcTargetMachine()));
  return false;
}

void SparcPassConfig::addPreEmitPass(){
  addPass(createSparcDelaySlotFillerPass());

  if (this->getSparcTargetMachine().getSubtargetImpl()->insertNOPLoad())
  {
    addPass(new InsertNOPLoad());
  }
  if (this->getSparcTargetMachine().getSubtargetImpl()->fixFSMULD())
  {
    addPass(new FixFSMULD());
  }
  if (this->getSparcTargetMachine().getSubtargetImpl()->replaceFMULS())
  {
    addPass(new ReplaceFMULS());
  }
  if (this->getSparcTargetMachine().getSubtargetImpl()->detectRoundChange()) {
    addPass(new DetectRoundChange());
  }
  if (this->getSparcTargetMachine().getSubtargetImpl()->fixAllFDIVSQRT())
  {
    addPass(new FixAllFDIVSQRT());
  }
}
*/
