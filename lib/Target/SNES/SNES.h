//===-- SNES.h - Top-level interface for SNES representation ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// SNES back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_H
#define LLVM_SNES_H

#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class SNESTargetMachine;
class FunctionPass;

FunctionPass *createSNESISelDag(SNESTargetMachine &TM,
                               CodeGenOpt::Level OptLevel);
FunctionPass *createSNESExpandPseudoPass();
FunctionPass *createSNESFrameAnalyzerPass();
FunctionPass *createSNESInstrumentFunctionsPass();
FunctionPass *createSNESRelaxMemPass();
FunctionPass *createSNESDynAllocaSRPass();
FunctionPass *createSNESBranchSelectionPass();

void initializeSNESExpandPseudoPass(PassRegistry&);
void initializeSNESInstrumentFunctionsPass(PassRegistry&);
void initializeSNESRelaxMemPass(PassRegistry&);

/// Contains the SNES backend.
namespace SNES {

enum AddressSpace { DataMemory, ProgramMemory };

template <typename T> bool isProgramMemoryAddress(T *V) {
  return cast<PointerType>(V->getType())->getAddressSpace() == ProgramMemory;
}

inline bool isProgramMemoryAccess(MemSDNode const *N) {
  auto V = N->getMemOperand()->getValue();

  return (V != nullptr) ? isProgramMemoryAddress(V) : false;
}

} // end of namespace SNES

} // end namespace llvm

#endif // LLVM_SNES_H
