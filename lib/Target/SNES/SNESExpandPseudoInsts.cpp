//===-- SNESExpandPseudoInsts.cpp - Expand pseudo instructions -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions. This pass should be run after register allocation but before
// the post-regalloc scheduling pass.
//
//===----------------------------------------------------------------------===//

#include "SNES.h"
#include "SNESInstrInfo.h"
#include "SNESTargetMachine.h"
#include "MCTargetDesc/SNESMCTargetDesc.h"

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Target/TargetRegisterInfo.h"

using namespace llvm;

#define SNES_EXPAND_PSEUDO_NAME "SNES pseudo instruction expansion pass"

namespace {

/// Expands "placeholder" instructions marked as pseudo into
/// actual SNES instructions.
class SNESExpandPseudo : public MachineFunctionPass {
public:
  static char ID;

  SNESExpandPseudo() : MachineFunctionPass(ID) {
    initializeSNESExpandPseudoPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return SNES_EXPAND_PSEUDO_NAME; }

private:
  typedef MachineBasicBlock Block;
  typedef Block::iterator BlockIt;

  const SNESRegisterInfo *TRI;
  const TargetInstrInfo *TII;

  /// The register to be used for temporary storage.
  const unsigned SCRATCH_REGISTER = SNES::R0;
  /// The IO address of the status register.
  const unsigned SREG_ADDR = 0x3f;

  bool expandMBB(Block &MBB);
  bool expandMI(Block &MBB, BlockIt MBBI);
  template <unsigned OP> bool expand(Block &MBB, BlockIt MBBI);

  MachineInstrBuilder buildMI(Block &MBB, BlockIt MBBI, unsigned Opcode) {
    return BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(Opcode));
  }

  MachineInstrBuilder buildMI(Block &MBB, BlockIt MBBI, unsigned Opcode,
                              unsigned DstReg) {
    return BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(Opcode), DstReg);
  }

  MachineRegisterInfo &getRegInfo(Block &MBB) { return MBB.getParent()->getRegInfo(); }

  bool expandArith(unsigned OpLo, unsigned OpHi, Block &MBB, BlockIt MBBI);
  bool expandLogic(unsigned Op, Block &MBB, BlockIt MBBI);
  bool expandLogicImm(unsigned Op, Block &MBB, BlockIt MBBI);
  bool isLogicImmOpRedundant(unsigned Op, unsigned ImmVal) const;

  template<typename Func>
  bool expandAtomic(Block &MBB, BlockIt MBBI, Func f);

  template<typename Func>
  bool expandAtomicBinaryOp(unsigned Opcode, Block &MBB, BlockIt MBBI, Func f);

  bool expandAtomicBinaryOp(unsigned Opcode, Block &MBB, BlockIt MBBI);

  bool expandAtomicArithmeticOp(unsigned MemOpcode,
                                unsigned ArithOpcode,
                                Block &MBB,
                                BlockIt MBBI);

  /// Scavenges a free GPR8 register for use.
  unsigned scavengeGPR8(MachineInstr &MI);
};

char SNESExpandPseudo::ID = 0;

bool SNESExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  BlockIt MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    BlockIt NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool SNESExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  bool Modified = false;

  const SNESSubtarget &STI = MF.getSubtarget<SNESSubtarget>();
  TRI = STI.getRegisterInfo();
  TII = STI.getInstrInfo();

  // We need to track liveness in order to use register scavenging.
  MF.getProperties().set(MachineFunctionProperties::Property::TracksLiveness);

  for (Block &MBB : MF) {
    bool ContinueExpanding = true;
    unsigned ExpandCount = 0;

    // Continue expanding the block until all pseudos are expanded.
    do {
      assert(ExpandCount < 10 && "pseudo expand limit reached");

      bool BlockModified = expandMBB(MBB);
      Modified |= BlockModified;
      ExpandCount++;

      ContinueExpanding = BlockModified;
    } while (ContinueExpanding);
  }

  return Modified;
}

