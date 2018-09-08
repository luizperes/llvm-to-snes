//===-- SNESInstrInfo.td - SNES Instruction defs -------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the SNES instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

include "SNESInstrFormats.td"

//===----------------------------------------------------------------------===//
// SNES Type Profiles
//===----------------------------------------------------------------------===//

def SDT_SNESCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i16>, SDTCisVT<1, i16>]>;
def SDT_SNESCallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, i16>, SDTCisVT<1, i16>]>;
def SDT_SNESCall : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_SNESWrapper : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;
def SDT_SNESBrcond : SDTypeProfile<0, 2,
                                  [SDTCisVT<0, OtherVT>, SDTCisVT<1, i16>]>;
def SDT_SNESCmp : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_SNESTst : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
def SDT_SNESSelectCC : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>,
                                    SDTCisSameAs<1, 2>, SDTCisVT<3, i16>]>;

//===----------------------------------------------------------------------===//
// SNES Specific Node Definitions
//===----------------------------------------------------------------------===//

def SNESretflag : SDNode<"SNESISD::RET_FLAG", SDTNone,
                        [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def SNESretiflag : SDNode<"SNESISD::RETI_FLAG", SDTNone,
                         [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def SNEScallseq_start : SDNode<"ISD::CALLSEQ_START", SDT_SNESCallSeqStart,
                              [SDNPHasChain, SDNPOutGlue]>;
def SNEScallseq_end : SDNode<"ISD::CALLSEQ_END", SDT_SNESCallSeqEnd,
                            [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def SNEScall : SDNode<"SNESISD::CALL", SDT_SNESCall,
                     [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;

def SNESWrapper : SDNode<"SNESISD::WRAPPER", SDT_SNESWrapper>;

def SNESbrcond : SDNode<"SNESISD::BRCOND", SDT_SNESBrcond,
                       [SDNPHasChain, SDNPInGlue]>;
def SNEScmp : SDNode<"SNESISD::CMP", SDT_SNESCmp, [SDNPOutGlue]>;
def SNEScmpc : SDNode<"SNESISD::CMPC", SDT_SNESCmp, [SDNPInGlue, SDNPOutGlue]>;
def SNEStst : SDNode<"SNESISD::TST", SDT_SNESTst, [SDNPOutGlue]>;
def SNESselectcc: SDNode<"SNESISD::SELECT_CC", SDT_SNESSelectCC, [SDNPInGlue]>;

// Shift nodes.
def SNESlsl : SDNode<"SNESISD::LSL", SDTIntUnaryOp>;
def SNESlsr : SDNode<"SNESISD::LSR", SDTIntUnaryOp>;
def SNESrol : SDNode<"SNESISD::ROL", SDTIntUnaryOp>;
def SNESror : SDNode<"SNESISD::ROR", SDTIntUnaryOp>;
def SNESasr : SDNode<"SNESISD::ASR", SDTIntUnaryOp>;

// Pseudo shift nodes for non-constant shift amounts.
def SNESlslLoop : SDNode<"SNESISD::LSLLOOP", SDTIntShiftOp>;
def SNESlsrLoop : SDNode<"SNESISD::LSRLOOP", SDTIntShiftOp>;
def SNESrolLoop : SDNode<"SNESISD::ROLLOOP", SDTIntShiftOp>;
def SNESrorLoop : SDNode<"SNESISD::RORLOOP", SDTIntShiftOp>;
def SNESasrLoop : SDNode<"SNESISD::ASRLOOP", SDTIntShiftOp>;

//===----------------------------------------------------------------------===//
// SNES Operands, Complex Patterns and Transformations Definitions.
//===----------------------------------------------------------------------===//

def imm8_neg_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(-N->getAPIntValue(), SDLoc(N), MVT::i16);
}]>;

def imm16_neg_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(-N->getAPIntValue(), SDLoc(N), MVT::i16);
}]>;

def imm0_63_neg : PatLeaf<(imm),
[{
  int64_t val = -N->getSExtValue();
  return val >= 0 && val < 64;
}], imm16_neg_XFORM>;

def uimm6 : PatLeaf<(imm), [{ return isUInt<6>(N->getZExtValue()); }]>;

def ioaddr_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(uint8_t(N->getZExtValue()) - 0x20, SDLoc(N), MVT::i16);
}]>;

def iobitpos8_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(Log2_32(uint8_t(N->getZExtValue())),
                                   SDLoc(N), MVT::i16);
}]>;

def iobitposn8_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(Log2_32(uint8_t(~N->getZExtValue())),
                                   SDLoc(N), MVT::i16);
}]>;

def ioaddr8 : PatLeaf<(imm),
[{
  uint64_t val = N->getZExtValue();
  return val >= 0x20 && val < 0x60;
}], ioaddr_XFORM>;

def lowioaddr8 : PatLeaf<(imm),
[{
  uint64_t val = N->getZExtValue();
  return val >= 0x20 && val < 0x40;
}], ioaddr_XFORM>;

def ioaddr16 : PatLeaf<(imm),
[{
  uint64_t val = N->getZExtValue();
  return val >= 0x20 && val < 0x5f;
}], ioaddr_XFORM>;

def iobitpos8 : PatLeaf<(imm),
[{
  return isPowerOf2_32(uint8_t(N->getZExtValue()));
}], iobitpos8_XFORM>;

def iobitposn8 : PatLeaf<(imm),
[{
  return isPowerOf2_32(uint8_t(~N->getZExtValue()));
}], iobitposn8_XFORM>;

def MemriAsmOperand : AsmOperandClass {
  let Name = "Memri";
  let ParserMethod = "parseMemriOperand";
}

/// Address operand for `reg+imm` used by STD and LDD.
def memri : Operand<iPTR>
{
  let MIOperandInfo = (ops MainRegs, i16imm);

  let PrintMethod = "printMemri";
  let EncoderMethod = "encodeMemri";

  let ParserMatchClass = MemriAsmOperand;
}

// Address operand for `SP+imm` used by STD{W}SPQRr
def memspi : Operand<iPTR>
{
  let MIOperandInfo = (ops SP, i16imm);
}

def imm_com8 : Operand<i16>
{
  let EncoderMethod = "encodeComplement";

  let MIOperandInfo = (ops i16imm);
}

def relbrtarget_7 : Operand<OtherVT>
{
    let PrintMethod   = "printPCRelImm";
    let EncoderMethod = "encodeRelCondBrTarget<SNES::fixup_7_pcrel>";
}

def brtarget_13 : Operand<OtherVT>
{
    let PrintMethod   = "printPCRelImm";
    let EncoderMethod = "encodeRelCondBrTarget<SNES::fixup_13_pcrel>";
}

// The target of a 22 or 16-bit call/jmp instruction.
def call_target : Operand<iPTR>
{
    let EncoderMethod = "encodeCallTarget";
}

// A 16-bit address (which can lead to an R_SNES_16 relocation).
def imm16 : Operand<i16>
{
    let EncoderMethod = "encodeImm<SNES::fixup_16, 2>";
}

/// A 6-bit immediate used in the ADIW/SBIW instructions.
def imm_arith6 : Operand<i16>
{
    let EncoderMethod = "encodeImm<SNES::fixup_6_adiw, 0>";
}

/// An 8-bit immediate inside an instruction with the same format
/// as the `LDI` instruction (the `FRdK` format).
def imm_ldi16 : Operand<i16>
{
    let EncoderMethod = "encodeImm<SNES::fixup_ldi, 0>";
}

/// A 5-bit port number used in SBIC and friends (the `FIOBIT` format).
def imm_port5 : Operand<i16>
{
    let EncoderMethod = "encodeImm<SNES::fixup_port5, 0>";
}

/// A 6-bit port number used in the `IN` instruction and friends (the
/// `FIORdA` format.
def imm_port6 : Operand<i16>
{
    let EncoderMethod = "encodeImm<SNES::fixup_port6, 0>";
}

// Addressing mode pattern reg+imm6
def addr : ComplexPattern<iPTR, 2, "SelectAddr", [], [SDNPWantRoot]>;

// AsmOperand class for a pointer register.
// Used with the LD/ST family of instructions.
// See FSTLD in SNESInstrFormats.td
def PtrRegAsmOperand : AsmOperandClass
{
   let Name = "Reg";
}

// A special operand type for the LD/ST instructions.
// It converts the pointer register number into a two-bit field used in the
// instruction.
def LDSTPtrReg : Operand<i16>
{
    let MIOperandInfo = (ops IndexRegs);
    let EncoderMethod = "encodeLDSTPtrReg";

    let ParserMatchClass = PtrRegAsmOperand;
}

// A special operand type for the LDD/STD instructions.
// It behaves identically to the LD/ST version, except restricts
// the pointer registers to Y and Z.
def LDDSTDPtrReg : Operand<i16>
{
    let MIOperandInfo = (ops MainRegs);
    let EncoderMethod = "encodeLDSTPtrReg";

    let ParserMatchClass = PtrRegAsmOperand;
}

// SNES specific condition code. These correspond to SNES_*_COND in
// SNESInstrInfo.td. They must be kept in synch.
def SNES_COND_EQ : PatLeaf<(i16 0)>;
def SNES_COND_NE : PatLeaf<(i16 1)>;
def SNES_COND_GE : PatLeaf<(i16 2)>;
def SNES_COND_LT : PatLeaf<(i16 3)>;
def SNES_COND_SH : PatLeaf<(i16 4)>;
def SNES_COND_LO : PatLeaf<(i16 5)>;
def SNES_COND_MI : PatLeaf<(i16 6)>;
def SNES_COND_PL : PatLeaf<(i16 7)>;

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// SNES Instruction list
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
multiclass Imp <bits<8> opcode, string asm> {
  def "" : SNESImplied<opcode,
                        (outs),
                        (ins),
                        asm,
                        []>;
}

