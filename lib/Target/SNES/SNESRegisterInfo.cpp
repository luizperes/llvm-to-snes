//===-- SNESRegisterInfo.cpp - SNES Register Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the SNES implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "SNESRegisterInfo.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/Target/TargetFrameLowering.h"

#include "SNES.h"
#include "SNESInstrInfo.h"
#include "SNESTargetMachine.h"
#include "MCTargetDesc/SNESMCTargetDesc.h"

#define GET_REGINFO_TARGET_DESC
#include "SNESGenRegisterInfo.inc"

namespace llvm {

SNESRegisterInfo::SNESRegisterInfo() : SNESGenRegisterInfo(0) {}

const uint16_t *
SNESRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_Normal_SaveList;
}

const uint32_t *
SNESRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                      CallingConv::ID CC) const {
  return CSR_Normal_RegMask;
}

BitVector SNESRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // Reserve the data bank register
  Reserved.set(SNES::DB);

  // Reserve the direct page register
  Reserved.set(SNES::DP);

  // Reserve the status (eflags) register
  Reserved.set(SNES::P);

  // Reserve the program counter register
  Reserved.set(SNES::PC);

  // Reserve the program bank register
  Reserved.set(SNES::PB);

  // Reserve the stack pointer register
  Reserved.set(SNES::SP);

  return Reserved;
}

const TargetRegisterClass *
SNESRegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
                                           const MachineFunction &MF) const {
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  if (TRI->isTypeLegalForClass(*RC, MVT::i16)) {
    return &SNES::MainRegsRegClass;
  }

  llvm_unreachable("Invalid register size");
}

void SNESRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected SPAdj value");

  MachineInstr &MI = *II;
  DebugLoc dl = MI.getDebugLoc();
  MachineBasicBlock &MBB = *MI.getParent();
  const MachineFunction &MF = *MBB.getParent();
  const SNESTargetMachine &TM = (const SNESTargetMachine &)MF.getTarget();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetFrameLowering *TFI = TM.getSubtargetImpl()->getFrameLowering();
  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  int Offset = MFI.getObjectOffset(FrameIndex);

  // Add one to the offset because SP points to an empty slot.
  Offset += MFI.getStackSize() - TFI->getOffsetOfLocalArea() + 1;
  // Fold incoming offset.
  Offset += MI.getOperand(FIOperandNum + 1).getImm();

  MI.getOperand(FIOperandNum).ChangeToRegister(SNES::A, false);
  assert(isUInt<6>(Offset) && "Offset is out of range");
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
}

unsigned SNESRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  if (TFI->hasFP(MF)) {
    // TODO: for now we will use the Y register as the frame pointer
    // keeping the value of SP + offset. Check if it is good enough
    return SNES::Y;
  }

  return SNES::SP;
}

const TargetRegisterClass *
SNESRegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                    unsigned Kind) const {
  // TODO: check if it is the right class of registers.
  return &SNES::MainRegsRegClass;
}

void SNESRegisterInfo::splitReg(unsigned Reg,
                               unsigned &LoReg,
                               unsigned &HiReg) const {
    assert(SNES::MainRegsRegClass.contains(Reg) && "can only split 16-bit registers");

    LoReg = getSubReg(Reg, SNES::sub_lo);
    HiReg = getSubReg(Reg, SNES::sub_hi);
}

} // end of namespace llvm