bool SNESExpandPseudo::
expandArith(unsigned OpLo, unsigned OpHi, Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstLoReg, getKillRegState(DstIsKill))
    .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, getKillRegState(DstIsKill))
    .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

bool SNESExpandPseudo::
expandLogic(unsigned Op, Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, Op)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstLoReg, getKillRegState(DstIsKill))
    .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  // SREG is always implicitly dead
  MIBLO->getOperand(3).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, Op)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, getKillRegState(DstIsKill))
    .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

bool SNESExpandPseudo::
  isLogicImmOpRedundant(unsigned Op, unsigned ImmVal) const {

  // ANDI Rd, 0xff is redundant.
  if (Op == SNES::ANDIRdK && ImmVal == 0xff)
    return true;

  // ORI Rd, 0x0 is redundant.
  if (Op == SNES::ORIRdK && ImmVal == 0x0)
    return true;

  return false;
}

bool SNESExpandPseudo::
expandLogicImm(unsigned Op, Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  unsigned Imm = MI.getOperand(2).getImm();
  unsigned Lo8 = Imm & 0xff;
  unsigned Hi8 = (Imm >> 8) & 0xff;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  if (!isLogicImmOpRedundant(Op, Lo8)) {
    auto MIBLO = buildMI(MBB, MBBI, Op)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstLoReg, getKillRegState(SrcIsKill))
      .addImm(Lo8);

    // SREG is always implicitly dead
    MIBLO->getOperand(3).setIsDead();
  }

  if (!isLogicImmOpRedundant(Op, Hi8)) {
    auto MIBHI = buildMI(MBB, MBBI, Op)
      .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(DstHiReg, getKillRegState(SrcIsKill))
      .addImm(Hi8);

    if (ImpIsDead)
      MIBHI->getOperand(3).setIsDead();
  }

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::ADDWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(SNES::ADDRdRr, SNES::ADCRdRr, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::ADCWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(SNES::ADCRdRr, SNES::ADCRdRr, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::SUBWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(SNES::SUBRdRr, SNES::SBCRdRr, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::SUBIWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, SNES::SUBIRdK)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, SNES::SBCIRdK)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, getKillRegState(SrcIsKill));

  switch (MI.getOperand(2).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(2).getGlobal();
    int64_t Offs = MI.getOperand(2).getOffset();
    unsigned TF = MI.getOperand(2).getTargetFlags();
    MIBLO.addGlobalAddress(GV, Offs, TF | SNESII::MO_NEG | SNESII::MO_LO);
    MIBHI.addGlobalAddress(GV, Offs, TF | SNESII::MO_NEG | SNESII::MO_HI);
    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(2).getImm();
    MIBLO.addImm(Imm & 0xff);
    MIBHI.addImm((Imm >> 8) & 0xff);
    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::SBCWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandArith(SNES::SBCRdRr, SNES::SBCRdRr, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::SBCIWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  unsigned Imm = MI.getOperand(2).getImm();
  unsigned Lo8 = Imm & 0xff;
  unsigned Hi8 = (Imm >> 8) & 0xff;
  OpLo = SNES::SBCIRdK;
  OpHi = SNES::SBCIRdK;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstLoReg, getKillRegState(SrcIsKill))
    .addImm(Lo8);

  // SREG is always implicitly killed
  MIBLO->getOperand(4).setIsKill();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, getKillRegState(SrcIsKill))
    .addImm(Hi8);

  if (ImpIsDead)
    MIBHI->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::ANDWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(SNES::ANDRdRr, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::ANDIWRdK>(Block &MBB, BlockIt MBBI) {
  return expandLogicImm(SNES::ANDIRdK, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::ORWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(SNES::ORRdRr, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::ORIWRdK>(Block &MBB, BlockIt MBBI) {
  return expandLogicImm(SNES::ORIRdK, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::EORWRdRr>(Block &MBB, BlockIt MBBI) {
  return expandLogic(SNES::EORRdRr, MBB, MBBI);
}

template <>
bool SNESExpandPseudo::expand<SNES::COMWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = SNES::COMRd;
  OpHi = SNES::COMRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstLoReg, getKillRegState(DstIsKill));

  // SREG is always implicitly dead
  MIBLO->getOperand(2).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(2).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::CPWRdRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = SNES::CPRdRr;
  OpHi = SNES::CPCRdRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Low part
  buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, getKillRegState(DstIsKill))
    .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, getKillRegState(DstIsKill))
    .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::CPCWRdRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = SNES::CPCRdRr;
  OpHi = SNES::CPCRdRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, getKillRegState(DstIsKill))
    .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, getKillRegState(DstIsKill))
    .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LDIWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  OpLo = SNES::LDIRdK;
  OpHi = SNES::LDIRdK;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead));

  switch (MI.getOperand(1).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(1).getGlobal();
    int64_t Offs = MI.getOperand(1).getOffset();
    unsigned TF = MI.getOperand(1).getTargetFlags();

    MIBLO.addGlobalAddress(GV, Offs, TF | SNESII::MO_LO);
    MIBHI.addGlobalAddress(GV, Offs, TF | SNESII::MO_HI);
    break;
  }
  case MachineOperand::MO_BlockAddress: {
    const BlockAddress *BA = MI.getOperand(1).getBlockAddress();
    unsigned TF = MI.getOperand(1).getTargetFlags();

    MIBLO.add(MachineOperand::CreateBA(BA, TF | SNESII::MO_LO));
    MIBHI.add(MachineOperand::CreateBA(BA, TF | SNESII::MO_HI));
    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(1).getImm();

    MIBLO.addImm(Imm & 0xff);
    MIBHI.addImm((Imm >> 8) & 0xff);
    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LDSWRdK>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  OpLo = SNES::LDSRdK;
  OpHi = SNES::LDSRdK;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead));

  switch (MI.getOperand(1).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(1).getGlobal();
    int64_t Offs = MI.getOperand(1).getOffset();
    unsigned TF = MI.getOperand(1).getTargetFlags();

    MIBLO.addGlobalAddress(GV, Offs, TF);
    MIBHI.addGlobalAddress(GV, Offs + 1, TF);
    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(1).getImm();

    MIBLO.addImm(Imm);
    MIBHI.addImm(Imm + 1);
    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LDWRdPtr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned TmpReg = 0; // 0 for no temporary register
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = SNES::LDRdPtr;
  OpHi = SNES::LDDRdPtrQ;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Use a temporary register if src and dst registers are the same.
  if (DstReg == SrcReg)
    TmpReg = scavengeGPR8(MI);

  unsigned CurDstLoReg = (DstReg == SrcReg) ? TmpReg : DstLoReg;
  unsigned CurDstHiReg = (DstReg == SrcReg) ? TmpReg : DstHiReg;

  // Load low byte.
  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(CurDstLoReg, RegState::Define)
    .addReg(SrcReg);

  // Push low byte onto stack if necessary.
  if (TmpReg)
    buildMI(MBB, MBBI, SNES::PUSHRr).addReg(TmpReg);

  // Load high byte.
  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(CurDstHiReg, RegState::Define)
    .addReg(SrcReg, getKillRegState(SrcIsKill))
    .addImm(1);

  if (TmpReg) {
    // Move the high byte into the final destination.
    buildMI(MBB, MBBI, SNES::MOVRdRr).addReg(DstHiReg).addReg(TmpReg);

    // Move the low byte from the scratch space into the final destination.
    buildMI(MBB, MBBI, SNES::POPRd).addReg(DstLoReg);
  }

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LDWRdPtrPi>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsDead = MI.getOperand(1).isKill();
  OpLo = SNES::LDRdPtrPi;
  OpHi = SNES::LDRdPtrPi;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcReg, RegState::Define)
    .addReg(SrcReg, RegState::Kill);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcReg, RegState::Define | getDeadRegState(SrcIsDead))
    .addReg(SrcReg, RegState::Kill);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LDWRdPtrPd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsDead = MI.getOperand(1).isKill();
  OpLo = SNES::LDRdPtrPd;
  OpHi = SNES::LDRdPtrPd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcReg, RegState::Define)
    .addReg(SrcReg, RegState::Kill);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(SrcReg, RegState::Define | getDeadRegState(SrcIsDead))
    .addReg(SrcReg, RegState::Kill);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LDDWRdPtrQ>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned TmpReg = 0; // 0 for no temporary register
  unsigned SrcReg = MI.getOperand(1).getReg();
  unsigned Imm = MI.getOperand(2).getImm();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = SNES::LDDRdPtrQ;
  OpHi = SNES::LDDRdPtrQ;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  assert(Imm <= 63 && "Offset is out of range");

  // Use a temporary register if src and dst registers are the same.
  if (DstReg == SrcReg)
    TmpReg = scavengeGPR8(MI);

  unsigned CurDstLoReg = (DstReg == SrcReg) ? TmpReg : DstLoReg;
  unsigned CurDstHiReg = (DstReg == SrcReg) ? TmpReg : DstHiReg;

  // Load low byte.
  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(CurDstLoReg, RegState::Define)
    .addReg(SrcReg)
    .addImm(Imm);

  // Push low byte onto stack if necessary.
  if (TmpReg)
    buildMI(MBB, MBBI, SNES::PUSHRr).addReg(TmpReg);

  // Load high byte.
  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(CurDstHiReg, RegState::Define)
    .addReg(SrcReg, getKillRegState(SrcIsKill))
    .addImm(Imm + 1);

  if (TmpReg) {
    // Move the high byte into the final destination.
    buildMI(MBB, MBBI, SNES::MOVRdRr).addReg(DstHiReg).addReg(TmpReg);

    // Move the low byte from the scratch space into the final destination.
    buildMI(MBB, MBBI, SNES::POPRd).addReg(DstLoReg);
  }

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LPMWRdZ>(Block &MBB, BlockIt MBBI) {
  llvm_unreachable("wide LPM is unimplemented");
}