multiclass ImpP <bits<8> opcode, string asm> {
  let Defs = [P] in {
    def "" : SNESImplied<opcode,
                         (outs),
                         (ins),
                         asm,
                         []>;
  }
}
//===----------------------------------------------------------------------===//
// Implied instructions -> <|opcode|>
//===----------------------------------------------------------------------===//
// no op
defm NOP : Imp<0xEA, "NOP">;
// stop processor
defm STP : Imp<0xDB, "STP">;
// wait for interrupt
defm WAI : Imp<0xCB, "WAI">;

// clear carry flag
defm CLC : ImpP<0x18, "CLC">;
// set carry flag
defm SEC : ImpP<0x28, "SEC">;
// clear decimal mode flag
defm CLD : ImpP<0xD8, "CLD">;
// set decimal mode flag
defm SED : ImpP<0xF8, "SED">;
// clear interrupt disable flag
defm CLI : ImpP<0x58, "CLI">;
// set interrupt disable flag
defm SEI : ImpP<0x78, "SEI">;
// clear overflow flag
defm CLV : ImpP<0xB8, "CLV">;
// exchange carry and emulation flags
defm XCE : ImpP<0xFB, "XCE">;

//===----------------------------------------------------------------------===//
// Implied Exchange instruction -> <|opcode|>
//===----------------------------------------------------------------------===//
let Uses = [AL, AH], Defs = [A, P] in {
  // exchange A (AL) and B (AH) register values
  defm XBA : Imp<0xEB, "XBA">;
}

let Uses = [A, X] in {
  // transfer A to X
  defm TAX : ImpP<0xAA, "TAX">;
  // transfer X to A
  defm TXA : ImpP<0x8A, "TXA">;
}

let Uses = [A, Y] in {
  // transfer A to Y
  defm TAY : ImpP<0xA8, "TAY">;
  // transfer Y to A
  defm TYA : ImpP<0x98, "TYA">;
}

let Uses = [A, DP] in {
  // transfer A to DP (direct page)
  defm TCD : ImpP<0x5B, "TCD">;
  // transfer DP (direct page) to A
  defm TDC : ImpP<0x7B, "TDC">;
}

let Uses = [A, SP] in {
  // transfer A to SP (stack pointer)
  defm TCS : Imp<0x1B, "TCS">;
  // transfer SP (stack pointer) to A
  defm TSC : ImpP<0x3B, "TSC">;
}

let Uses = [X, SP] in {
  // transfer X to SP (stack pointer)
  defm TXS : Imp<0x9A, "TXS">;
  // transfer SP (stack pointer) to X
  defm TSX : ImpP<0xBA, "TSX">;
}

let Uses = [X, Y] in {
  // transfer y to x
  defm TYX : ImpP<0xBB, "TYX">;
  // transfer x to y
  defm TXY : ImpP<0x9B, "TXY">;
}

//===----------------------------------------------------------------------===//
// Implied Increment and Decrement -> <|opcode|>
//===----------------------------------------------------------------------===//
let Constraints = "$src = $rd",
Defs = [P] in {
  // decrement A
  def DEA : SNESImplied<0x3A,
                        (outs AccRegs:$rd),
                        (ins AccRegs:$src),
                        "DEA",
                        [(set i16:$rd, (add i16:$src, -1)),
                         (implicit P)]>;

  // increment A
  def INA : SNESImplied<0x1A,
                        (outs AccRegs:$rd),
                        (ins AccRegs:$src),
                        "INA",
                        [(set i16:$rd, (add i16:$src, 1)),
                         (implicit P)]>;
}

// TODO: check when we will need to decrement X and Y
let Uses = [X] in {
  // decrement X
  defm DEX : ImpP<0xCA, "DEX">;
  // increment X
  defm INX : ImpP<0xE8, "INX">;
}

let Uses = [Y] in {
  // decrement Y
  defm DEY : ImpP<0xC8, "DEY">;
  // increment Y
  defm INY : ImpP<0xE8, "INY">;
}

//===----------------------------------------------------------------------===//
// Immediate 8 bit <|opcode|imm8|>
//===----------------------------------------------------------------------===//
let Defs = [P] in {
  // set processor status bits
  def SEP : SNESImm8<0xE2,
                     (outs),
                     (ins i8imm:$k),
                     "SEP\t#$k",
                     []>;

  // reset processor status bits
  def REP : SNESImm8<0xC2,
                     (outs),
                     (ins i8imm:$k),
                     "REP\t#$k",
                     []>;
}

//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// End Instruction list
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

// ADJCALLSTACKDOWN/UP implicitly use/def SP because they may be expanded into
// a stack adjustment and the codegen must know that they may modify the stack
// pointer before prolog-epilog rewriting occurs.
// Pessimistically assume ADJCALLSTACKDOWN / ADJCALLSTACKUP will become
// sub / add which can clobber P.
let Defs = [SP, P],
Uses = [SP] in
{
  def ADJCALLSTACKDOWN : Pseudo<(outs),
                                (ins i16imm:$amt, i16imm:$amt2),
                                "#ADJCALLSTACKDOWN",
                                [(SNEScallseq_start timm:$amt, timm:$amt2)]>;

  // A is used to update SP, since it is a scratch reg and this instruction
  // is placed after the function call then A should be always free.
  //let Defs = [A],
  //Uses = [A] in
  //:TODO: if we enable this, the pseudo is killed because it looks dead
  def ADJCALLSTACKUP : Pseudo<(outs),
                              (ins i16imm:$amt1, i16imm:$amt2),
                              "#ADJCALLSTACKUP",
                              [(SNEScallseq_end timm:$amt1, timm:$amt2)]>;
}

//===----------------------------------------------------------------------===//
// Addition
//===----------------------------------------------------------------------===//
let isCommutable = 1,
Constraints = "$src = $rd",
Defs = [P] in
{
  // ADD Rd, Rr
  // Adds two 8-bit registers.
  def ADDRdRr : FRdRr<0b0000,
                      0b11,
                      (outs MainRegs:$rd),
                      (ins MainRegs:$src, MainRegs:$rr),
                      "add\t$rd, $rr",
                      [(set i16:$rd, (add i16:$src, i16:$rr)),
                       (implicit P)]>;

  // ADDW Rd+1:Rd, Rr+1:Rr
  // Pseudo instruction to add four 8-bit registers as two 16-bit values.
  //
  // Expands to:
  // add Rd,    Rr
  // adc Rd+1, Rr+1
  def ADDWRdRr : Pseudo<(outs MainRegs:$rd),
                        (ins MainRegs:$src, MainRegs:$rr),
                        "addw\t$rd, $rr",
                        [(set i16:$rd, (add i16:$src, i16:$rr)),
                         (implicit P)]>;

  // ADC Rd, Rr
  // Adds two 8-bit registers with carry.
  let Uses = [P] in
  def ADCRdRr : FRdRr<0b0001,
                      0b11,
                      (outs MainRegs:$rd),
                      (ins MainRegs:$src, MainRegs:$rr),
                      "adc\t$rd, $rr",
                      [(set i16:$rd, (adde i16:$src, i16:$rr)),
                       (implicit P)]>;

  // ADCW Rd+1:Rd, Rr+1:Rr
  // Pseudo instruction to add four 8-bit registers as two 16-bit values with
  // carry.
  //
  // Expands to:
  // adc Rd,   Rr
  // adc Rd+1, Rr+1
  let Uses = [P] in
  def ADCWRdRr : Pseudo<(outs MainRegs:$rd),
                        (ins MainRegs:$src, MainRegs:$rr),
                        "adcw\t$rd, $rr",
                        [(set i16:$rd, (adde i16:$src, i16:$rr)),
                         (implicit P)]>;

  // AIDW Rd, k
  // Adds an immediate 6-bit value K to Rd, placing the result in Rd.
  def ADIWRdK : FWRdK<0b0,
                      (outs MainRegs:$rd),
                      (ins MainRegs:$src, imm_arith6:$k),
                      "adiw\t$rd, $k",
                      [(set i16:$rd, (add i16:$src, uimm6:$k)),
                       (implicit P)]>,
                Requires<[/*TODO: check it: HasADDSUBIW*/]>;
}

