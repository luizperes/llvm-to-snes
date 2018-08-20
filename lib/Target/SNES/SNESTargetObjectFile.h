//===-- SNESTargetObjectFile.h - SNES Object Info -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_TARGETOBJECTFILE_H
#define LLVM_SNES_TARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {

class MCContext;
class TargetMachine;

class SNESELFTargetObjectFile : public TargetLoweringObjectFileELF {
public:
  SNESELFTargetObjectFile() : TargetLoweringObjectFileELF() {}

  void Initialize(MCContext &Ctx, const TargetMachine &TM) override;

  const MCExpr *getTTypeGlobalReference(const GlobalValue *GV,
                                        unsigned Encoding,
                                        const TargetMachine &TM,
                                        MachineModuleInfo *MMI,
                                        MCStreamer &Streamer) const override;
};

} // end namespace llvm

#endif // LLVM_SNES_TARGETOBJECTFILE_H