template <>
bool SNESExpandPseudo::expand<SNES::LPMWRdZPi>(Block &MBB, BlockIt MBBI) {
  llvm_unreachable("wide LPMPi is unimplemented");
}

template<typename Func>
bool SNESExpandPseudo::expandAtomic(Block &MBB, BlockIt MBBI, Func f) {
  // Remove the pseudo instruction.
  MachineInstr &MI = *MBBI;

  // Store the SREG.
  buildMI(MBB, MBBI, SNES::INRdA)
    .addReg(SCRATCH_REGISTER, RegState::Define)
    .addImm(SREG_ADDR);

  // Disable exceptions.
  buildMI(MBB, MBBI, SNES::BCLRs).addImm(7); // CLI

  f(MI);

  // Restore the status reg.
  buildMI(MBB, MBBI, SNES::OUTARr)
    .addImm(SREG_ADDR)
    .addReg(SCRATCH_REGISTER);

  MI.eraseFromParent();
  return true;
}

template<typename Func>
bool SNESExpandPseudo::expandAtomicBinaryOp(unsigned Opcode,
                                           Block &MBB,
                                           BlockIt MBBI,
                                           Func f) {
  return expandAtomic(MBB, MBBI, [&](MachineInstr &MI) {
      auto Op1 = MI.getOperand(0);
      auto Op2 = MI.getOperand(1);

      MachineInstr &NewInst =
          *buildMI(MBB, MBBI, Opcode).add(Op1).add(Op2).getInstr();
      f(NewInst);
  });
}

