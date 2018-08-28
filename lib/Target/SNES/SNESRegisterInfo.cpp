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
  // TODO: check the code below
  /*
  CallingConv::ID CC = MF->getFunction()->getCallingConv();
  return ((CC == CallingConv::SNES_INTR || CC == CallingConv::SNES_SIGNAL)
              ? CSR_Interrupts_SaveList
              : CSR_Normal_SaveList);
  */
  return CSR_Normal_SaveList;
}

const uint32_t *
SNESRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                      CallingConv::ID CC) const {
  // TODO: check the code below
  /*
  return ((CC == CallingConv::SNES_INTR || CC == CallingConv::SNES_SIGNAL)
              ? CSR_Interrupts_RegMask
              : CSR_Normal_RegMask);
  */
  return CSR_Normal_RegMask;
}

BitVector SNESRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // Reserve the intermediate result registers r1 and r2
  // The result of instructions like 'mul' is always stored here.
  Reserved.set(SNES::R0);
  Reserved.set(SNES::R1);
  Reserved.set(SNES::R1R0);

  //  Reserve the stack pointer.
  Reserved.set(SNES::SPL);
  Reserved.set(SNES::SPH);
  Reserved.set(SNES::SP);

  // We tenatively reserve the frame pointer register r29:r28 because the
  // function may require one, but we cannot tell until register allocation
  // is complete, which can be too late.
  //
  // Instead we just unconditionally reserve the Y register.
  //
  // TODO: Write a pass to enumerate functions which reserved the Y register
  //       but didn't end up needing a frame pointer. In these, we can
  //       convert one or two of the spills inside to use the Y register.
  Reserved.set(SNES::R28);
  Reserved.set(SNES::R29);
  Reserved.set(SNES::R29R28);

  return Reserved;
}

const TargetRegisterClass *
SNESRegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
                                           const MachineFunction &MF) const {
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  if (TRI->isTypeLegalForClass(*RC, MVT::i16)) {
    return &SNES::DREGSRegClass;
  }

  if (TRI->isTypeLegalForClass(*RC, MVT::i8)) {
    return &SNES::GPR8RegClass;
  }

  llvm_unreachable("Invalid register size");
}