//===----------------------------------------------------------------------===//
// Subtraction
//===----------------------------------------------------------------------===//
let Constraints = "$src = $rd",
Defs = [P] in
{
  // SUB Rd, Rr
  // Subtracts the 8-bit value of Rr from Rd and places the value in Rd.
  def SUBRdRr : FRdRr<0b0001,
                      0b10,
                      (outs MainRegs:$rd),
                      (ins MainRegs:$src, MainRegs:$rr),
                      "sub\t$rd, $rr",
                      [(set i16:$rd, (sub i16:$src, i16:$rr)),
                       (implicit P)]>;

  // SUBW Rd+1:Rd, Rr+1:Rr
  // Subtracts two 16-bit values and places the result into Rd.
  //
  // Expands to:
  // sub Rd,   Rr
  // sbc Rd+1, Rr+1
  def SUBWRdRr : Pseudo<(outs MainRegs:$rd),
                        (ins MainRegs:$src, MainRegs:$rr),
                        "subw\t$rd, $rr",
                        [(set i16:$rd, (sub i16:$src, i16:$rr)),
                         (implicit P)]>;

  def SUBIRdK : FRdK<0b0101,
                     (outs MainRegs:$rd),
                     (ins MainRegs:$src, imm_ldi16:$k),
                     "subi\t$rd, $k",
                     [(set i16:$rd, (sub i16:$src, imm:$k)),
                      (implicit P)]>;

  // SUBIW Rd+1:Rd, K+1:K
  //
  // Expands to:
  // subi Rd,   K
  // sbci Rd+1, K+1
  def SUBIWRdK : Pseudo<(outs MainRegs:$rd),
                        (ins MainRegs:$src, i16imm:$rr),
                        "subiw\t$rd, $rr",
                        [(set i16:$rd, (sub i16:$src, imm:$rr)),
                         (implicit P)]>;

  def SBIWRdK : FWRdK<0b1,
                      (outs MainRegs:$rd),
                      (ins MainRegs:$src, imm_arith6:$k),
                      "sbiw\t$rd, $k",
                      [(set i16:$rd, (sub i16:$src, uimm6:$k)),
                       (implicit P)]>,
                Requires<[/* TODO: check it: HasADDSUBIW */]>;

  // Subtract with carry operations which must read the carry flag in P.
  let Uses = [P] in
  {
    def SBCRdRr : FRdRr<0b0000,
                        0b10,
                        (outs MainRegs:$rd),
                        (ins MainRegs:$src, MainRegs:$rr),
                        "sbc\t$rd, $rr",
                        [(set i16:$rd, (sube i16:$src, i16:$rr)),
                         (implicit P)]>;

    // SBCW Rd+1:Rd, Rr+1:Rr
    //
    // Expands to:
    // sbc Rd,   Rr
    // sbc Rd+1, Rr+1
    def SBCWRdRr : Pseudo<(outs MainRegs:$rd),
                          (ins MainRegs:$src, MainRegs:$rr),
                          "sbcw\t$rd, $rr",
                          [(set i16:$rd, (sube i16:$src, i16:$rr)),
                           (implicit P)]>;

    def SBCIRdK : FRdK<0b0100,
                       (outs MainRegs:$rd),
                       (ins MainRegs:$src, imm_ldi16:$k),
                       "sbci\t$rd, $k",
                       [(set i16:$rd, (sube i16:$src, imm:$k)),
                        (implicit P)]>;

    // SBCIW Rd+1:Rd, K+1:K
    // sbci Rd,   K
    // sbci Rd+1, K+1
    def SBCIWRdK : Pseudo<(outs MainRegs:$rd),
                          (ins MainRegs:$src, i16imm:$rr),
                          "sbciw\t$rd, $rr",
                          [(set i16:$rd, (sube i16:$src, imm:$rr)),
                           (implicit P)]>;
  }
}

//===----------------------------------------------------------------------===//
// Multiplication
//===----------------------------------------------------------------------===//

let isCommutable = 1,
Defs = [A, P] in
{
  // MUL Rd, Rr
  // Multiplies Rd by Rr and places the result into R1:A.
  let usesCustomInserter = 1 in {
    def MULRdRr : FRdRr<0b1001, 0b11,
                        (outs),
                        (ins MainRegs:$lhs, MainRegs:$rhs),
                        "mul\t$lhs, $rhs",
                        [/*(set R1, A, (smullohi i16:$lhs, i16:$rhs))*/]>,
                    Requires<[/* TODO: check it: SupportsMultiplication */]>;

    def MULSRdRr : FMUL2RdRr<0,
                             (outs),
                             (ins MainRegs:$lhs, MainRegs:$rhs),
                             "muls\t$lhs, $rhs",
                             []>,
                   Requires<[/* TODO: check it: SupportsMultiplication */]>;
  }

  def MULSURdRr : FMUL2RdRr<1,
                            (outs),
                            (ins MainRegs:$lhs, MainRegs:$rhs),
                            "mulsu\t$lhs, $rhs",
                            []>,
                  Requires<[/* TODO: check it: SupportsMultiplication */]>;

  def FMUL : FFMULRdRr<0b01,
                       (outs),
                       (ins MainRegs:$lhs, MainRegs:$rhs),
                       "fmul\t$lhs, $rhs",
                       []>,
             Requires<[/* TODO: check it: SupportsMultiplication */]>;

  def FMULS : FFMULRdRr<0b10,
                        (outs),
                        (ins MainRegs:$lhs, MainRegs:$rhs),
                        "fmuls\t$lhs, $rhs",
                        []>,
              Requires<[/* SupportsMultiplication */]>;

  def FMULSU : FFMULRdRr<0b11,
                         (outs),
                         (ins MainRegs:$lhs, MainRegs:$rhs),
                         "fmulsu\t$lhs, $rhs",
                         []>,
               Requires<[/* TODO: check it: SupportsMultiplication */]>;
}

let Defs = [A] in
def DESK : FDES<(outs),
                (ins i16imm:$k),
                "des\t$k",
                []>,
           Requires<[/* TODO: check it: HasDES */]>;

//===----------------------------------------------------------------------===//
// Logic
//===----------------------------------------------------------------------===//
let Constraints = "$src = $rd",
Defs = [P] in
{
  // Register-Register logic instructions (which have the
  // property of commutativity).
  let isCommutable = 1 in
  {
    def ANDRdRr : FRdRr<0b0010,
                        0b00,
                        (outs MainRegs:$rd),
                        (ins MainRegs:$src, MainRegs:$rr),
                        "and\t$rd, $rr",
                        [(set i16:$rd, (and i16:$src, i16:$rr)),
                         (implicit P)]>;

    // ANDW Rd+1:Rd, Rr+1:Rr
    //
    // Expands to:
    // and Rd,   Rr
    // and Rd+1, Rr+1
    def ANDWRdRr : Pseudo<(outs MainRegs:$rd),
                          (ins MainRegs:$src, MainRegs:$rr),
                          "andw\t$rd, $rr",
                          [(set i16:$rd, (and i16:$src, i16:$rr)),
                           (implicit P)]>;

    def ORRdRr : FRdRr<0b0010,
                       0b10,
                       (outs MainRegs:$rd),
                       (ins MainRegs:$src, MainRegs:$rr),
                       "or\t$rd, $rr",
                       [(set i16:$rd, (or i16:$src, i16:$rr)),
                        (implicit P)]>;

    // ORW Rd+1:Rd, Rr+1:Rr
    //
    // Expands to:
    // or Rd,   Rr
    // or Rd+1, Rr+1
    def ORWRdRr : Pseudo<(outs MainRegs:$rd),
                         (ins MainRegs:$src, MainRegs:$rr),
                         "orw\t$rd, $rr",
                         [(set i16:$rd, (or i16:$src, i16:$rr)),
                          (implicit P)]>;

    def EORRdRr : FRdRr<0b0010,
                        0b01,
                        (outs MainRegs:$rd),
                        (ins MainRegs:$src, MainRegs:$rr),
                        "eor\t$rd, $rr",
                        [(set i16:$rd, (xor i16:$src, i16:$rr)),
                         (implicit P)]>;

    // EORW Rd+1:Rd, Rr+1:Rr
    //
    // Expands to:
    // eor Rd,   Rr
    // eor Rd+1, Rr+1
    def EORWRdRr : Pseudo<(outs MainRegs:$rd),
                          (ins MainRegs:$src, MainRegs:$rr),
                          "eorw\t$rd, $rr",
                          [(set i16:$rd, (xor i16:$src, i16:$rr)),
                           (implicit P)]>;
  }

  def ANDIRdK : FRdK<0b0111,
                     (outs MainRegs:$rd),
                     (ins MainRegs:$src, imm_ldi16:$k),
                     "andi\t$rd, $k",
                     [(set i16:$rd, (and i16:$src, imm:$k)),
                      (implicit P)]>;

  // ANDI Rd+1:Rd, K+1:K
  //
  // Expands to:
  // andi Rd,   K
  // andi Rd+1, K+1
  def ANDIWRdK : Pseudo<(outs MainRegs:$rd),
                        (ins MainRegs:$src, i16imm:$k),
                        "andiw\t$rd, $k",
                        [(set i16:$rd, (and i16:$src, imm:$k)),
                         (implicit P)]>;

  def ORIRdK : FRdK<0b0110,
                    (outs MainRegs:$rd),
                    (ins MainRegs:$src, imm_ldi16:$k),
                    "ori\t$rd, $k",
                    [(set i16:$rd, (or i16:$src, imm:$k)),
                     (implicit P)]>;

  // ORIW Rd+1:Rd, K+1,K
  //
  // Expands to:
  // ori Rd,   K
  // ori Rd+1, K+1
  def ORIWRdK : Pseudo<(outs MainRegs:$rd),
                       (ins MainRegs:$src, i16imm:$rr),
                       "oriw\t$rd, $rr",
                       [(set i16:$rd, (or i16:$src, imm:$rr)),
                        (implicit P)]>;
}

//===----------------------------------------------------------------------===//
// One's/Two's Complement
//===----------------------------------------------------------------------===//
let Constraints = "$src = $rd",
Defs = [P] in
{
  def COMRd : FRd<0b1001,
                  0b0100000,
                  (outs MainRegs:$rd),
                  (ins MainRegs:$src),
                  "com\t$rd",
                  [(set i16:$rd, (not i16:$src)), (implicit P)]>;

  // COMW Rd+1:Rd
  //
  // Expands to:
  // com Rd
  // com Rd+1
  def COMWRd : Pseudo<(outs MainRegs:$rd),
                      (ins MainRegs:$src),
                      "comw\t$rd",
                      [(set i16:$rd, (not i16:$src)), (implicit P)]>;

  //:TODO: optimize NEG for wider types
  def NEGRd : FRd<0b1001,
                  0b0100001,
                  (outs MainRegs:$rd),
                  (ins MainRegs:$src),
                  "neg\t$rd",
                  [(set i16:$rd, (ineg i16:$src)), (implicit P)]>;
}