bool SNESExpandPseudo::expandAtomicBinaryOp(unsigned Opcode,
                                           Block &MBB,
                                           BlockIt MBBI) {
  return expandAtomicBinaryOp(Opcode, MBB, MBBI, [](MachineInstr &MI) {});
}

bool SNESExpandPseudo::expandAtomicArithmeticOp(unsigned Width,
                                               unsigned ArithOpcode,
                                               Block &MBB,
                                               BlockIt MBBI) {
  return expandAtomic(MBB, MBBI, [&](MachineInstr &MI) {
      auto Op1 = MI.getOperand(0);
      auto Op2 = MI.getOperand(1);

      unsigned LoadOpcode = (Width == 8) ? SNES::LDRdPtr : SNES::LDWRdPtr;
      unsigned StoreOpcode = (Width == 8) ? SNES::STPtrRr : SNES::STWPtrRr;

      // Create the load
      buildMI(MBB, MBBI, LoadOpcode).add(Op1).add(Op2);

      // Create the arithmetic op
      buildMI(MBB, MBBI, ArithOpcode).add(Op1).add(Op1).add(Op2);

      // Create the store
      buildMI(MBB, MBBI, StoreOpcode).add(Op2).add(Op1);
  });
}

unsigned SNESExpandPseudo::scavengeGPR8(MachineInstr &MI) {
  MachineBasicBlock &MBB = *MI.getParent();
  RegScavenger RS;

  RS.enterBasicBlock(MBB);
  RS.forward(MI);

  BitVector Candidates =
      TRI->getAllocatableSet
      (*MBB.getParent(), &SNES::GPR8RegClass);

  // Exclude all the registers being used by the instruction.
  for (MachineOperand &MO : MI.operands()) {
    if (MO.isReg() && MO.getReg() != 0 && !MO.isDef() &&
        !TargetRegisterInfo::isVirtualRegister(MO.getReg()))
      Candidates.reset(MO.getReg());
  }

  BitVector Available = RS.getRegsAvailable(&SNES::GPR8RegClass);
  Available &= Candidates;

  signed Reg = Available.find_first();
  assert(Reg != -1 && "ran out of registers");
  return Reg;
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoad8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicBinaryOp(SNES::LDRdPtr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoad16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicBinaryOp(SNES::LDWRdPtr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicStore8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicBinaryOp(SNES::STPtrRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicStore16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicBinaryOp(SNES::STWPtrRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadAdd8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(8, SNES::ADDRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadAdd16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(16, SNES::ADDWRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadSub8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(8, SNES::SUBRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadSub16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(16, SNES::SUBWRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadAnd8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(8, SNES::ANDRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadAnd16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(16, SNES::ANDWRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadOr8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(8, SNES::ORRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadOr16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(16, SNES::ORWRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadXor8>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(8, SNES::EORRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicLoadXor16>(Block &MBB, BlockIt MBBI) {
  return expandAtomicArithmeticOp(16, SNES::EORWRdRr, MBB, MBBI);
}

template<>
bool SNESExpandPseudo::expand<SNES::AtomicFence>(Block &MBB, BlockIt MBBI) {
  // On SNES, there is only one core and so atomic fences do nothing.
  MBBI->eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::STSWKRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = SNES::STSKRr;
  OpHi = SNES::STSKRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  // Write the high byte first in case this address belongs to a special
  // I/O address with a special temporary register.
  auto MIBHI = buildMI(MBB, MBBI, OpHi);
  auto MIBLO = buildMI(MBB, MBBI, OpLo);

  switch (MI.getOperand(0).getType()) {
  case MachineOperand::MO_GlobalAddress: {
    const GlobalValue *GV = MI.getOperand(0).getGlobal();
    int64_t Offs = MI.getOperand(0).getOffset();
    unsigned TF = MI.getOperand(0).getTargetFlags();

    MIBLO.addGlobalAddress(GV, Offs, TF);
    MIBHI.addGlobalAddress(GV, Offs + 1, TF);
    break;
  }
  case MachineOperand::MO_Immediate: {
    unsigned Imm = MI.getOperand(0).getImm();

    MIBLO.addImm(Imm);
    MIBHI.addImm(Imm + 1);
    break;
  }
  default:
    llvm_unreachable("Unknown operand type!");
  }

  MIBLO.addReg(SrcLoReg, getKillRegState(SrcIsKill));
  MIBHI.addReg(SrcHiReg, getKillRegState(SrcIsKill));

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::STWPtrRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = SNES::STPtrRr;
  OpHi = SNES::STDPtrQRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  //:TODO: need to reverse this order like inw and stsw?
  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstReg)
    .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstReg)
    .addImm(1)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::STWPtrPiRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(3).getImm();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(2).isKill();
  OpLo = SNES::STPtrPiRr;
  OpHi = SNES::STPtrPiRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstReg, RegState::Define)
    .addReg(DstReg, RegState::Kill)
    .addReg(SrcLoReg, getKillRegState(SrcIsKill))
    .addImm(Imm);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstReg, RegState::Kill)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill))
    .addImm(Imm);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::STWPtrPdRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(3).getImm();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(2).isKill();
  OpLo = SNES::STPtrPdRr;
  OpHi = SNES::STPtrPdRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstReg, RegState::Define)
    .addReg(DstReg, RegState::Kill)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill))
    .addImm(Imm);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstReg, RegState::Kill)
    .addReg(SrcLoReg, getKillRegState(SrcIsKill))
    .addImm(Imm);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::STDWPtrQRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(1).getImm();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  OpLo = SNES::STDPtrQRr;
  OpHi = SNES::STDPtrQRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  assert(Imm <= 63 && "Offset is out of range");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstReg)
    .addImm(Imm)
    .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstReg, getKillRegState(DstIsKill))
    .addImm(Imm + 1)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::INWRdA>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned Imm = MI.getOperand(1).getImm();
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  OpLo = SNES::INRdA;
  OpHi = SNES::INRdA;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  assert(Imm <= 63 && "Address is out of range");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addImm(Imm);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addImm(Imm + 1);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::OUTWARr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned Imm = MI.getOperand(0).getImm();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = SNES::OUTARr;
  OpHi = SNES::OUTARr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  assert(Imm <= 63 && "Address is out of range");

  // 16 bit I/O writes need the high byte first
  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addImm(Imm + 1)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill));

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addImm(Imm)
    .addReg(SrcLoReg, getKillRegState(SrcIsKill));

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::PUSHWRr>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned SrcReg = MI.getOperand(0).getReg();
  bool SrcIsKill = MI.getOperand(0).isKill();
  unsigned Flags = MI.getFlags();
  OpLo = SNES::PUSHRr;
  OpHi = SNES::PUSHRr;
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  // Low part
  buildMI(MBB, MBBI, OpLo)
    .addReg(SrcLoReg, getKillRegState(SrcIsKill))
    .setMIFlags(Flags);

  // High part
  buildMI(MBB, MBBI, OpHi)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill))
    .setMIFlags(Flags);

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::POPWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned Flags = MI.getFlags();
  OpLo = SNES::POPRd;
  OpHi = SNES::POPRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  buildMI(MBB, MBBI, OpHi, DstHiReg).setMIFlags(Flags); // High
  buildMI(MBB, MBBI, OpLo, DstLoReg).setMIFlags(Flags); // Low

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LSLWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = SNES::LSLRd;
  OpHi = SNES::ROLRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Low part
  buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstLoReg, getKillRegState(DstIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBHI->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBHI->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::LSRWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = SNES::RORRd;
  OpHi = SNES::LSRRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // High part
  buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, getKillRegState(DstIsKill));

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstLoReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBLO->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::RORWRd>(Block &MBB, BlockIt MBBI) {
  llvm_unreachable("RORW unimplemented");
  return false;
}

