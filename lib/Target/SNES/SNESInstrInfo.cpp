//===-- SNESInstrInfo.cpp - SNES Instruction Information --------------------===//
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

#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#include "SNES.h"
#include "SNESMachineFunctionInfo.h"
#include "SNESRegisterInfo.h"
#include "SNESTargetMachine.h"
#include "MCTargetDesc/SNESMCTargetDesc.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "SNESGenInstrInfo.inc"

namespace llvm {

SNESInstrInfo::SNESInstrInfo()
    : SNESGenInstrInfo(SNES::ADJCALLSTACKDOWN, SNES::ADJCALLSTACKUP), RI() {}

void SNESInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MI,
                               const DebugLoc &DL, unsigned DestReg,
                               unsigned SrcReg, bool KillSrc) const {
  const SNESSubtarget &STI = MBB.getParent()->getSubtarget<SNESSubtarget>();
  const SNESRegisterInfo &TRI = *STI.getRegisterInfo();
  unsigned Opc;

  // Not all SNES devices support the 16-bit `MOVW` instruction.
  if (SNES::MainRegsRegClass.contains(DestReg, SrcReg)) {
    // TODO: check if we need hasMOVW
    // if (STI.hasMOVW()) {
    //   BuildMI(MBB, MI, DL, get(SNES::MOVWRdRr), DestReg)
    //       .addReg(SrcReg, getKillRegState(KillSrc));
    // } else {
      unsigned DestLo, DestHi, SrcLo, SrcHi;

      TRI.splitReg(DestReg, DestLo, DestHi);
      TRI.splitReg(SrcReg,  SrcLo,  SrcHi);

      // Copy each individual register with the `MOV` instruction.
      BuildMI(MBB, MI, DL, get(SNES::MOVRdRr), DestLo)
        .addReg(SrcLo, getKillRegState(KillSrc));
      BuildMI(MBB, MI, DL, get(SNES::MOVRdRr), DestHi)
        .addReg(SrcHi, getKillRegState(KillSrc));
    // }
  } else {
    if (SNES::MainRegsRegClass.contains(DestReg, SrcReg)) {
      Opc = SNES::MOVRdRr;
    } else if (SrcReg == SNES::SP && SNES::MainRegsRegClass.contains(DestReg)) {
      Opc = SNES::SPREAD;
    } else if (DestReg == SNES::SP && SNES::MainRegsRegClass.contains(SrcReg)) {
      Opc = SNES::SPWRITE;
    } else {
      llvm_unreachable("Impossible reg-to-reg copy");
    }

    BuildMI(MBB, MI, DL, get(Opc), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc));
  }
}

unsigned SNESInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                           int &FrameIndex) const {
  switch (MI.getOpcode()) {
  case SNES::LDDRdPtrQ:
  case SNES::LDDWRdYQ: { //:FIXME: remove this once PR13375 gets fixed
    if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
        MI.getOperand(2).getImm() == 0) {
      FrameIndex = MI.getOperand(1).getIndex();
      return MI.getOperand(0).getReg();
    }
    break;
  }
  default:
    break;
  }

  return 0;
}

unsigned SNESInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                          int &FrameIndex) const {
  switch (MI.getOpcode()) {
  case SNES::STDPtrQRr:
  case SNES::STDWPtrQRr: {
    if (MI.getOperand(0).isFI() && MI.getOperand(1).isImm() &&
        MI.getOperand(1).getImm() == 0) {
      FrameIndex = MI.getOperand(0).getIndex();
      return MI.getOperand(2).getReg();
    }
    break;
  }
  default:
    break;
  }

  return 0;
}

void SNESInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MI,
                                       unsigned SrcReg, bool isKill,
                                       int FrameIndex,
                                       const TargetRegisterClass *RC,
                                       const TargetRegisterInfo *TRI) const {
  MachineFunction &MF = *MBB.getParent();
  SNESMachineFunctionInfo *AFI = MF.getInfo<SNESMachineFunctionInfo>();

  AFI->setHasSpills(true);

  DebugLoc DL;
  if (MI != MBB.end()) {
    DL = MI->getDebugLoc();
  }

  const MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlignment(FrameIndex));

  unsigned Opcode = 0;
  if (TRI->isTypeLegalForClass(*RC, MVT::i8)) {
    Opcode = SNES::STDPtrQRr;
  } else if (TRI->isTypeLegalForClass(*RC, MVT::i16)) {
    Opcode = SNES::STDWPtrQRr;
  } else {
    llvm_unreachable("Cannot store this register into a stack slot!");
  }

  fddfd

  BuildMI(MBB, MI, DL, get(Opcode))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addReg(SrcReg, getKillRegState(isKill))
      .addMemOperand(MMO);
}

void SNESInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        unsigned DestReg, int FrameIndex,
                                        const TargetRegisterClass *RC,
                                        const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (MI != MBB.end()) {
    DL = MI->getDebugLoc();
  }

  MachineFunction &MF = *MBB.getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlignment(FrameIndex));

  unsigned Opcode = 0;
  if (TRI->isTypeLegalForClass(*RC, MVT::i8)) {
    Opcode = SNES::LDDRdPtrQ;
  } else if (TRI->isTypeLegalForClass(*RC, MVT::i16)) {
    // Opcode = SNES::LDDWRdPtrQ;
    //:FIXME: remove this once PR13375 gets fixed
    Opcode = SNES::LDDWRdYQ;
  } else {
    llvm_unreachable("Cannot load this register from a stack slot!");
  }

  BuildMI(MBB, MI, DL, get(Opcode), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO);
}

const MCInstrDesc &SNESInstrInfo::getBrCond(SNESCC::CondCodes CC) const {
  switch (CC) {
  default:
    llvm_unreachable("Unknown condition code!");
  case SNESCC::COND_EQ:
    return get(SNES::BREQk);
  case SNESCC::COND_NE:
    return get(SNES::BRNEk);
  case SNESCC::COND_GE:
    return get(SNES::BRGEk);
  case SNESCC::COND_LT:
    return get(SNES::BRLTk);
  case SNESCC::COND_SH:
    return get(SNES::BRSHk);
  case SNESCC::COND_LO:
    return get(SNES::BRLOk);
  case SNESCC::COND_MI:
    return get(SNES::BRMIk);
  case SNESCC::COND_PL:
    return get(SNES::BRPLk);
  }
}

SNESCC::CondCodes SNESInstrInfo::getCondFromBranchOpc(unsigned Opc) const {
  switch (Opc) {
  default:
    return SNESCC::COND_INVALID;
  case SNES::BREQk:
    return SNESCC::COND_EQ;
  case SNES::BRNEk:
    return SNESCC::COND_NE;
  case SNES::BRSHk:
    return SNESCC::COND_SH;
  case SNES::BRLOk:
    return SNESCC::COND_LO;
  case SNES::BRMIk:
    return SNESCC::COND_MI;
  case SNES::BRPLk:
    return SNESCC::COND_PL;
  case SNES::BRGEk:
    return SNESCC::COND_GE;
  case SNES::BRLTk:
    return SNESCC::COND_LT;
  }
}

SNESCC::CondCodes SNESInstrInfo::getOppositeCondition(SNESCC::CondCodes CC) const {
  switch (CC) {
  default:
    llvm_unreachable("Invalid condition!");
  case SNESCC::COND_EQ:
    return SNESCC::COND_NE;
  case SNESCC::COND_NE:
    return SNESCC::COND_EQ;
  case SNESCC::COND_SH:
    return SNESCC::COND_LO;
  case SNESCC::COND_LO:
    return SNESCC::COND_SH;
  case SNESCC::COND_GE:
    return SNESCC::COND_LT;
  case SNESCC::COND_LT:
    return SNESCC::COND_GE;
  case SNESCC::COND_MI:
    return SNESCC::COND_PL;
  case SNESCC::COND_PL:
    return SNESCC::COND_MI;
  }
}

