//===-- SNESInstrInfo.cpp - SNES Instruction Information ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the SNES implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "SNESInstrInfo.h"
#include "SNES.h"
#include "SNESMachineFunctionInfo.h"
#include "SNESSubtarget.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "SNESGenInstrInfo.inc"

SNESInstrInfo::SNESInstrInfo(SNESSubtarget &ST)
    : SNESGenInstrInfo(), RI(),
      Subtarget(ST) {}
