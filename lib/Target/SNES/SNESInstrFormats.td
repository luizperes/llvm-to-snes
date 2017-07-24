//===-- SNESInstrFormats.td - SNES Instruction Formats ---*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

class InstSNES8<dag outs, dag ins, string asmstr, list<dag> pattern,
             InstrItinClass itin = NoItinerary>
   : Instruction {
  field bits<8> Inst;

  let Namespace = "SNES";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString   = asmstr;
  let Pattern = pattern;

  let DecoderNamespace = "SuperNintendo";
  let Itinerary = itin;
}

//class InstSNES16<dag outs, dag ins, string asmstr, list<dag> pattern,
//             InstrItinClass itin = NoItinerary>
//   : Instruction {
//  field bits<16> Inst;
//
//  let Namespace = "SNES";
//  dag OutOperandList = outs;
//  dag InOperandList = ins;
//  let AsmString   = asmstr;
//  let Pattern = pattern;
//
//  let DecoderNamespace = "SuperNintendo";
//  let Itinerary = itin;
//}