template <>
bool SNESExpandPseudo::expand<SNES::ROLWRd>(Block &MBB, BlockIt MBBI) {
  llvm_unreachable("ROLW unimplemented");
  return false;
}

template <>
bool SNESExpandPseudo::expand<SNES::ASRWRd>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = SNES::RORRd;
  OpHi = SNES::ASRRd;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // High part
  buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, getKillRegState(DstIsKill));

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstLoReg, getKillRegState(DstIsKill));

  if (ImpIsDead)
    MIBLO->getOperand(2).setIsDead();

  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <> bool SNESExpandPseudo::expand<SNES::SEXT>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned DstLoReg, DstHiReg;
  // sext R17:R16, R17
  // mov     r16, r17
  // lsl     r17
  // sbc     r17, r17
  // sext R17:R16, R13
  // mov     r16, r13
  // mov     r17, r13
  // lsl     r17
  // sbc     r17, r17
  // sext R17:R16, R16
  // mov     r17, r16
  // lsl     r17
  // sbc     r17, r17
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  if (SrcReg != DstLoReg) {
    auto MOV = buildMI(MBB, MBBI, SNES::MOVRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(SrcReg);

    if (SrcReg == DstHiReg) {
      MOV->getOperand(1).setIsKill();
    }
  }

  if (SrcReg != DstHiReg) {
    buildMI(MBB, MBBI, SNES::MOVRdRr)
      .addReg(DstHiReg, RegState::Define)
      .addReg(SrcReg, getKillRegState(SrcIsKill));
  }

  buildMI(MBB, MBBI, SNES::LSLRd)
    .addReg(DstHiReg, RegState::Define)
    .addReg(DstHiReg, RegState::Kill);

  auto SBC = buildMI(MBB, MBBI, SNES::SBCRdRr)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, RegState::Kill)
    .addReg(DstHiReg, RegState::Kill);

  if (ImpIsDead)
    SBC->getOperand(3).setIsDead();

  // SREG is always implicitly killed
  SBC->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <> bool SNESExpandPseudo::expand<SNES::ZEXT>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned DstLoReg, DstHiReg;
  // zext R25:R24, R20
  // mov      R24, R20
  // eor      R25, R25
  // zext R25:R24, R24
  // eor      R25, R25
  // zext R25:R24, R25
  // mov      R24, R25
  // eor      R25, R25
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  if (SrcReg != DstLoReg) {
    buildMI(MBB, MBBI, SNES::MOVRdRr)
      .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      .addReg(SrcReg, getKillRegState(SrcIsKill));
  }

  auto EOR = buildMI(MBB, MBBI, SNES::EORRdRr)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addReg(DstHiReg, RegState::Kill)
    .addReg(DstHiReg, RegState::Kill);

  if (ImpIsDead)
    EOR->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::SPREAD>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  unsigned Flags = MI.getFlags();
  OpLo = SNES::INRdA;
  OpHi = SNES::INRdA;
  TRI->splitReg(DstReg, DstLoReg, DstHiReg);

  // Low part
  buildMI(MBB, MBBI, OpLo)
    .addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    .addImm(0x3d)
    .setMIFlags(Flags);

  // High part
  buildMI(MBB, MBBI, OpHi)
    .addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    .addImm(0x3e)
    .setMIFlags(Flags);

  MI.eraseFromParent();
  return true;
}