// TST Rd
// Test for zero of minus.
// This operation is identical to a `Rd AND Rd`.
//def : InstAlias<"tst\t$rd", (ANDRdRr MainRegs:$rd, MainRegs:$rd), 1>;

let Defs = [P] in
def TSTRd : FTST<0b0010,
                  0b00,
                  (outs),
                  (ins MainRegs:$rd),
                  "tst\t$rd",
                  [(SNEStst i16:$rd)]>;

//===----------------------------------------------------------------------===//
// Jump instructions
//===----------------------------------------------------------------------===//
let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
{
  def RJMPk : FBRk<0,
                   (outs),
                   (ins brtarget_13:$target),
                   "rjmp\t$target",
                   [(br bb:$target)]>;

  let isIndirectBranch = 1,
  Uses = [A] in
  def IJMP : F16<0b1001010000001001,
                 (outs),
                 (ins),
                 "ijmp",
                 []>,
             Requires<[/*TODO: check it: HasIJMPCALL */]>;

  let isIndirectBranch = 1,
  Uses = [A] in
  def EIJMP : F16<0b1001010000011001,
                  (outs),
                  (ins),
                  "eijmp",
                  []>,
              Requires<[/*TODO: check it: HasEIJMPCALL */]>;

  def JMPk : F32BRk<0b110,
                    (outs),
                    (ins call_target:$k),
                    "jmp\t$k",
                    []>,
             Requires<[/*TODO: check it: HasJMPCALL */]>;
}

//===----------------------------------------------------------------------===//
// Call instructions
//===----------------------------------------------------------------------===//
let isCall = 1 in
{
  // SP is marked as a use to prevent stack-pointer assignments that appear
  // immediately before calls from potentially appearing dead.
  let Uses = [SP] in
  def RCALLk : FBRk<1,
                    (outs),
                    (ins brtarget_13:$target),
                    "rcall\t$target",
                    []>;

  // SP is marked as a use to prevent stack-pointer assignments that appear
  // immediately before calls from potentially appearing dead.
  let Uses = [SP, A] in
  def ICALL : F16<0b1001010100001001,
                  (outs),
                  (ins variable_ops),
                  "icall",
                  []>,
              Requires<[/*TODO: check it: HasIJMPCALL */]>;

  // SP is marked as a use to prevent stack-pointer assignments that appear
  // immediately before calls from potentially appearing dead.
  let Uses = [SP, A] in
  def EICALL : F16<0b1001010100011001,
                   (outs),
                   (ins variable_ops),
                   "eicall",
                   []>,
               Requires<[/*TODO: check it: HasEIJMPCALL */]>;

  // SP is marked as a use to prevent stack-pointer assignments that appear
  // immediately before calls from potentially appearing dead.
  //
  //:TODO: the imm field can be either 16 or 22 bits in devices with more
  // than 64k of ROM, fix it once we support the largest devices.
  let Uses = [SP] in
  def CALLk : F32BRk<0b111,
                     (outs),
                     (ins call_target:$k),
                     "call\t$k",
                     [(SNEScall imm:$k)]>,
              Requires<[/*TODO: check it: HasJMPCALL */]>;
}

//===----------------------------------------------------------------------===//
// Return instructions.
//===----------------------------------------------------------------------===//
let isTerminator = 1,
isReturn = 1,
isBarrier = 1 in 
{
  def RET : F16<0b1001010100001000,
                (outs),
                (ins),
                "ret",
                [(SNESretflag)]>;

  def RETI : F16<0b1001010100011000,
                 (outs),
                 (ins),
                 "reti",
                 [(SNESretiflag)]>;
}

//===----------------------------------------------------------------------===//
// Compare operations.
//===----------------------------------------------------------------------===//
let Defs = [P] in
{
  // CPSE Rd, Rr
  // Compare Rd and Rr, skipping the next instruction if they are equal.
  let isBarrier = 1,
  isBranch = 1,
  isTerminator = 1 in
  def CPSE : FRdRr<0b0001,
                   0b00,
                   (outs),
                   (ins MainRegs:$rd, MainRegs:$rr),
                   "cpse\t$rd, $rr",
                   []>;

  def CPRdRr : FRdRr<0b0001,
                     0b01,
                     (outs),
                     (ins MainRegs:$rd, MainRegs:$rr),
                     "cp\t$rd, $rr",
                     [(SNEScmp i16:$rd, i16:$rr), (implicit P)]>;

  // CPW Rd+1:Rd, Rr+1:Rr
  //
  // Expands to:
  // cp  Rd,   Rr
  // cpc Rd+1, Rr+1
  def CPWRdRr : Pseudo<(outs),
                       (ins MainRegs:$src, MainRegs:$src2),
                       "cpw\t$src, $src2",
                       [(SNEScmp i16:$src, i16:$src2), (implicit P)]>;

  let Uses = [P] in
  def CPCRdRr : FRdRr<0b0000,
                      0b01,
                      (outs),
                      (ins MainRegs:$rd, MainRegs:$rr),
                      "cpc\t$rd, $rr",
                      [(SNEScmpc i16:$rd, i16:$rr), (implicit P)]>;

  // CPCW Rd+1:Rd. Rr+1:Rr
  //
  // Expands to:
  // cpc Rd,   Rr
  // cpc Rd+1, Rr+1
  let Uses = [P] in
  def CPCWRdRr : Pseudo<(outs),
                        (ins MainRegs:$src, MainRegs:$src2),
                        "cpcw\t$src, $src2",
                        [(SNEScmpc i16:$src, i16:$src2), (implicit P)]>;

  // CPI Rd, K
  // Compares a register with an 8 bit immediate.
  def CPIRdK : FRdK<0b0011,
                    (outs),
                    (ins MainRegs:$rd, imm_ldi16:$k),
                    "cpi\t$rd, $k",
                    [(SNEScmp i16:$rd, imm:$k), (implicit P)]>;
}

//===----------------------------------------------------------------------===//
// Register conditional skipping/branching operations.
//===----------------------------------------------------------------------===//
let isBranch = 1,
isTerminator = 1 in
{
  // Conditional skipping on GPR register bits, and
  // conditional skipping on IO register bits.
  let isBarrier = 1 in
  {
    def SBRCRrB : FRdB<0b10,
                       (outs),
                       (ins MainRegs:$rr, i16imm:$b),
                       "sbrc\t$rr, $b",
                       []>;

    def SBRSRrB : FRdB<0b11,
                       (outs),
                       (ins MainRegs:$rr, i16imm:$b),
                       "sbrs\t$rr, $b",
                       []>;

    def SBICAb : FIOBIT<0b01,
                        (outs),
                        (ins imm_port5:$a, i16imm:$b),
                        "sbic\t$a, $b",
                        []>;

    def SBISAb : FIOBIT<0b11,
                        (outs),
                        (ins imm_port5:$a, i16imm:$b),
                        "sbis\t$a, $b",
                        []>;
  }

  // Relative branches on status flag bits.
  let Uses = [P] in
  {
    // BRBS s, k
    // Branch if `s` flag in status register is set.
    def BRBSsk : FSK<0,
                     (outs),
                     (ins i16imm:$s, relbrtarget_7:$k),
                     "brbs\t$s, $k",
                     []>;

    // BRBC s, k
    // Branch if `s` flag in status register is clear.
    def BRBCsk : FSK<1,
                     (outs),
                     (ins i16imm:$s, relbrtarget_7:$k),
                     "brbc\t$s, $k",
                     []>;
  }
}


// BRCS k
// Branch if carry flag is set
def : InstAlias<"brcs\t$k", (BRBSsk 0, relbrtarget_7:$k)>;

// BRCC k
// Branch if carry flag is clear
def : InstAlias<"brcc\t$k", (BRBCsk 0, relbrtarget_7:$k)>;

// BRHS k
// Branch if half carry flag is set
def : InstAlias<"brhs\t$k", (BRBSsk 5, relbrtarget_7:$k)>;

// BRHC k
// Branch if half carry flag is clear
def : InstAlias<"brhc\t$k", (BRBCsk 5, relbrtarget_7:$k)>;

// BRTS k
// Branch if the T flag is set
def : InstAlias<"brts\t$k", (BRBSsk 6, relbrtarget_7:$k)>;

// BRTC k
// Branch if the T flag is clear
def : InstAlias<"brtc\t$k", (BRBCsk 6, relbrtarget_7:$k)>;

// BRVS k
// Branch if the overflow flag is set
def : InstAlias<"brvs\t$k", (BRBSsk 3, relbrtarget_7:$k)>;

// BRVC k
// Branch if the overflow flag is clear
def : InstAlias<"brvc\t$k", (BRBCsk 3, relbrtarget_7:$k)>;

// BRIE k
// Branch if the global interrupt flag is enabled
def : InstAlias<"brie\t$k", (BRBSsk 7, relbrtarget_7:$k)>;

// BRID k
// Branch if the global interrupt flag is disabled
def : InstAlias<"brid\t$k", (BRBCsk 7, relbrtarget_7:$k)>;

