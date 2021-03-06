//===-- SNESInstPrinter.cpp - Convert SNES MCInst to assembly syntax --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an SNES MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "SNESInstPrinter.h"

#include "MCTargetDesc/SNESMCTargetDesc.h"

#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

#include <cstring>

#define DEBUG_TYPE "asm-printer"

namespace llvm {

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#include "SNESGenAsmWriter.inc"

void SNESInstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                               StringRef Annot, const MCSubtargetInfo &STI) {
  unsigned Opcode = MI->getOpcode();

  // First handle load and store instructions with postinc or predec
  // of the form "ld reg, X+".
  // TODO: We should be able to rewrite this using TableGen data.
  switch (Opcode) {
  case SNES::LDRdPtr:
  case SNES::LDRdPtrPi:
  case SNES::LDRdPtrPd:
    O << "\tld\t";
    printOperand(MI, 0, O);
    O << ", ";

    if (Opcode == SNES::LDRdPtrPd)
      O << '-';

    printOperand(MI, 1, O);

    if (Opcode == SNES::LDRdPtrPi)
      O << '+';
    break;
  case SNES::STPtrRr:
    O << "\tst\t";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
    break;
  case SNES::STPtrPiRr:
  case SNES::STPtrPdRr:
    O << "\tst\t";

    if (Opcode == SNES::STPtrPdRr)
      O << '-';

    printOperand(MI, 1, O);

    if (Opcode == SNES::STPtrPiRr)
      O << '+';

    O << ", ";
    printOperand(MI, 2, O);
    break;
  default:
    if (!printAliasInstr(MI, O))
      printInstruction(MI, O);

    printAnnotation(O, Annot);
    break;
  }
}

const char *SNESInstPrinter::getPrettyRegisterName(unsigned RegNum,
                                                  MCRegisterInfo const &MRI) {
  // GCC prints register pairs by just printing the lower register
  // If the register contains a subregister, print it instead
  if (MRI.getNumSubRegIndices() > 0) {
    unsigned RegLoNum = MRI.getSubReg(RegNum, SNES::sub_lo);
    RegNum = (RegLoNum != SNES::NoRegister) ? RegLoNum : RegNum;
  }

  return getRegisterName(RegNum);
}

void SNESInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                  raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  // const MCOperandInfo &MOI = this->MII.get(MI->getOpcode()).OpInfo[OpNo];

  if (Op.isReg()) {
    bool isPtrReg = false;
    // bool isPtrReg = (MOI.RegClass == SNES::PTRREGSRegClassID) ||
    //                 (MOI.RegClass == SNES::PTRDISPREGSRegClassID) ||
    //                 (MOI.RegClass == SNES::XRegClassID);

    if (isPtrReg) {
      O << getRegisterName(Op.getReg());
    } else {
      O << getPrettyRegisterName(Op.getReg(), MRI);
    }
  } else if (Op.isImm()) {
    O << Op.getImm();
  } else {
    assert(Op.isExpr() && "Unknown operand kind in printOperand");
    O << *Op.getExpr();
  }
}

/// This is used to print an immediate value that ends up
/// being encoded as a pc-relative value.
void SNESInstPrinter::printPCRelImm(const MCInst *MI, unsigned OpNo,
                                   raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);

  if (Op.isImm()) {
    int64_t Imm = Op.getImm();
    O << '.';

    // Print a position sign if needed.
    // Negative values have their sign printed automatically.
    if (Imm >= 0)
      O << '+';

    O << Imm;
  } else {
    assert(Op.isExpr() && "Unknown pcrel immediate operand");
    O << *Op.getExpr();
  }
}

void SNESInstPrinter::printMemri(const MCInst *MI, unsigned OpNo,
                                raw_ostream &O) {
  assert(MI->getOperand(OpNo).isReg() && "Expected a register for the first operand");

  const MCOperand &OffsetOp = MI->getOperand(OpNo + 1);

  // Print the register.
  printOperand(MI, OpNo, O);

  // Print the {+,-}offset.
  if (OffsetOp.isImm()) {
    int64_t Offset = OffsetOp.getImm();

    if (Offset >= 0)
      O << '+';

    O << Offset;
  } else if (OffsetOp.isExpr()) {
    O << *OffsetOp.getExpr();
  } else {
    llvm_unreachable("unknown type for offset");
  }
}

} // end of namespace llvm