template <>
bool SNESExpandPseudo::expand<SNES::SPWRITE>(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  unsigned SrcLoReg, SrcHiReg;
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  unsigned Flags = MI.getFlags();
  TRI->splitReg(SrcReg, SrcLoReg, SrcHiReg);

  buildMI(MBB, MBBI, SNES::INRdA)
    .addReg(SNES::R0, RegState::Define)
    .addImm(SREG_ADDR)
    .setMIFlags(Flags);

  buildMI(MBB, MBBI, SNES::BCLRs).addImm(0x07).setMIFlags(Flags);

  buildMI(MBB, MBBI, SNES::OUTARr)
    .addImm(0x3e)
    .addReg(SrcHiReg, getKillRegState(SrcIsKill))
    .setMIFlags(Flags);

  buildMI(MBB, MBBI, SNES::OUTARr)
    .addImm(SREG_ADDR)
    .addReg(SNES::R0, RegState::Kill)
    .setMIFlags(Flags);

  buildMI(MBB, MBBI, SNES::OUTARr)
    .addImm(0x3d)
    .addReg(SrcLoReg, getKillRegState(SrcIsKill))
    .setMIFlags(Flags);

  MI.eraseFromParent();
  return true;
}

bool SNESExpandPseudo::expandMI(Block &MBB, BlockIt MBBI) {
  MachineInstr &MI = *MBBI;
  int Opcode = MBBI->getOpcode();

#define EXPAND(Op)               \
  case Op:                       \
    return expand<Op>(MBB, MI)

  switch (Opcode) {
    EXPAND(SNES::ADDWRdRr);
    EXPAND(SNES::ADCWRdRr);
    EXPAND(SNES::SUBWRdRr);
    EXPAND(SNES::SUBIWRdK);
    EXPAND(SNES::SBCWRdRr);
    EXPAND(SNES::SBCIWRdK);
    EXPAND(SNES::ANDWRdRr);
    EXPAND(SNES::ANDIWRdK);
    EXPAND(SNES::ORWRdRr);
    EXPAND(SNES::ORIWRdK);
    EXPAND(SNES::EORWRdRr);
    EXPAND(SNES::COMWRd);
    EXPAND(SNES::CPWRdRr);
    EXPAND(SNES::CPCWRdRr);
    EXPAND(SNES::LDIWRdK);
    EXPAND(SNES::LDSWRdK);
    EXPAND(SNES::LDWRdPtr);
    EXPAND(SNES::LDWRdPtrPi);
    EXPAND(SNES::LDWRdPtrPd);
  case SNES::LDDWRdYQ: //:FIXME: remove this once PR13375 gets fixed
    EXPAND(SNES::LDDWRdPtrQ);
    EXPAND(SNES::LPMWRdZ);
    EXPAND(SNES::LPMWRdZPi);
    EXPAND(SNES::AtomicLoad8);
    EXPAND(SNES::AtomicLoad16);
    EXPAND(SNES::AtomicStore8);
    EXPAND(SNES::AtomicStore16);
    EXPAND(SNES::AtomicLoadAdd8);
    EXPAND(SNES::AtomicLoadAdd16);
    EXPAND(SNES::AtomicLoadSub8);
    EXPAND(SNES::AtomicLoadSub16);
    EXPAND(SNES::AtomicLoadAnd8);
    EXPAND(SNES::AtomicLoadAnd16);
    EXPAND(SNES::AtomicLoadOr8);
    EXPAND(SNES::AtomicLoadOr16);
    EXPAND(SNES::AtomicLoadXor8);
    EXPAND(SNES::AtomicLoadXor16);
    EXPAND(SNES::AtomicFence);
    EXPAND(SNES::STSWKRr);
    EXPAND(SNES::STWPtrRr);
    EXPAND(SNES::STWPtrPiRr);
    EXPAND(SNES::STWPtrPdRr);
    EXPAND(SNES::STDWPtrQRr);
    EXPAND(SNES::INWRdA);
    EXPAND(SNES::OUTWARr);
    EXPAND(SNES::PUSHWRr);
    EXPAND(SNES::POPWRd);
    EXPAND(SNES::LSLWRd);
    EXPAND(SNES::LSRWRd);
    EXPAND(SNES::RORWRd);
    EXPAND(SNES::ROLWRd);
    EXPAND(SNES::ASRWRd);
    EXPAND(SNES::SEXT);
    EXPAND(SNES::ZEXT);
    EXPAND(SNES::SPREAD);
    EXPAND(SNES::SPWRITE);
  }
#undef EXPAND
  return false;
}

} // end of anonymous namespace

INITIALIZE_PASS(SNESExpandPseudo, "snes-expand-pseudo",
                SNES_EXPAND_PSEUDO_NAME, false, false)
namespace llvm {

FunctionPass *createSNESExpandPseudoPass() { return new SNESExpandPseudo(); }

} // end of namespace llvm
