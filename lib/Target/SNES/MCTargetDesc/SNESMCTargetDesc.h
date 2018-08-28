//===-- SNESMCTargetDesc.h - SNES Target Descriptions -------------*- C++ -*-===//
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

#ifndef LLVM_SNES_MCTARGET_DESC_H
#define LLVM_SNES_MCTARGET_DESC_H

#include "llvm/Support/DataTypes.h"

namespace llvm {

class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectWriter;
class MCRegisterInfo;
class MCTargetOptions;
class StringRef;
class Target;
class Triple;
class raw_pwrite_stream;

Target &getTheSNESTarget();

/// Creates a machine code emitter for SNES.
MCCodeEmitter *createSNESMCCodeEmitter(const MCInstrInfo &MCII,
                                      const MCRegisterInfo &MRI,
                                      MCContext &Ctx);

/// Creates an assembly backend for SNES.
MCAsmBackend *createSNESAsmBackend(const Target &T, const MCRegisterInfo &MRI,
                                  const Triple &TT, StringRef CPU,
                                  const llvm::MCTargetOptions &TO);

/// Creates an ELF object writer for SNES.
MCObjectWriter *createSNESELFObjectWriter(raw_pwrite_stream &OS, uint8_t OSABI);

} // end namespace llvm

#define GET_REGINFO_ENUM
#include "SNESGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "SNESGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "SNESGenSubtargetInfo.inc"

#endif // LLVM_SNES_MCTARGET_DESC_H
