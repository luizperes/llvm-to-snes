//===-- SNESFrameLowering.cpp - SNES Frame Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the SNES implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "SNESFrameLowering.h"

#include "SNES.h"
#include "SNESInstrInfo.h"
#include "SNESMachineFunctionInfo.h"
#include "SNESTargetMachine.h"
#include "MCTargetDesc/SNESMCTargetDesc.h"

#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"

#include <vector>

namespace llvm {

SNESFrameLowering::SNESFrameLowering()
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 1, -2) {}

bool SNESFrameLowering::canSimplifyCallFramePseudos(
    const MachineFunction &MF) const {
  // Always simplify call frame pseudo instructions, even when
  // hasReservedCallFrame is false.
  return true;
}

bool SNESFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  // Reserve call frame memory in function prologue under the following
  // conditions:
  // - Y pointer is reserved to be the frame pointer.
  // - The function does not contain variable sized objects.

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return hasFP(MF) && !MFI.hasVarSizedObjects();
}

void SNESFrameLowering::emitPrologue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
// TODO: check what we will use on the code below
/*
  MachineBasicBlock::iterator MBBI = MBB.begin();
  CallingConv::ID CallConv = MF.getFunction()->getCallingConv();
  DebugLoc DL = (MBBI != MBB.end()) ? MBBI->getDebugLoc() : DebugLoc();
  const SNESSubtarget &STI = MF.getSubtarget<SNESSubtarget>();
  const SNESInstrInfo &TII = *STI.getInstrInfo();
  bool HasFP = hasFP(MF);

  // Interrupt handlers re-enable interrupts in function entry.
  if (CallConv == CallingConv::SNES_INTR) {
    BuildMI(MBB, MBBI, DL, TII.get(SNES::BSETs))
        .addImm(0x07)
        .setMIFlag(MachineInstr::FrameSetup);
  }

  // Save the frame pointer if we have one.
  if (HasFP) {
    BuildMI(MBB, MBBI, DL, TII.get(SNES::PUSHWRr))
        .addReg(SNES::R29R28, RegState::Kill)
        .setMIFlag(MachineInstr::FrameSetup);
  }

  // Emit special prologue code to save R1, A and P in interrupt/signal
  // handlers before saving any other registers.
  if (CallConv == CallingConv::SNES_INTR ||
      CallConv == CallingConv::SNES_SIGNAL) {
    BuildMI(MBB, MBBI, DL, TII.get(SNES::PUSHWRr))
        .addReg(SNES::AA, RegState::Kill)
        .setMIFlag(MachineInstr::FrameSetup);

    BuildMI(MBB, MBBI, DL, TII.get(SNES::INRdA), SNES::A)
        .addImm(0x3f)
        .setMIFlag(MachineInstr::FrameSetup);
    BuildMI(MBB, MBBI, DL, TII.get(SNES::PUSHRr))
        .addReg(SNES::A, RegState::Kill)
        .setMIFlag(MachineInstr::FrameSetup);
    BuildMI(MBB, MBBI, DL, TII.get(SNES::EORRdRr))
        .addReg(SNES::A, RegState::Define)
        .addReg(SNES::A, RegState::Kill)
        .addReg(SNES::A, RegState::Kill)
        .setMIFlag(MachineInstr::FrameSetup);
  }

  // Early exit if the frame pointer is not needed in this function.
  if (!HasFP) {
    return;
  }

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const SNESMachineFunctionInfo *AFI = MF.getInfo<SNESMachineFunctionInfo>();
  unsigned FrameSize = MFI.getStackSize() - AFI->getCalleeSavedFrameSize();

  // Skip the callee-saved push instructions.
  while (
      (MBBI != MBB.end()) && MBBI->getFlag(MachineInstr::FrameSetup) &&
      (MBBI->getOpcode() == SNES::PUSHRr || MBBI->getOpcode() == SNES::PUSHWRr)) {
    ++MBBI;
  }

  // Update Y with the new base value.
  BuildMI(MBB, MBBI, DL, TII.get(SNES::SPREAD), SNES::R29R28)
      .addReg(SNES::SP)
      .setMIFlag(MachineInstr::FrameSetup);

  // Mark the FramePtr as live-in in every block except the entry.
  for (MachineFunction::iterator I = std::next(MF.begin()), E = MF.end();
       I != E; ++I) {
    I->addLiveIn(SNES::R29R28);
  }

  if (!FrameSize) {
    return;
  }

  // Reserve the necessary frame memory by doing FP -= <size>.
  unsigned Opcode = (isUInt<6>(FrameSize)) ? SNES::SBIWRdK : SNES::SUBIWRdK;

  MachineInstr *MI = BuildMI(MBB, MBBI, DL, TII.get(Opcode), SNES::R29R28)
                         .addReg(SNES::R29R28, RegState::Kill)
                         .addImm(FrameSize)
                         .setMIFlag(MachineInstr::FrameSetup);
  // The P implicit def is dead.
  MI->getOperand(3).setIsDead();

  // Write back R29R28 to SP and temporarily disable interrupts.
  BuildMI(MBB, MBBI, DL, TII.get(SNES::SPWRITE), SNES::SP)
      .addReg(SNES::R29R28)
      .setMIFlag(MachineInstr::FrameSetup);
*/
}