//===----------------------------------------------------------------------===//
// PC-relative conditional branches
//===----------------------------------------------------------------------===//
// Based on status register. We cannot simplify these into instruction aliases
// because we also need to be able to specify a pattern to match for ISel.
let isBranch = 1,
isTerminator = 1,
Uses = [P] in
{
  def BREQk : FBRsk<0,
                    0b001,
                    (outs),
                    (ins relbrtarget_7:$target),
                    "breq\t$target",
                    [(SNESbrcond bb:$target, SNES_COND_EQ)]>;

  def BRNEk : FBRsk<1,
                    0b001,
                    (outs),
                    (ins relbrtarget_7:$target),
                    "brne\t$target",
                    [(SNESbrcond bb:$target, SNES_COND_NE)]>;


  def BRSHk : FBRsk<1,
                    0b000,
                    (outs),
                    (ins relbrtarget_7:$target),
                    "brsh\t$target",
                    [(SNESbrcond bb:$target, SNES_COND_SH)]>;

  def BRLOk : FBRsk<0,
                    0b000,
                    (outs),
                    (ins relbrtarget_7:$target),
                    "brlo\t$target",
                    [(SNESbrcond bb:$target, SNES_COND_LO)]>;

  def BRMIk : FBRsk<0,
                    0b010,
                    (outs),
                    (ins relbrtarget_7:$target),
                    "brmi\t$target",
                    [(SNESbrcond bb:$target, SNES_COND_MI)]>;

  def BRPLk : FBRsk<1,
                    0b010,
                    (outs),
                    (ins relbrtarget_7:$target),
                    "brpl\t$target",
                    [(SNESbrcond bb:$target, SNES_COND_PL)]>;

  def BRGEk : FBRsk<1,
                    0b100,
                    (outs),
                    (ins relbrtarget_7:$target),
                    "brge\t$target",
                    [(SNESbrcond bb:$target, SNES_COND_GE)]>;

  def BRLTk : FBRsk<0,
                    0b100,
                    (outs),
                    (ins relbrtarget_7:$target),
                    "brlt\t$target",
                    [(SNESbrcond bb:$target, SNES_COND_LT)]>;
}

//===----------------------------------------------------------------------===//
// Data transfer instructions
//===----------------------------------------------------------------------===//
// 8 and 16-bit register move instructions.
let hasSideEffects = 0 in
{
  def MOVRdRr : FRdRr<0b0010,
                      0b11,
                      (outs MainRegs:$rd),
                      (ins MainRegs:$rr),
                      "mov\t$rd, $rr",
                      []>;

  def MOVWRdRr : FMOVWRdRr<(outs MainRegs:$dst),
                           (ins MainRegs:$src),
                           "movw\t$dst, $src",
                           []>,
                 Requires<[/*TODO: check it: HasMOVW */]>;
}

// Load immediate values into registers.
let isReMaterializable = 1 in
{
  def LDIRdK : FRdK<0b1110,
                    (outs MainRegs:$rd),
                    (ins imm_ldi16:$k),
                    "ldi\t$rd, $k",
                    [(set i16:$rd, imm:$k)]>;

  // LDIW Rd+1:Rd, K+1:K
  //
  // Expands to:
  // ldi Rd,   K
  // ldi Rd+1, K+1
  def LDIWRdK : Pseudo<(outs MainRegs:$dst),
                       (ins i16imm:$src),
                       "ldiw\t$dst, $src",
                       [(set i16:$dst, imm:$src)]>;
}

// Load from data space into register.
let canFoldAsLoad = 1,
isReMaterializable = 1 in
{
  def LDSRdK : F32DM<0b0,
                     (outs MainRegs:$rd),
                     (ins imm16:$k),
                     "lds\t$rd, $k",
                     [(set i16:$rd, (load imm:$k))]>,
               Requires<[/*TODO: check it: HasSRAM */]>;

  // LDSW Rd+1:Rd, K+1:K
  //
  // Expands to:
  // lds Rd,  (K+1:K)
  // lds Rd+1 (K+1:K) + 1
  def LDSWRdK : Pseudo<(outs MainRegs:$dst),
                       (ins i16imm:$src),
                       "ldsw\t$dst, $src",
                       [(set i16:$dst, (load imm:$src))]>,
                Requires<[/*TODO: check it: HasSRAM */]>;
}

// Indirect loads.
let canFoldAsLoad = 1,
isReMaterializable = 1 in
{
  def LDRdPtr : FSTLD<0,
                      0b00,
                      (outs MainRegs:$reg),
                      (ins LDSTPtrReg:$ptrreg),
                      "ld\t$reg, $ptrreg",
                      [(set MainRegs:$reg, (load i16:$ptrreg))]>,
                Requires<[/*TODO: check it: HasSRAM */]>;

  // LDW Rd+1:Rd, P
  //
  // Expands to:
  // ld Rd,   P+
  // ld Rd+1, P+
  let Constraints = "@earlyclobber $reg" in
  def LDWRdPtr : Pseudo<(outs MainRegs:$reg),
                        (ins MainRegs:$ptrreg),
                        "ldw\t$reg, $ptrreg",
                        [(set i16:$reg, (load i16:$ptrreg))]>,
                 Requires<[/*TODO: check it: HasSRAM */]>;
}

// Indirect loads (with postincrement or predecrement).
let mayLoad = 1,
hasSideEffects = 0,
Constraints = "$ptrreg = $base_wb,@earlyclobber $reg,@earlyclobber $base_wb" in
{
  def LDRdPtrPi : FSTLD<0,
                        0b01,
                        (outs MainRegs:$reg, IndexRegs:$base_wb),
                        (ins LDSTPtrReg:$ptrreg),
                        "ld\t$reg, $ptrreg+",
                        []>,
                  Requires<[/*TODO: check it: HasSRAM */]>;

  // LDW Rd+1:Rd, P+
  // Expands to:
  // ld Rd,   P+
  // ld Rd+1, P+
  def LDWRdPtrPi : Pseudo<(outs MainRegs:$reg, IndexRegs:$base_wb),
                          (ins IndexRegs:$ptrreg),
                          "ldw\t$reg, $ptrreg+",
                          []>,
                   Requires<[/*TODO: check it: HasSRAM */]>;

  def LDRdPtrPd : FSTLD<0,
                        0b10,
                        (outs MainRegs:$reg, IndexRegs:$base_wb),
                        (ins LDSTPtrReg:$ptrreg),
                        "ld\t$reg, -$ptrreg",
                        []>,
                  Requires<[/*TODO: check it: HasSRAM */]>;

  // LDW Rd+1:Rd, -P
  //
  // Expands to:
  // ld Rd+1, -P
  // ld Rd,   -P
  def LDWRdPtrPd : Pseudo<(outs MainRegs:$reg, IndexRegs:$base_wb),
                          (ins IndexRegs:$ptrreg),
                          "ldw\t$reg, -$ptrreg",
                          []>,
                   Requires<[/*TODO: check it: HasSRAM */]>;
}

// Load indirect with displacement operations.
let canFoldAsLoad = 1,
isReMaterializable = 1 in
{
  let Constraints = "@earlyclobber $reg" in
  def LDDRdPtrQ : FSTDLDD<0,
                          (outs MainRegs:$reg),
                          (ins memri:$memri),
                          "ldd\t$reg, $memri",
                          [(set i16:$reg, (load addr:$memri))]>,
                  Requires<[/*TODO: check it: HasSRAM */]>;

  // LDDW Rd+1:Rd, P+q
  //
  // Expands to:
  // ldd Rd,   P+q
  // ldd Rd+1, P+q+1
  let Constraints = "@earlyclobber $dst" in
  def LDDWRdPtrQ : Pseudo<(outs MainRegs:$dst),
                          (ins memri:$memri),
                          "lddw\t$dst, $memri",
                          [(set i16:$dst, (load addr:$memri))]>,
                   Requires<[/*TODO: check it: HasSRAM */]>;

  let mayLoad = 1,
  hasSideEffects = 0,
  Constraints = "@earlyclobber $dst" in
  def LDDWRdYQ : Pseudo<(outs MainRegs:$dst),
                        (ins memri:$memri),
                        "lddw\t$dst, $memri",
                        []>,
                 Requires<[/*TODO: check it: HasSRAM */]>;
}

class AtomicLoad<PatFrag Op, RegisterClass DRC> :
  Pseudo<(outs DRC:$rd), (ins IndexRegs:$rr), "atomic_op",
         [(set DRC:$rd, (Op i16:$rr))]>;

class AtomicStore<PatFrag Op, RegisterClass DRC> :
  Pseudo<(outs), (ins MainRegs:$rd, DRC:$rr), "atomic_op",
         [(Op i16:$rd, DRC:$rr)]>;

class AtomicLoadOp<PatFrag Op, RegisterClass DRC> :
  Pseudo<(outs DRC:$rd), (ins IndexRegs:$rr, DRC:$operand),
         "atomic_op",
         [(set DRC:$rd, (Op i16:$rr, DRC:$operand))]>;

def AtomicLoad8   : AtomicLoad<atomic_load_8, MainRegs>;
def AtomicLoad16  : AtomicLoad<atomic_load_16, MainRegs>;

def AtomicStore8  : AtomicStore<atomic_store_8, MainRegs>;
def AtomicStore16 : AtomicStore<atomic_store_16, MainRegs>;

def AtomicLoadAdd8  : AtomicLoadOp<atomic_load_add_8, MainRegs>;
def AtomicLoadAdd16 : AtomicLoadOp<atomic_load_add_16, MainRegs>;
def AtomicLoadSub8  : AtomicLoadOp<atomic_load_sub_8, MainRegs>;
def AtomicLoadSub16 : AtomicLoadOp<atomic_load_sub_16, MainRegs>;
def AtomicLoadAnd8  : AtomicLoadOp<atomic_load_and_8, MainRegs>;
def AtomicLoadAnd16 : AtomicLoadOp<atomic_load_and_16, MainRegs>;
def AtomicLoadOr8   : AtomicLoadOp<atomic_load_or_8, MainRegs>;
def AtomicLoadOr16  : AtomicLoadOp<atomic_load_or_16, MainRegs>;
def AtomicLoadXor8  : AtomicLoadOp<atomic_load_xor_8, MainRegs>;
def AtomicLoadXor16 : AtomicLoadOp<atomic_load_xor_16, MainRegs>;
def AtomicFence     : Pseudo<(outs), (ins), "atomic_fence",
                             [(atomic_fence imm, imm)]>;