bool SNESInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                 MachineBasicBlock *&TBB,
                                 MachineBasicBlock *&FBB,
                                 SmallVectorImpl<MachineOperand> &Cond,
                                 bool AllowModify) const {
  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  MachineBasicBlock::iterator I = MBB.end();
  MachineBasicBlock::iterator UnCondBrIter = MBB.end();

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue()) {
      continue;
    }

    // Working from the bottom, when we see a non-terminator
    // instruction, we're done.
    if (!isUnpredicatedTerminator(*I)) {
      break;
    }

    // A terminator that isn't a branch can't easily be handled
    // by this analysis.
    if (!I->getDesc().isBranch()) {
      return true;
    }

    // Handle unconditional branches.
    //:TODO: add here jmp
    if (I->getOpcode() == SNES::RJMPk) {
      UnCondBrIter = I;

      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      // If the block has any instructions after a JMP, delete them.
      while (std::next(I) != MBB.end()) {
        std::next(I)->eraseFromParent();
      }

      Cond.clear();
      FBB = 0;

      // Delete the JMP if it's equivalent to a fall-through.
      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = 0;
        I->eraseFromParent();
        I = MBB.end();
        UnCondBrIter = MBB.end();
        continue;
      }

      // TBB is used to indicate the unconditinal destination.
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    // Handle conditional branches.
    SNESCC::CondCodes BranchCode = getCondFromBranchOpc(I->getOpcode());
    if (BranchCode == SNESCC::COND_INVALID) {
      return true; // Can't handle indirect branch.
    }

    // Working from the bottom, handle the first conditional branch.
    if (Cond.empty()) {
      MachineBasicBlock *TargetBB = I->getOperand(0).getMBB();
      if (AllowModify && UnCondBrIter != MBB.end() &&
          MBB.isLayoutSuccessor(TargetBB)) {
        // If we can modify the code and it ends in something like:
        //
        //     jCC L1
        //     jmp L2
        //   L1:
        //     ...
        //   L2:
        //
        // Then we can change this to:
        //
        //     jnCC L2
        //   L1:
        //     ...
        //   L2:
        //
        // Which is a bit more efficient.
        // We conditionally jump to the fall-through block.
        BranchCode = getOppositeCondition(BranchCode);
        unsigned JNCC = getBrCond(BranchCode).getOpcode();
        MachineBasicBlock::iterator OldInst = I;

        BuildMI(MBB, UnCondBrIter, MBB.findDebugLoc(I), get(JNCC))
            .addMBB(UnCondBrIter->getOperand(0).getMBB());
        BuildMI(MBB, UnCondBrIter, MBB.findDebugLoc(I), get(SNES::RJMPk))
            .addMBB(TargetBB);

        OldInst->eraseFromParent();
        UnCondBrIter->eraseFromParent();

        // Restart the analysis.
        UnCondBrIter = MBB.end();
        I = MBB.end();
        continue;
      }

      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(BranchCode));
      continue;
    }

    // Handle subsequent conditional branches. Only handle the case where all
    // conditional branches branch to the same destination.
    assert(Cond.size() == 1);
    assert(TBB);

    // Only handle the case where all conditional branches branch to
    // the same destination.
    if (TBB != I->getOperand(0).getMBB()) {
      return true;
    }

    SNESCC::CondCodes OldBranchCode = (SNESCC::CondCodes)Cond[0].getImm();
    // If the conditions are the same, we can leave them alone.
    if (OldBranchCode == BranchCode) {
      continue;
    }

    return true;
  }

  return false;
}

unsigned SNESInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *TBB,
                                    MachineBasicBlock *FBB,
                                    ArrayRef<MachineOperand> Cond,
                                    const DebugLoc &DL,
                                    int *BytesAdded) const {
  if (BytesAdded) *BytesAdded = 0;

  // Shouldn't be a fall through.
  assert(TBB && "insertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 1 || Cond.size() == 0) &&
         "SNES branch conditions have one component!");

  if (Cond.empty()) {
    assert(!FBB && "Unconditional branch with multiple successors!");
    auto &MI = *BuildMI(&MBB, DL, get(SNES::RJMPk)).addMBB(TBB);
    if (BytesAdded)
      *BytesAdded += getInstSizeInBytes(MI);
    return 1;
  }

  // Conditional branch.
  unsigned Count = 0;
  SNESCC::CondCodes CC = (SNESCC::CondCodes)Cond[0].getImm();
  auto &CondMI = *BuildMI(&MBB, DL, getBrCond(CC)).addMBB(TBB);

  if (BytesAdded) *BytesAdded += getInstSizeInBytes(CondMI);
  ++Count;

  if (FBB) {
    // Two-way Conditional branch. Insert the second branch.
    auto &MI = *BuildMI(&MBB, DL, get(SNES::RJMPk)).addMBB(FBB);
    if (BytesAdded) *BytesAdded += getInstSizeInBytes(MI);
    ++Count;
  }

  return Count;
}