void SNESFrameLowering::emitEpilogue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
// TODO: check what will be used on the code below
/*
  CallingConv::ID CallConv = MF.getFunction()->getCallingConv();
  bool isHandler = (CallConv == CallingConv::SNES_INTR ||
                    CallConv == CallingConv::SNES_SIGNAL);

  // Early exit if the frame pointer is not needed in this function except for
  // signal/interrupt handlers where special code generation is required.
  if (!hasFP(MF) && !isHandler) {
    return;
  }

  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  assert(MBBI->getDesc().isReturn() &&
         "Can only insert epilog into returning blocks");

  DebugLoc DL = MBBI->getDebugLoc();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const SNESMachineFunctionInfo *AFI = MF.getInfo<SNESMachineFunctionInfo>();
  unsigned FrameSize = MFI.getStackSize() - AFI->getCalleeSavedFrameSize();
  const SNESSubtarget &STI = MF.getSubtarget<SNESSubtarget>();
  const SNESInstrInfo &TII = *STI.getInstrInfo();

  // Emit special epilogue code to restore R1, A and P in interrupt/signal
  // handlers at the very end of the function, just before reti.
  if (isHandler) {
    BuildMI(MBB, MBBI, DL, TII.get(SNES::POPRd), SNES::A);
    BuildMI(MBB, MBBI, DL, TII.get(SNES::OUTARr))
        .addImm(0x3f)
        .addReg(SNES::A, RegState::Kill);
    BuildMI(MBB, MBBI, DL, TII.get(SNES::POPWRd), SNES::AA);
  }

  if (hasFP(MF))
    BuildMI(MBB, MBBI, DL, TII.get(SNES::POPWRd), SNES::R29R28);

  // Early exit if there is no need to restore the frame pointer.
  if (!FrameSize) {
    return;
  }

  // Skip the callee-saved pop instructions.
  while (MBBI != MBB.begin()) {
    MachineBasicBlock::iterator PI = std::prev(MBBI);
    int Opc = PI->getOpcode();

    if (Opc != SNES::POPRd && Opc != SNES::POPWRd && !PI->isTerminator()) {
      break;
    }

    --MBBI;
  }

  unsigned Opcode;

  // Select the optimal opcode depending on how big it is.
  if (isUInt<6>(FrameSize)) {
    Opcode = SNES::ADIWRdK;
  } else {
    Opcode = SNES::SUBIWRdK;
    FrameSize = -FrameSize;
  }

  // Restore the frame pointer by doing FP += <size>.
  MachineInstr *MI = BuildMI(MBB, MBBI, DL, TII.get(Opcode), SNES::R29R28)
                         .addReg(SNES::R29R28, RegState::Kill)
                         .addImm(FrameSize);
  // The P implicit def is dead.
  MI->getOperand(3).setIsDead();

  // Write back R29R28 to SP and temporarily disable interrupts.
  BuildMI(MBB, MBBI, DL, TII.get(SNES::SPWRITE), SNES::SP)
      .addReg(SNES::R29R28, RegState::Kill);
*/
}