// Indirect store from register to data space.
def STSKRr : F32DM<0b1,
                   (outs),
                   (ins imm16:$k, MainRegs:$rd),
                   "sts\t$k, $rd",
                   [(store i16:$rd, imm:$k)]>,
             Requires<[/*TODO: check it: HasSRAM */]>;

// STSW K+1:K, Rr+1:Rr
//
// Expands to:
// sts Rr+1, (K+1:K) + 1
// sts Rr,   (K+1:K)
def STSWKRr : Pseudo<(outs),
                     (ins i16imm:$dst, MainRegs:$src),
                     "stsw\t$dst, $src",
                     [(store i16:$src, imm:$dst)]>,
              Requires<[/*TODO: check it: HasSRAM */]>;

// Indirect stores.
// ST P, Rr
// Stores the value of Rr into the location addressed by pointer P.
def STPtrRr : FSTLD<1,
                    0b00,
                    (outs),
                    (ins LDSTPtrReg:$ptrreg, MainRegs:$reg),
                    "st\t$ptrreg, $reg",
                    [(store MainRegs:$reg, i16:$ptrreg)]>,
              Requires<[/*TODO: check it: HasSRAM */]>;

// STW P, Rr+1:Rr
// Stores the value of Rr into the location addressed by pointer P.
//
// Expands to:
// st P, Rr
// std P+1, Rr+1
def STWPtrRr : Pseudo<(outs),
                      (ins MainRegs:$ptrreg, MainRegs:$reg),
                      "stw\t$ptrreg, $reg",
                      [(store i16:$reg, i16:$ptrreg)]>,
               Requires<[/*TODO: check it: HasSRAM */]>;

// Indirect stores (with postincrement or predecrement).
let Constraints = "$ptrreg = $base_wb,@earlyclobber $base_wb" in
{

  // ST P+, Rr
  // Stores the value of Rr into the location addressed by pointer P.
  // Post increments P.
  def STPtrPiRr : FSTLD<1,
                        0b01,
                        (outs LDSTPtrReg:$base_wb),
                        (ins LDSTPtrReg:$ptrreg, MainRegs:$reg, i16imm:$offs),
                        "st\t$ptrreg+, $reg",
                        [(set i16:$base_wb,
                         (post_store MainRegs:$reg, i16:$ptrreg, imm:$offs))]>,
                  Requires<[/*TODO: check it: HasSRAM */]>;

  // STW P+, Rr+1:Rr
  // Stores the value of Rr into the location addressed by pointer P.
  // Post increments P.
  //
  // Expands to:
  // st P+, Rr
  // st P+, Rr+1
  def STWPtrPiRr : Pseudo<(outs IndexRegs:$base_wb),
                          (ins IndexRegs:$ptrreg, MainRegs:$trh, i16imm:$offs),
                          "stw\t$ptrreg+, $trh",
                          [(set IndexRegs:$base_wb,
                           (post_store MainRegs:$trh, IndexRegs:$ptrreg, imm:$offs))]>,
                   Requires<[/*TODO: check it: HasSRAM */]>;

  // ST -P, Rr
  // Stores the value of Rr into the location addressed by pointer P.
  // Pre decrements P.
  def STPtrPdRr : FSTLD<1,
                        0b10,
                        (outs LDSTPtrReg:$base_wb),
                        (ins LDSTPtrReg:$ptrreg, MainRegs:$reg, i16imm:$offs),
                        "st\t-$ptrreg, $reg",
                        [(set i16:$base_wb,
                         (pre_store MainRegs:$reg, i16:$ptrreg, imm:$offs))]>,
                  Requires<[/*TODO: check it: HasSRAM */]>;

  // STW -P, Rr+1:Rr
  // Stores the value of Rr into the location addressed by pointer P.
  // Pre decrements P.
  //
  // Expands to:
  // st -P, Rr+1
  // st -P, Rr
  def STWPtrPdRr : Pseudo<(outs IndexRegs:$base_wb),
                          (ins IndexRegs:$ptrreg, MainRegs:$reg, i16imm:$offs),
                          "stw\t-$ptrreg, $reg",
                          [(set IndexRegs:$base_wb,
                           (pre_store i16:$reg, i16:$ptrreg, imm:$offs))]>,
                   Requires<[/*TODO: check it: HasSRAM */]>;
}

// Store indirect with displacement operations.
// STD P+q, Rr
// Stores the value of Rr into the location addressed by pointer P with a
// displacement of q. Does not modify P.
def STDPtrQRr : FSTDLDD<1,
                        (outs),
                        (ins memri:$memri, MainRegs:$reg),
                        "std\t$memri, $reg",
                        [(store i16:$reg, addr:$memri)]>,
                Requires<[/*TODO: check it: HasSRAM */]>;

// STDW P+q, Rr+1:Rr
// Stores the value of Rr into the location addressed by pointer P with a
// displacement of q. Does not modify P.
//
// Expands to:
// std P+q,   Rr
// std P+q+1, Rr+1
def STDWPtrQRr : Pseudo<(outs),
                        (ins memri:$memri, MainRegs:$src),
                        "stdw\t$memri, $src",
                        [(store i16:$src, addr:$memri)]>,
                 Requires<[/*TODO: check it: HasSRAM */]>;


// Load program memory operations.
let canFoldAsLoad = 1,
isReMaterializable = 1,
hasSideEffects = 0 in
{
  let Defs = [A],
      Uses = [A] in
  def LPM : F16<0b1001010111001000,
                (outs),
                (ins),
                "lpm",
                []>,
            Requires<[/*TODO: check it: HasLPM */]>;

  def LPMRdZ : FLPMX<0,
                     0,
                     (outs MainRegs:$dst),
                     (ins IndexRegs:$z),
                     "lpm\t$dst, $z",
                     []>,
               Requires<[/*TODO: check it: HasLPMX */]>;

  // Load program memory, while postincrementing the Z register.
  let mayLoad = 1,
  Defs = [A] in
  {
    def LPMRdZPi : FLPMX<0,
                         1,
                         (outs MainRegs:$dst),
                         (ins IndexRegs:$z),
                         "lpm\t$dst, $z+",
                         []>,
                   Requires<[/*TODO: check it: HasLPMX */]>;

    def LPMWRdZ : Pseudo<(outs MainRegs:$dst),
                         (ins IndexRegs:$z),
                         "lpmw\t$dst, $z",
                         []>,
                  Requires<[/*TODO: check it: HasLPMX */]>;

    def LPMWRdZPi : Pseudo<(outs MainRegs:$dst),
                           (ins IndexRegs:$z),
                           "lpmw\t$dst, $z+",
                           []>,
                    Requires<[/*TODO: check it: HasLPMX */]>;
  }
}

// Extended load program memory operations.
let mayLoad = 1,
hasSideEffects = 0 in
{
  let Defs = [A],
      Uses = [A] in
  def ELPM : F16<0b1001010111011000,
                 (outs),
                 (ins),
                 "elpm",
                 []>,
             Requires<[/*TODO: check it: HasELPM */]>;

  def ELPMRdZ : FLPMX<1,
                      0,
                      (outs MainRegs:$dst),
                      (ins IndexRegs:$z),
                      "elpm\t$dst, $z",
                      []>,
                Requires<[/*TODO: check it: HasELPMX */]>;

  let Defs = [A] in
  def ELPMRdZPi : FLPMX<1,
                        1,
                        (outs MainRegs:$dst),
                        (ins IndexRegs: $z),
                        "elpm\t$dst, $z+",
                        []>,
                  Requires<[/*TODO: check it: HasELPMX */]>;
}

// Store program memory operations.
let Uses = [A] in
{
  let Uses = [A] in
  def SPM : F16<0b1001010111101000,
                (outs),
                (ins),
                "spm",
                []>,
            Requires<[/*TODO: check it: HasSPM */]>;

  let Defs = [A] in
  def SPMZPi : F16<0b1001010111111000,
                   (outs),
                   (ins IndexRegs:$z),
                   "spm $z+",
                   []>,
               Requires<[/*TODO: check it: HasSPMX */]>;
}

// Read data from IO location operations.
let canFoldAsLoad = 1,
isReMaterializable = 1 in
{
  def INRdA : FIORdA<(outs MainRegs:$dst),
                     (ins imm_port6:$src),
                     "in\t$dst, $src",
                     [(set i16:$dst, (load ioaddr8:$src))]>;

  def INWRdA : Pseudo<(outs MainRegs:$dst),
                      (ins imm_port6:$src),
                      "inw\t$dst, $src",
                      [(set i16:$dst, (load ioaddr16:$src))]>;
}

// Write data to IO location operations.
def OUTARr : FIOARr<(outs),
                    (ins imm_port6:$dst, MainRegs:$src),
                    "out\t$dst, $src",
                    [(store i16:$src, ioaddr8:$dst)]>;

def OUTWARr : Pseudo<(outs),
                     (ins imm_port6:$dst, MainRegs:$src),
                     "outw\t$dst, $src",
                     [(store i16:$src, ioaddr16:$dst)]>;