/// Fold a frame offset shared between two add instructions into a single one.
static void foldFrameOffset(MachineBasicBlock::iterator &II, int &Offset, unsigned DstReg) {
  MachineInstr &MI = *II;
  int Opcode = MI.getOpcode();

  // Don't bother trying if the next instruction is not an add or a sub.
  if ((Opcode != SNES::SUBIWRdK) && (Opcode != SNES::ADIWRdK)) {
    return;
  }

  // Check that DstReg matches with next instruction, otherwise the instruction
  // is not related to stack address manipulation.
  if (DstReg != MI.getOperand(0).getReg()) {
    return;
  }

  // Add the offset in the next instruction to our offset.
  switch (Opcode) {
  case SNES::SUBIWRdK:
    Offset += -MI.getOperand(2).getImm();
    break;
  case SNES::ADIWRdK:
    Offset += MI.getOperand(2).getImm();
    break;
  }

  // Finally remove the instruction.
  II++;
  MI.eraseFromParent();
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
  const TargetInstrInfo &TII = *TM.getSubtargetImpl()->getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetFrameLowering *TFI = TM.getSubtargetImpl()->getFrameLowering();
  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  int Offset = MFI.getObjectOffset(FrameIndex);

  // Add one to the offset because SP points to an empty slot.
  Offset += MFI.getStackSize() - TFI->getOffsetOfLocalArea() + 1;
  // Fold incoming offset.
  Offset += MI.getOperand(FIOperandNum + 1).getImm();

  // This is actually "load effective address" of the stack slot
  // instruction. We have only two-address instructions, thus we need to
  // expand it into move + add.
  if (MI.getOpcode() == SNES::FRMIDX) {
    MI.setDesc(TII.get(SNES::MOVWRdRr));
    MI.getOperand(FIOperandNum).ChangeToRegister(SNES::R29R28, false);

    assert(Offset > 0 && "Invalid offset");

    // We need to materialize the offset via an add instruction.
    unsigned Opcode;
    unsigned DstReg = MI.getOperand(0).getReg();
    assert(DstReg != SNES::R29R28 && "Dest reg cannot be the frame pointer");

    II++; // Skip over the FRMIDX (and now MOVW) instruction.

    // Generally, to load a frame address two add instructions are emitted that
    // could get folded into a single one:
    //  movw    r31:r30, r29:r28
    //  adiw    r31:r30, 29
    //  adiw    r31:r30, 16
    // to:
    //  movw    r31:r30, r29:r28
    //  adiw    r31:r30, 45
    if (II != MBB.end())
      foldFrameOffset(II, Offset, DstReg);

    // Select the best opcode based on DstReg and the offset size.
    switch (DstReg) {
    case SNES::R25R24:
    case SNES::R27R26:
    case SNES::R31R30: {
      if (isUInt<6>(Offset)) {
        Opcode = SNES::ADIWRdK;
        break;
      }
      LLVM_FALLTHROUGH;
    }
    default: {
      // This opcode will get expanded into a pair of subi/sbci.
      Opcode = SNES::SUBIWRdK;
      Offset = -Offset;
      break;
    }
    }

    MachineInstr *New = BuildMI(MBB, II, dl, TII.get(Opcode), DstReg)
                            .addReg(DstReg, RegState::Kill)
                            .addImm(Offset);
    New->getOperand(3).setIsDead();

    return;
  }

  // If the offset is too big we have to adjust and restore the frame pointer
  // to materialize a valid load/store with displacement.
  //:TODO: consider using only one adiw/sbiw chain for more than one frame index
  if (Offset > 63) {
    unsigned AddOpc = SNES::ADIWRdK, SubOpc = SNES::SBIWRdK;
    int AddOffset = Offset - 63 + 1;

    // For huge offsets where adiw/sbiw cannot be used use a pair of subi/sbci.
    if ((Offset - 63 + 1) > 63) {
      AddOpc = SNES::SUBIWRdK;
      SubOpc = SNES::SUBIWRdK;
      AddOffset = -AddOffset;
    }

    // It is possible that the spiller places this frame instruction in between
    // a compare and branch, invalidating the contents of SREG set by the
    // compare instruction because of the add/sub pairs. Conservatively save and
    // restore SREG before and after each add/sub pair.
    BuildMI(MBB, II, dl, TII.get(SNES::INRdA), SNES::R0).addImm(0x3f);

    MachineInstr *New = BuildMI(MBB, II, dl, TII.get(AddOpc), SNES::R29R28)
                            .addReg(SNES::R29R28, RegState::Kill)
                            .addImm(AddOffset);
    New->getOperand(3).setIsDead();

    // Restore SREG.
    BuildMI(MBB, std::next(II), dl, TII.get(SNES::OUTARr))
        .addImm(0x3f)
        .addReg(SNES::R0, RegState::Kill);

    // No need to set SREG as dead here otherwise if the next instruction is a
    // cond branch it will be using a dead register.
    New = BuildMI(MBB, std::next(II), dl, TII.get(SubOpc), SNES::R29R28)
              .addReg(SNES::R29R28, RegState::Kill)
              .addImm(Offset - 63 + 1);

    Offset = 62;
  }

  MI.getOperand(FIOperandNum).ChangeToRegister(SNES::R29R28, false);
  assert(isUInt<6>(Offset) && "Offset is out of range");
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
}

unsigned SNESRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  if (TFI->hasFP(MF)) {
    // The Y pointer register
    return SNES::R28;
  }

  return SNES::SP;
}

const TargetRegisterClass *
SNESRegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                    unsigned Kind) const {
  // FIXME: Currently we're using snes-gcc as reference, so we restrict
  // ptrs to Y and Z regs. Though snes-gcc has buggy implementation
  // of memory constraint, so we can fix it and bit snes-gcc here ;-)
  return &SNES::PTRDISPREGSRegClass;
}

void SNESRegisterInfo::splitReg(unsigned Reg,
                               unsigned &LoReg,
                               unsigned &HiReg) const {
    assert(SNES::DREGSRegClass.contains(Reg) && "can only split 16-bit registers");

    LoReg = getSubReg(Reg, SNES::sub_lo);
    HiReg = getSubReg(Reg, SNES::sub_hi);
}

} // end of namespace llvm