// Return true if the specified function should have a dedicated frame
// pointer register. This is true if the function meets any of the following
// conditions:
//  - a register has been spilled
//  - has allocas
//  - input arguments are passed using the stack
//
// Notice that strictly this is not a frame pointer because it contains SP after
// frame allocation instead of having the original SP in function entry.
bool SNESFrameLowering::hasFP(const MachineFunction &MF) const {
  const SNESMachineFunctionInfo *FuncInfo = MF.getInfo<SNESMachineFunctionInfo>();

  return (FuncInfo->getHasSpills() || FuncInfo->getHasAllocas() ||
          FuncInfo->getHasStackArgs());
}

bool SNESFrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    const std::vector<CalleeSavedInfo> &CSI,
    const TargetRegisterInfo *TRI) const {
  if (CSI.empty()) {
    return false;
  }

  unsigned CalleeFrameSize = 0;
  DebugLoc DL = MBB.findDebugLoc(MI);
  MachineFunction &MF = *MBB.getParent();
  const SNESSubtarget &STI = MF.getSubtarget<SNESSubtarget>();
  const TargetInstrInfo &TII = *STI.getInstrInfo();
  SNESMachineFunctionInfo *SNESFI = MF.getInfo<SNESMachineFunctionInfo>();

  for (unsigned i = CSI.size(); i != 0; --i) {
    unsigned Reg = CSI[i - 1].getReg();
    bool IsNotLiveIn = !MBB.isLiveIn(Reg);

    assert(TRI->getRegSizeInBits(*TRI->getMinimalPhysRegClass(Reg)) == 16 &&
           "Invalid register size");

    // Add the callee-saved register as live-in only if it is not already a
    // live-in register, this usually happens with arguments that are passed
    // through callee-saved registers.
    if (IsNotLiveIn) {
      MBB.addLiveIn(Reg);
    }

    // Do not kill the register when it is an input argument.
    BuildMI(MBB, MI, DL, TII.get(SNES::PUSHRr))
        .addReg(Reg, getKillRegState(IsNotLiveIn))
        .setMIFlag(MachineInstr::FrameSetup);
    ++CalleeFrameSize;
  }

  SNESFI->setCalleeSavedFrameSize(CalleeFrameSize);

  return true;
}

bool SNESFrameLowering::restoreCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    const std::vector<CalleeSavedInfo> &CSI,
    const TargetRegisterInfo *TRI) const {
  if (CSI.empty()) {
    return false;
  }

  DebugLoc DL = MBB.findDebugLoc(MI);
  const MachineFunction &MF = *MBB.getParent();
  const SNESSubtarget &STI = MF.getSubtarget<SNESSubtarget>();
  const TargetInstrInfo &TII = *STI.getInstrInfo();

  for (const CalleeSavedInfo &CCSI : CSI) {
    unsigned Reg = CCSI.getReg();

    assert(TRI->getRegSizeInBits(*TRI->getMinimalPhysRegClass(Reg)) == 16 &&
           "Invalid register size");

    BuildMI(MBB, MI, DL, TII.get(SNES::POPRd), Reg);
  }

  return true;
}