// Stack push/pop operations.
let Defs = [SP],
Uses = [SP],
hasSideEffects = 0 in
{
  // Stack push operations.
  let mayStore = 1 in
  {
    def PUSHRr : FRd<0b1001,
                     0b0011111,
                     (outs),
                     (ins MainRegs:$reg),
                     "push\t$reg",
                     []>,
                 Requires<[/*TODO: check it: HasSRAM */]>;

    def PUSHWRr : Pseudo<(outs),
                         (ins MainRegs:$reg),
                         "pushw\t$reg",
                         []>,
                  Requires<[/*TODO: check it: HasSRAM */]>;
  }

  // Stack pop operations.
  let mayLoad = 1 in
  {
    def POPRd : FRd<0b1001,
                    0b0001111,
                    (outs MainRegs:$reg),
                    (ins),
                    "pop\t$reg",
                    []>,
                Requires<[/*TODO: check it: HasSRAM */]>;

    def POPWRd : Pseudo<(outs MainRegs:$reg),
                        (ins),
                        "popw\t$reg",
                        []>,
                 Requires<[/*TODO: check it: HasSRAM */]>;
  }
}

// Read-Write-Modify (RMW) instructions.
def XCHZRd : FZRd<0b100,
                  (outs MainRegs:$rd),
                  (ins IndexRegs:$z),
                  "xch\t$z, $rd",
                  []>,
             Requires<[/*TODO: check it: SupportsRMW] */]>;

def LASZRd : FZRd<0b101,
                  (outs MainRegs:$rd),
                  (ins IndexRegs:$z),
                  "las\t$z, $rd",
                  []>,
             Requires<[/*TODO: check it: SupportsRMW */]>;

def LACZRd : FZRd<0b110,
                  (outs MainRegs:$rd),
                  (ins IndexRegs:$z),
                  "lac\t$z, $rd",
                  []>,
             Requires<[/*TODO: check it: SupportsRMW */]>;

def LATZRd : FZRd<0b111,
                  (outs MainRegs:$rd),
                  (ins IndexRegs:$z),
                  "lat\t$z, $rd",
                  []>,
             Requires<[/*TODO: check it: SupportsRMW */]>;

//===----------------------------------------------------------------------===//
// Bit and bit-test instructions
//===----------------------------------------------------------------------===//

// Bit shift/rotate operations.
let Constraints = "$src = $rd",
Defs = [P] in
{
  def LSLRd : FRdRr<0b0000,
                    0b11,
                    (outs MainRegs:$rd),
                    (ins MainRegs:$src),
                    "lsl\t$rd",
                    [(set i16:$rd, (SNESlsl i16:$src)), (implicit P)]>;

  def LSLWRd : Pseudo<(outs MainRegs:$rd),
                      (ins MainRegs:$src),
                      "lslw\t$rd",
                      [(set i16:$rd, (SNESlsl i16:$src)), (implicit P)]>;

  def LSRRd : FRd<0b1001,
                  0b0100110,
                  (outs MainRegs:$rd),
                  (ins MainRegs:$src),
                  "lsr\t$rd",
                  [(set i16:$rd, (SNESlsr i16:$src)), (implicit P)]>;

  def LSRWRd : Pseudo<(outs MainRegs:$rd),
                      (ins MainRegs:$src),
                      "lsrw\t$rd",
                      [(set i16:$rd, (SNESlsr i16:$src)), (implicit P)]>;

  def ASRRd : FRd<0b1001,
                  0b0100101,
                  (outs MainRegs:$rd),
                  (ins MainRegs:$src),
                  "asr\t$rd",
                  [(set i16:$rd, (SNESasr i16:$src)), (implicit P)]>;

  def ASRWRd : Pseudo<(outs MainRegs:$rd),
                      (ins MainRegs:$src),
                      "asrw\t$rd",
                      [(set i16:$rd, (SNESasr i16:$src)), (implicit P)]>;

  // Bit rotate operations.
  let Uses = [P] in
  {
    def ROLRd : FRdRr<0b0001,
                      0b11,
                      (outs MainRegs:$rd),
                      (ins MainRegs:$src),
                      "rol\t$rd",
                      [(set i16:$rd, (SNESrol i16:$src)), (implicit P)]>;

    def ROLWRd : Pseudo<(outs MainRegs:$rd),
                        (ins MainRegs:$src),
                        "rolw\t$rd",
                        [(set i16:$rd, (SNESrol i16:$src)), (implicit P)]>;

    def RORRd : FRd<0b1001,
                    0b0100111,
                    (outs MainRegs:$rd),
                    (ins MainRegs:$src),
                    "ror\t$rd",
                    [(set i16:$rd, (SNESror i16:$src)), (implicit P)]>;

    def RORWRd : Pseudo<(outs MainRegs:$rd),
                        (ins MainRegs:$src),
                        "rorw\t$rd",
                        [(set i16:$rd, (SNESror i16:$src)), (implicit P)]>;
  }
}

// SWAP Rd
// Swaps the high and low nibbles in a register.
let Constraints = "$src = $rd" in
def SWAPRd : FRd<0b1001,
                 0b0100010,
                 (outs MainRegs:$rd),
                 (ins MainRegs:$src),
                 "swap\t$rd",
                 [(set i16:$rd, (bswap i16:$src))]>;

// IO register bit set/clear operations.
//:TODO: add patterns when popcount(imm)==2 to be expanded with 2 sbi/cbi
// instead of in+ori+out which requires one more instr.
def SBIAb : FIOBIT<0b10,
                   (outs),
                   (ins imm_port5:$addr, i16imm:$bit),
                   "sbi\t$addr, $bit",
                   [(store (or (i16 (load lowioaddr8:$addr)), iobitpos8:$bit),
                     lowioaddr8:$addr)]>;

def CBIAb : FIOBIT<0b00,
                   (outs),
                   (ins imm_port5:$addr, i16imm:$bit),
                   "cbi\t$addr, $bit",
                   [(store (and (i16 (load lowioaddr8:$addr)), iobitposn8:$bit),
                     lowioaddr8:$addr)]>;

// Status register bit load/store operations.
let Defs = [P] in
def BST : FRdB<0b01,
               (outs),
               (ins MainRegs:$rd, i16imm:$b),
               "bst\t$rd, $b",
               []>;

let Uses = [P] in
def BLD : FRdB<0b00,
               (outs),
               (ins MainRegs:$rd, i16imm:$b),
               "bld\t$rd, $b",
               []>;

// Set/clear bit in register operations.
let Constraints = "$src = $rd",
Defs = [P] in
{
  // SBR Rd, K
  // Alias for ORI Rd, K
  def SBRRdK : FRdK<0b0110,
                    (outs MainRegs:$rd),
                    (ins MainRegs:$src, imm_ldi16:$k),
                    "sbr\t$rd, $k",
                    [(set i16:$rd, (or i16:$src, imm:$k)),
                     (implicit P)]>;

  // CBR Rd, K
  // Alias for `ANDI Rd, COM(K)` where COM(K) is the complement of K.
  // FIXME: This uses the 'complement' encoder. We need it to also use the
  // imm_ldi16 encoder. This will cause no fixups to be created on this instruction.
  def CBRRdK : FRdK<0b0111,
                    (outs MainRegs:$rd),
                    (ins MainRegs:$src, imm_com8:$k),
                    "cbr\t$rd, $k",
                    []>;
}

// CLR Rd
// Alias for EOR Rd, Rd
// -------------
// Clears all bits in a register.
def CLR : InstAlias<"clr\t$rd", (EORRdRr MainRegs:$rd, MainRegs:$rd)>;

// SER Rd
// Alias for LDI Rd, 0xff
// ---------
// Sets all bits in a register.
def : InstAlias<"ser\t$rd", (LDIRdK MainRegs:$rd, 0xff), 0>;

let Defs = [P] in
def BSETs : FS<0,
               (outs),
               (ins i16imm:$s),
               "bset\t$s",
               []>;

let Defs = [P] in
def BCLRs : FS<1,
               (outs),
               (ins i16imm:$s),
               "bclr\t$s",
               []>;

// Set/clear aliases for the zero (Z) status flag (bit 1).
def : InstAlias<"sez", (BSETs 1)>;
def : InstAlias<"clz", (BCLRs 1)>;

// Set/clear aliases for the negative (N) status flag (bit 2).
def : InstAlias<"sen", (BSETs 2)>;
def : InstAlias<"cln", (BCLRs 2)>;

// Set/clear aliases for the overflow (V) status flag (bit 3).
def : InstAlias<"sev", (BSETs 3)>;

// Set/clear aliases for the signed (S) status flag (bit 4).
def : InstAlias<"ses", (BSETs 4)>;
def : InstAlias<"cls", (BCLRs 4)>;

// Set/clear aliases for the half-carry (H) status flag (bit 5).
def : InstAlias<"seh", (BSETs 5)>;
def : InstAlias<"clh", (BCLRs 5)>;

// Set/clear aliases for the T status flag (bit 6).
def : InstAlias<"set", (BSETs 6)>;
def : InstAlias<"clt", (BCLRs 6)>;

//===----------------------------------------------------------------------===//
// Special/Control instructions
//===----------------------------------------------------------------------===//

// BREAK
// Breakpoint instruction
// ---------
// <|1001|0101|1001|1000>
def BREAK : F16<0b1001010110011000,
                (outs),
                (ins),
                "break",
                []>,
            Requires<[/*TODO: check it: HasBREAK */]>;

// SLEEP
// Sleep instruction
// ---------
// <|1001|0101|1000|1000>
def SLEEP : F16<0b1001010110001000,
                (outs),
                (ins),
                "sleep",
                []>;

// WDR
// Watchdog reset
// ---------
// <|1001|0101|1010|1000>
def WDR : F16<0b1001010110101000,
              (outs),
              (ins),
              "wdr",
              []>;

//===----------------------------------------------------------------------===//
// Pseudo instructions for later expansion
//===----------------------------------------------------------------------===//

