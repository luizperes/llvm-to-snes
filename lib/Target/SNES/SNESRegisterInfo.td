//===-- SNESRegisterInfo.td - SNES Register defs ---------*- tablegen -*-===//
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

class SNESReg<bits<16> Enc, string n> : Register<n> {
  let HWEncoding = Enc;
  let Namespace = "SNES";
}

class SNESCtrlReg8<bits<16> Enc, string n>: Register<n> {
  let HWEncoding = Enc;
  let Namespace = "SNES";
}

class SNESCtrlReg<bits<16> Enc, string n>: Register<n> {
  let HWEncoding = Enc;
  let Namespace = "SNES";
}

def A  : SNESReg<0, "A">;
def X  : SNESReg<1, "X">;
def Y  : SNESReg<2, "Y">;

def P  : SNESCtrlReg8<3, "P">;
def SP : SNESCtrlReg8<4, "SP">;
def PB : SNESCtrlReg8<5, "PB">;

def DP : SNESCtrlReg<6, "DP">;
def PC : SNESCtrlReg<7, "PC">;