/// Replace pseudo store instructions that pass arguments through the stack with
/// real instructions. If insertPushes is true then all instructions are
/// replaced with push instructions, otherwise regular std instructions are
/// inserted.
static void fixStackStores(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI,
                           const TargetInstrInfo &TII, bool insertPushes) {
  const SNESSubtarget &STI = MBB.getParent()->getSubtarget<SNESSubtarget>();
  const TargetRegisterInfo &TRI = *STI.getRegisterInfo();

  // Iterate through the BB until we hit a call instruction or we reach the end.
  for (auto I = MI, E = MBB.end(); I != E && !I->isCall();) {
    MachineBasicBlock::iterator NextMI = std::next(I);
    MachineInstr &MI = *I;
    unsigned Opcode = I->getOpcode();

    // Only care of pseudo store instructions where SP is the base pointer.
    if (Opcode != SNES::STDSPQRr && Opcode != SNES::STDWSPQRr) {
      I = NextMI;
      continue;
    }

    assert(MI.getOperand(0).getReg() == SNES::SP &&
           "Invalid register, should be SP!");
    if (insertPushes) {
      // Replace this instruction with a push.
      unsigned SrcReg = MI.getOperand(2).getReg();
      bool SrcIsKill = MI.getOperand(2).isKill();

      // We can't use PUSHWRr here because when expanded the order of the new
      // instructions are reversed from what we need. Perform the expansion now.
      if (Opcode == SNES::STDWSPQRr) {
        BuildMI(MBB, I, MI.getDebugLoc(), TII.get(SNES::PUSHRr))
            .addReg(TRI.getSubReg(SrcReg, SNES::sub_hi),
                    getKillRegState(SrcIsKill));
        BuildMI(MBB, I, MI.getDebugLoc(), TII.get(SNES::PUSHRr))
            .addReg(TRI.getSubReg(SrcReg, SNES::sub_lo),
                    getKillRegState(SrcIsKill));
      } else {
        BuildMI(MBB, I, MI.getDebugLoc(), TII.get(SNES::PUSHRr))
            .addReg(SrcReg, getKillRegState(SrcIsKill));
      }

      MI.eraseFromParent();
      I = NextMI;
      continue;
    }

    // Replace this instruction with a regular store. Use Y as the base
    // pointer since it is guaranteed to contain a copy of SP.
    unsigned STOpc =
        (Opcode == SNES::STDWSPQRr) ? SNES::STDWPtrQRr : SNES::STDPtrQRr;

    MI.setDesc(TII.get(STOpc));
    MI.getOperand(0).setReg(SNES::A);

    I = NextMI;
  }
}

MachineBasicBlock::iterator SNESFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI) const {
  const SNESSubtarget &STI = MF.getSubtarget<SNESSubtarget>();
  const TargetFrameLowering &TFI = *STI.getFrameLowering();
  const SNESInstrInfo &TII = *STI.getInstrInfo();

  // There is nothing to insert when the call frame memory is allocated during
  // function entry. Delete the call frame pseudo and replace all pseudo stores
  // with real store instructions.
  if (TFI.hasReservedCallFrame(MF)) {
    fixStackStores(MBB, MI, TII, false);
    return MBB.erase(MI);
  }

  DebugLoc DL = MI->getDebugLoc();
  unsigned int Opcode = MI->getOpcode();
  int Amount = TII.getFrameSize(*MI);

  // Adjcallstackup does not need to allocate stack space for the call, instead
  // we insert push instructions that will allocate the necessary stack.
  // For adjcallstackdown we convert it into an 'adiw reg, <amt>' handling
  // the read and write of SP in I/O space.
  if (Amount != 0) {
    assert(TFI.getStackAlignment() == 1 && "Unsupported stack alignment");

    if (Opcode == TII.getCallFrameSetupOpcode()) {
      fixStackStores(MBB, MI, TII, true);
    } else {
      assert(Opcode == TII.getCallFrameDestroyOpcode());

      // Select the best opcode to adjust SP based on the offset size.
      unsigned addOpcode;
      // TODO: check if we need to use the line below
      // if (isUInt<6>(Amount)) {
      //   addOpcode = SNES::ADIWRdK;
      // } else {
        addOpcode = SNES::SUBIWRdK;
        Amount = -Amount;
      // }

      // Build the instruction sequence.
      BuildMI(MBB, MI, DL, TII.get(SNES::SPREAD), SNES::A).addReg(SNES::SP);

      MachineInstr *New = BuildMI(MBB, MI, DL, TII.get(addOpcode), SNES::A)
                              .addReg(SNES::A, RegState::Kill)
                              .addImm(Amount);
      New->getOperand(3).setIsDead();

      BuildMI(MBB, MI, DL, TII.get(SNES::SPWRITE), SNES::SP)
          .addReg(SNES::A, RegState::Kill);
    }
  }

  return MBB.erase(MI);
}

void SNESFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                            BitVector &SavedRegs,
                                            RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  // If we have a frame pointer, the Y register needs to be saved as well.
  // We don't do that here however - the prologue and epilogue generation
  // code will handle it specially.
}
/// The frame analyzer pass.
///
/// Scans the function for allocas and used arguments
/// that are passed through the stack.
struct SNESFrameAnalyzer : public MachineFunctionPass {
  static char ID;
  SNESFrameAnalyzer() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) {
    const MachineFrameInfo &MFI = MF.getFrameInfo();
    SNESMachineFunctionInfo *FuncInfo = MF.getInfo<SNESMachineFunctionInfo>();

    // If there are no fixed frame indexes during this stage it means there
    // are allocas present in the function.
    if (MFI.getNumObjects() != MFI.getNumFixedObjects()) {
      // Check for the type of allocas present in the function. We only care
      // about fixed size allocas so do not give false positives if only
      // variable sized allocas are present.
      for (unsigned i = 0, e = MFI.getObjectIndexEnd(); i != e; ++i) {
        // Variable sized objects have size 0.
        if (MFI.getObjectSize(i)) {
          FuncInfo->setHasAllocas(true);
          break;
        }
      }
    }

    // If there are fixed frame indexes present, scan the function to see if
    // they are really being used.
    if (MFI.getNumFixedObjects() == 0) {
      return false;
    }

    // Ok fixed frame indexes present, now scan the function to see if they
    // are really being used, otherwise we can ignore them.
    for (const MachineBasicBlock &BB : MF) {
      for (const MachineInstr &MI : BB) {
        int Opcode = MI.getOpcode();

        if ((Opcode != SNES::LDDRdPtrQ) && (Opcode != SNES::LDDWRdPtrQ) &&
            (Opcode != SNES::STDPtrQRr) && (Opcode != SNES::STDWPtrQRr)) {
          continue;
        }

        for (const MachineOperand &MO : MI.operands()) {
          if (!MO.isFI()) {
            continue;
          }

          if (MFI.isFixedObjectIndex(MO.getIndex())) {
            FuncInfo->setHasStackArgs(true);
            return false;
          }
        }
      }
    }

    return false;
  }

  StringRef getPassName() const { return "SNES Frame Analyzer"; }
};

char SNESFrameAnalyzer::ID = 0;

/// Creates instance of the frame analyzer pass.
FunctionPass *createSNESFrameAnalyzerPass() { return new SNESFrameAnalyzer(); }

/// Create the Dynalloca Stack Pointer Save/Restore pass.
/// Insert a copy of SP before allocating the dynamic stack memory and restore
/// it in function exit to restore the original SP state. This avoids the need
/// of reserving a register pair for a frame pointer.
struct SNESDynAllocaSR : public MachineFunctionPass {
  static char ID;
  SNESDynAllocaSR() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) {
    // Early exit when there are no variable sized objects in the function.
    if (!MF.getFrameInfo().hasVarSizedObjects()) {
      return false;
    }

    const SNESSubtarget &STI = MF.getSubtarget<SNESSubtarget>();
    const TargetInstrInfo &TII = *STI.getInstrInfo();
    MachineBasicBlock &EntryMBB = MF.front();
    MachineBasicBlock::iterator MBBI = EntryMBB.begin();
    DebugLoc DL = EntryMBB.findDebugLoc(MBBI);

    unsigned SPCopy =
        MF.getRegInfo().createVirtualRegister(&SNES::MainRegsRegClass);

    // Create a copy of SP in function entry before any dynallocas are
    // inserted.
    BuildMI(EntryMBB, MBBI, DL, TII.get(SNES::COPY), SPCopy).addReg(SNES::SP);

    // Restore SP in all exit basic blocks.
    for (MachineBasicBlock &MBB : MF) {
      // If last instruction is a return instruction, add a restore copy.
      if (!MBB.empty() && MBB.back().isReturn()) {
        MBBI = MBB.getLastNonDebugInstr();
        DL = MBBI->getDebugLoc();
        BuildMI(MBB, MBBI, DL, TII.get(SNES::COPY), SNES::SP)
            .addReg(SPCopy, RegState::Kill);
      }
    }

    return true;
  }

  StringRef getPassName() const {
    return "SNES dynalloca stack pointer save/restore";
  }
};

char SNESDynAllocaSR::ID = 0;

/// createSNESDynAllocaSRPass - returns an instance of the dynalloca stack
/// pointer save/restore pass.
FunctionPass *createSNESDynAllocaSRPass() { return new SNESDynAllocaSR(); }

} // end of namespace llvm