//:TODO: Optimize this for wider types AND optimize the following code
//       compile int foo(char a, char b, char c, char d) {return d+b;}
//       looks like a missed sext_inreg opportunity.
// def SEXT : ExtensionPseudo<
//   (outs EFLAGSRegs:$dst),
//   (ins MainRegs:$src),
//   "sext\t$dst, $src",
//   [(set i16:$dst, (sext i8:$src)), (implicit P)]
// >;

// def ZEXT : ExtensionPseudo<
//   (outs EFLAGSRegs:$dst),
//   (ins EFLAGSRegs:$src),
//   "zext\t$dst, $src",
//   [(set i8:$dst, (zext i8:$src)), (implicit P)]
// >;

// This pseudo gets expanded into a movw+adiw thus it clobbers P.
let Defs = [P],
    hasSideEffects = 0 in
def FRMIDX : Pseudo<(outs MainRegs:$dst),
                    (ins MainRegs:$src, i16imm:$src2),
                    "frmidx\t$dst, $src, $src2",
                    []>;

// This pseudo is either converted to a regular store or a push which clobbers
// SP.
def STDSPQRr : StorePseudo<
  (outs),
  (ins memspi:$dst, MainRegs:$src),
  "stdstk\t$dst, $src",
  [(store i16:$src, addr:$dst)]
>;

// This pseudo is either converted to a regular store or a push which clobbers
// SP.
def STDWSPQRr : StorePseudo<
  (outs),
  (ins memspi:$dst, MainRegs:$src),
  "stdwstk\t$dst, $src",
  [(store i16:$src, addr:$dst)]
>;

// SP read/write pseudos.
let hasSideEffects = 0 in
{
  let Uses = [SP] in
  def SPREAD : Pseudo<
    (outs MainRegs:$dst),
    (ins StackPointerRegs:$src),
    "spread\t$dst, $src",
    []
  >;

  let Defs = [SP] in
  def SPWRITE : Pseudo<
    (outs StackPointerRegs:$dst),
    (ins MainRegs:$src),
    "spwrite\t$dst, $src",
    []>;
}

def Select8 : SelectPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$src2, i16imm:$cc),
  "# Select8 PSEUDO",
  [(set i16:$dst, (SNESselectcc i16:$src, i16:$src2, imm:$cc))]
>;

def Select16 : SelectPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$src2, i16imm:$cc),
  "# Select16 PSEUDO",
  [(set i16:$dst, (SNESselectcc i16:$src, i16:$src2, imm:$cc))]
>;

def Lsl8 : ShiftPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$cnt),
  "# Lsl8 PSEUDO",
  [(set i16:$dst, (SNESlslLoop i16:$src, i16:$cnt))]
>;

def Lsl16 : ShiftPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$cnt),
  "# Lsl16 PSEUDO",
  [(set i16:$dst, (SNESlslLoop i16:$src, i16:$cnt))]
>;

def Lsr8 : ShiftPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$cnt),
  "# Lsr8 PSEUDO",
  [(set i16:$dst, (SNESlsrLoop i16:$src, i16:$cnt))]
>;

def Lsr16 : ShiftPseudo<
  (outs MainRegs:$dst),
   (ins MainRegs:$src, MainRegs:$cnt),
   "# Lsr16 PSEUDO",
   [(set i16:$dst, (SNESlsrLoop i16:$src, i16:$cnt))]
>;

def Rol8 : ShiftPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$cnt),
  "# Rol8 PSEUDO",
  [(set i16:$dst, (SNESrolLoop i16:$src, i16:$cnt))]
>;

def Rol16 : ShiftPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$cnt),
  "# Rol16 PSEUDO",
  [(set i16:$dst, (SNESrolLoop i16:$src, i16:$cnt))]
>;

def Ror8 : ShiftPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$cnt),
  "# Ror8 PSEUDO",
  [(set i16:$dst, (SNESrorLoop i16:$src, i16:$cnt))]
>;

def Ror16 : ShiftPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$cnt),
  "# Ror16 PSEUDO",
  [(set i16:$dst, (SNESrorLoop i16:$src, i16:$cnt))]
>;

def Asr8 : ShiftPseudo<
  (outs MainRegs:$dst),
  (ins MainRegs:$src, MainRegs:$cnt),
  "# Asr8 PSEUDO",
  [(set i16:$dst, (SNESasrLoop i16:$src, i16:$cnt))]
>;

def Asr16 : ShiftPseudo<
  (outs MainRegs:$dst),
   (ins MainRegs:$src, MainRegs:$cnt),
   "# Asr16 PSEUDO",
   [(set i16:$dst, (SNESasrLoop i16:$src, i16:$cnt))]
>;


//===----------------------------------------------------------------------===//
// Non-Instruction Patterns
//===----------------------------------------------------------------------===//

//:TODO: look in x86InstrCompiler.td for odd encoding trick related to
// add x, 128 -> sub x, -128. Clang is emitting an eor for this (ldi+eor)

// the add instruction always writes the carry flag
def : Pat<(addc i16:$src, i16:$src2),
          (ADDRdRr i16:$src, i16:$src2)>;
def : Pat<(addc MainRegs:$src, MainRegs:$src2),
          (ADDWRdRr MainRegs:$src, MainRegs:$src2)>;

// all sub instruction variants always writes the carry flag
def : Pat<(subc i16:$src, i16:$src2),
          (SUBRdRr i16:$src, i16:$src2)>;
def : Pat<(subc i16:$src, i16:$src2),
          (SUBWRdRr i16:$src, i16:$src2)>;
def : Pat<(subc i16:$src, imm:$src2),
          (SUBIRdK i16:$src, imm:$src2)>;
def : Pat<(subc i16:$src, imm:$src2),
          (SUBIWRdK i16:$src, imm:$src2)>;

// These patterns convert add (x, -imm) to sub (x, imm) since we dont have
// any add with imm instructions. Also take care of the adiw/sbiw instructions.
def : Pat<(add i16:$src1, imm0_63_neg:$src2),
          (SBIWRdK i16:$src1, (imm0_63_neg:$src2))>;
def : Pat<(add i16:$src1, imm:$src2),
          (SUBIWRdK i16:$src1, (imm16_neg_XFORM imm:$src2))>;
def : Pat<(addc i16:$src1, imm:$src2),
          (SUBIWRdK i16:$src1, (imm16_neg_XFORM imm:$src2))>;
def : Pat<(adde i16:$src1, imm:$src2),
          (SBCIWRdK i16:$src1, (imm16_neg_XFORM imm:$src2))>;

def : Pat<(add i16:$src1, imm:$src2),
          (SUBIRdK i16:$src1, (imm8_neg_XFORM imm:$src2))>;
def : Pat<(addc i16:$src1, imm:$src2),
          (SUBIRdK i16:$src1, (imm8_neg_XFORM imm:$src2))>;
def : Pat<(adde i16:$src1, imm:$src2),
          (SBCIRdK i16:$src1, (imm8_neg_XFORM imm:$src2))>;

// Calls.
def : Pat<(SNEScall (i16 tglobaladdr:$dst)),
          (CALLk tglobaladdr:$dst)>;
def : Pat<(SNEScall (i16 texternalsym:$dst)),
          (CALLk texternalsym:$dst)>;

// `anyext`
// def : Pat<(i16 (anyext i16:$src)),
//           (INSERT_SUBREG (i16 (IMPLICIT_DEF)), i16:$src, sub_lo)>;

// `trunc`
// def : Pat<(i16 (trunc i16:$src)),
//           (EXTRACT_SUBREG i16:$src, sub_lo)>;

// sext_inreg
// def : Pat<(sext_inreg i16:$src, i8),
//           (SEXT (i8 (EXTRACT_SUBREG i16:$src, sub_lo)))>;

// GlobalAddress
def : Pat<(i16 (SNESWrapper tglobaladdr:$dst)),
          (LDIWRdK tglobaladdr:$dst)>;
def : Pat<(add i16:$src, (SNESWrapper tglobaladdr:$src2)),
          (SUBIWRdK i16:$src, tglobaladdr:$src2)>;
def : Pat<(i16 (load (SNESWrapper tglobaladdr:$dst))),
          (LDSRdK tglobaladdr:$dst)>;
def : Pat<(i16 (load (SNESWrapper tglobaladdr:$dst))),
          (LDSWRdK tglobaladdr:$dst)>;
def : Pat<(store i16:$src, (i16 (SNESWrapper tglobaladdr:$dst))),
          (STSKRr tglobaladdr:$dst, i16:$src)>;
def : Pat<(store i16:$src, (i16 (SNESWrapper tglobaladdr:$dst))),
          (STSWKRr tglobaladdr:$dst, i16:$src)>;

// BlockAddress
def : Pat<(i16 (SNESWrapper tblockaddress:$dst)),
          (LDIWRdK tblockaddress:$dst)>;

// hi-reg truncation : trunc(int16 >> 8)
//:FIXME: i think it's better to emit an extract subreg node in the DAG than
// all this mess once we get optimal shift code
// lol... I think so, too. [@agnat]
// def : Pat<(i16 (trunc (SNESlsr (SNESlsr (SNESlsr (SNESlsr (SNESlsr (SNESlsr (SNESlsr
//                      (SNESlsr MainRegs:$src)))))))))),
//           (EXTRACT_SUBREG MainRegs:$src, sub_hi)>;

// :FIXME: DAGCombiner produces an shl node after legalization from these seq:
// BR_JT -> (mul x, 2) -> (shl x, 1)
def : Pat<(shl i16:$src1, (i16 1)),
          (LSLWRd i16:$src1)>;

