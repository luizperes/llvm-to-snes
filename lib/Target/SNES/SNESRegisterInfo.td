//===-- SNESRegisterInfo.td - SNES Register defs -------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the SNES register file
//===----------------------------------------------------------------------===//

// 8-16-bit register definition.
class SNESReg<bits<16> num,
              string name,
              list<Register> subregs = [],
              list<string> altNames = []>
  : RegisterWithSubRegs<name, subregs>
{
  field bits<16> Num = num;

  let HWEncoding = num;
  let Namespace = "SNES";
  let SubRegs = subregs;
  let AltNames = altNames;
}

// TODO: check if subregisters are needed.
// Using them because the registers A, X and Y
// can be changed by setting native or non-native mode
// (0 = 16-bit, 1 = 8-bit)
// Subregister indices.
let Namespace = "SNES" in
{
  def sub_lo : SubRegIndex<8>;
  def sub_hi : SubRegIndex<8, 8>;
}

// TODO: check if we should use the correct mnemonic for registers
//===----------------------------------------------------------------------===//
//  Accumulator registers
//===----------------------------------------------------------------------===//
def AL : SNESReg<0, "AL">,  DwarfRegNum<[0]>;
def AH : SNESReg<3, "AH">,  DwarfRegNum<[3]>;

//===----------------------------------------------------------------------===//
//  Index registers
//===----------------------------------------------------------------------===//
def XL  : SNESReg<1, "XL">,  DwarfRegNum<[1]>;
def XH  : SNESReg<4, "XH">,  DwarfRegNum<[4]>;
def YL  : SNESReg<2, "YL">,  DwarfRegNum<[2]>;
def YH  : SNESReg<5, "YH">,  DwarfRegNum<[5]>;

//===----------------------------------------------------------------------===//
//  Subregisters
//===----------------------------------------------------------------------===//
let SubRegIndices = [sub_lo, sub_hi], CoveredBySubRegs = 1 in {
  def A : SNESReg<0, "A", [AL,AH]>, DwarfRegNum<[0]>;
  def X : SNESReg<1, "X", [XL,XH]>, DwarfRegNum<[1]>;
  def Y : SNESReg<2, "Y", [YL,YH]>, DwarfRegNum<[2]>;
}

//===----------------------------------------------------------------------===//
//  8-bit control registers
//===----------------------------------------------------------------------===//
def DB : SNESReg<9,  "DB">,  DwarfRegNum<[9]>; // Data Bank
def P  : SNESReg<10, "P">,  DwarfRegNum<[10]>; // Processor Status - Status register
def PB : SNESReg<11, "PB">, DwarfRegNum<[11]>; // Program Bank

//===----------------------------------------------------------------------===//
//  16-bit control registers
//===----------------------------------------------------------------------===//
def SP : SNESReg<6, "SP">, DwarfRegNum<[6]>; // Stack Pointer
def DP : SNESReg<7, "DP">, DwarfRegNum<[7]>; // Direct Page
def PC : SNESReg<8, "PC">, DwarfRegNum<[8]>; // Program Counter

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

// Main 8-bit (low) register class.
def MainLoRegs : RegisterClass<"SNES", [i8], 8, (add AL, XL, YL)> {
  let AltOrders = [(sub MainLoRegs, AH, XH, YH)];
  let AltOrderSelect = [{ return 0; }]; // the default
}

// Main 16-bit register class.
def MainRegs : RegisterClass<"SNES", [i16], 8, (add A, X, Y)>;

// Accumulator register class.
def AccRegs : RegisterClass<"SNES", [i16], 8, (add A)>;

// Accumulator register class.
def Acc8Regs : RegisterClass<"SNES", [i8], 8, (add AL)>;

// TODO: Check if index regs need to be in separated classes
// Indexes register class.
def IndexRegs : RegisterClass<"SNES", [i16], 8, (add X, Y)>;

// Direct pages register class
def DirectPageRegs : RegisterClass<"SNES", [i16], 8, (add DP)>;

// Stack pointer register class
def StackPointerRegs : RegisterClass<"SNES", [i16], 8, (add SP)>;

// Program counter register class
def ProgramCounterRegs : RegisterClass<"SNES", [i16], 8, (add PC)>;

// Data bank register class.
def DataBankRegs : RegisterClass<"SNES", [i8], 8, (add DB)>;

// Program bank register class
def ProgramBankRegs : RegisterClass<"SNES", [i8], 8, (add PB)>;

// Status pointer register class.
def EFLAGSRegs : RegisterClass<"SNES", [i8], 8, (add P)> {
  let CopyCost = -1;  // Don't allow copying of status registers.
  let isAllocatable = 0;
}
