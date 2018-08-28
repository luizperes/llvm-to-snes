//===-- SNESMCTargetDesc.cpp - SNES Target Descriptions ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides SNES specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "SNESMCTargetDesc.h"
#include "SNESELFStreamer.h"
#include "SNESMCAsmInfo.h"
#include "SNESTargetStreamer.h"
#include "InstPrinter/SNESInstPrinter.h"

#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "SNESGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "SNESGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "SNESGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createSNESMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitSNESMCInstrInfo(X);

  return X;
}

static MCRegisterInfo *createSNESMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitSNESMCRegisterInfo(X, 0);

  return X;
}

static MCSubtargetInfo *createSNESMCSubtargetInfo(const Triple &TT,
                                                 StringRef CPU, StringRef FS) {
  return createSNESMCSubtargetInfoImpl(TT, CPU, FS);
}

static MCInstPrinter *createSNESMCInstPrinter(const Triple &T,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0) {
    return new SNESInstPrinter(MAI, MII, MRI);
  }

  return nullptr;
}

static MCStreamer *createMCStreamer(const Triple &T, MCContext &Context,
                                    MCAsmBackend &MAB, raw_pwrite_stream &OS,
                                    MCCodeEmitter *Emitter, bool RelaxAll) {
  return createELFStreamer(Context, MAB, OS, Emitter, RelaxAll);
}

static MCTargetStreamer *
createSNESObjectTargetStreamer(MCStreamer &S, const MCSubtargetInfo &STI) {
  return new SNESELFStreamer(S, STI);
}

static MCTargetStreamer *createMCAsmTargetStreamer(MCStreamer &S,
                                                   formatted_raw_ostream &OS,
                                                   MCInstPrinter *InstPrint,
                                                   bool isVerboseAsm) {
  return new SNESTargetAsmStreamer(S);
}

extern "C" void LLVMInitializeSNESTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfo<SNESMCAsmInfo> X(getTheSNESTarget());

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(getTheSNESTarget(), createSNESMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(getTheSNESTarget(), createSNESMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(getTheSNESTarget(),
                                          createSNESMCSubtargetInfo);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(getTheSNESTarget(),
                                        createSNESMCInstPrinter);

  // Register the MC Code Emitter
  TargetRegistry::RegisterMCCodeEmitter(getTheSNESTarget(), createSNESMCCodeEmitter);

  // Register the ELF streamer
  TargetRegistry::RegisterELFStreamer(getTheSNESTarget(), createMCStreamer);

  // Register the obj target streamer.
  TargetRegistry::RegisterObjectTargetStreamer(getTheSNESTarget(),
                                               createSNESObjectTargetStreamer);

  // Register the asm target streamer.
  TargetRegistry::RegisterAsmTargetStreamer(getTheSNESTarget(),
                                            createMCAsmTargetStreamer);

  // Register the asm backend (as little endian).
  TargetRegistry::RegisterMCAsmBackend(getTheSNESTarget(), createSNESAsmBackend);
}

