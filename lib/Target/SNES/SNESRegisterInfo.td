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

class SNES8Reg<bits<8> Enc, string n> : Register<n> {
  let HWEncoding = Enc;
  let Namespace = "WLADX";
}

class SNES16Reg<bits<16> Enc, string n> : Register<n> {
  let HWEncoding = Enc;
  let Namespace = "WLADX";
}