unsigned SNESInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                    int *BytesRemoved) const {
  if (BytesRemoved) *BytesRemoved = 0;

  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue()) {
      continue;
    }
    //:TODO: add here the missing jmp instructions once they are implemented
    // like jmp, {e}ijmp, and other cond branches, ...
    if (I->getOpcode() != SNES::RJMPk &&
        getCondFromBranchOpc(I->getOpcode()) == SNESCC::COND_INVALID) {
      break;
    }

    // Remove the branch.
    if (BytesRemoved) *BytesRemoved += getInstSizeInBytes(*I);
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  return Count;
}

bool SNESInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "Invalid SNES branch condition!");

  SNESCC::CondCodes CC = static_cast<SNESCC::CondCodes>(Cond[0].getImm());
  Cond[0].setImm(getOppositeCondition(CC));

  return false;
}

unsigned SNESInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  unsigned Opcode = MI.getOpcode();

  switch (Opcode) {
  // A regular instruction
  default: {
    const MCInstrDesc &Desc = get(Opcode);
    return Desc.getSize();
  }
  case TargetOpcode::EH_LABEL:
  case TargetOpcode::IMPLICIT_DEF:
  case TargetOpcode::KILL:
  case TargetOpcode::DBG_VALUE:
    return 0;
  case TargetOpcode::INLINEASM: {
    const MachineFunction &MF = *MI.getParent()->getParent();
    const SNESTargetMachine &TM = static_cast<const SNESTargetMachine&>(MF.getTarget());
    const SNESSubtarget &STI = MF.getSubtarget<SNESSubtarget>();
    const TargetInstrInfo &TII = *STI.getInstrInfo();

    return TII.getInlineAsmLength(MI.getOperand(0).getSymbolName(),
                                  *TM.getMCAsmInfo());
  }
  }
}

MachineBasicBlock *
SNESInstrInfo::getBranchDestBlock(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("unexpected opcode!");
  case SNES::JMPk:
  case SNES::CALLk:
  case SNES::RCALLk:
  case SNES::RJMPk:
  case SNES::BREQk:
  case SNES::BRNEk:
  case SNES::BRSHk:
  case SNES::BRLOk:
  case SNES::BRMIk:
  case SNES::BRPLk:
  case SNES::BRGEk:
  case SNES::BRLTk:
    return MI.getOperand(0).getMBB();
  case SNES::BRBSsk:
  case SNES::BRBCsk:
    return MI.getOperand(1).getMBB();
  case SNES::SBRCRrB:
  case SNES::SBRSRrB:
  case SNES::SBICAb:
  case SNES::SBISAb:
    llvm_unreachable("unimplemented branch instructions");
  }
}

bool SNESInstrInfo::isBranchOffsetInRange(unsigned BranchOp,
                                         int64_t BrOffset) const {

  switch (BranchOp) {
  default:
    llvm_unreachable("unexpected opcode!");
  case SNES::JMPk:
  case SNES::CALLk:
    assert(BrOffset >= 0 && "offset must be absolute address");
    return isUIntN(16, BrOffset);
  case SNES::RCALLk:
  case SNES::RJMPk:
    return isIntN(13, BrOffset);
  case SNES::BRBSsk:
  case SNES::BRBCsk:
  case SNES::BREQk:
  case SNES::BRNEk:
  case SNES::BRSHk:
  case SNES::BRLOk:
  case SNES::BRMIk:
  case SNES::BRPLk:
  case SNES::BRGEk:
  case SNES::BRLTk:
    return isIntN(7, BrOffset);
  }
}

} // end of namespace llvm

