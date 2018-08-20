//===-- SNESInstrInfo.h - SNES Instruction Information --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the NES implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_INSTRINFO_H
#define LLVM_SNES_INSTRINFO_H

#include "SNESRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "SNESGenInstrInfo.inc"

namespace llvm {

class SNESSubtarget;

}

class SNESInstrInfo : public SNESGenInstrInfo {
  const SNESRegisterInfo RI;
  const SNESSubtarget& Subtarget;
public:
  explicit SNESInstrInfo(SNESSubtarget &ST);

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  ///
  const SNESRegisterInfo &getRegisterInfo() const { return RI; }
};

} // end of llvm namespace

#endif // LLVM_SNES_INSTRINFO_H
