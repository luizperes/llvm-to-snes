//===-- SNESInstrFormats.td - SNES Instruction Formats ---*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

class SNESInst<dag outs, dag ins, string asmstr, list<dag> pattern,
             InstrItinClass itin = NoItinerary>
   : Instruction {

  let Namespace = "SNES";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString   = asmstr;
  let Pattern = pattern;

  let DecoderNamespace = "Super Nintendo";
  let Itinerary = itin;
}

class SNESInst8<dag outs, dag ins, string asmstr, list<dag> pattern,
             InstrItinClass itin = NoItinerary>
   : SNESInst<outs, ins, asmstr, pattern> {
  field bits<8> Inst;

  let bits<8> op;
  let Inst{7-0} = op;  
}

class SNESInst16<dag outs, dag ins, string asmstr, list<dag> pattern,
             InstrItinClass itin = NoItinerary>
   : SNESInst<outs, ins, asmstr, pattern> {
  field bits<16> Inst;

  let bits<8> op;
  let Inst{15-8} = op;

  let bits<8> value;
  let Inst{7-0} = value;
}

class SNESInst24<dag outs, dag ins, string asmstr, list<dag> pattern,
             InstrItinClass itin = NoItinerary>
   : SNESInst<outs, ins, asmstr, pattern> {
  field bits<24> Inst;

  let bits<8> op;
  let Inst{23-16} = op;

  let bits<16> value;
  let Inst{15-0} = value;
}

class SNESInst32<dag outs, dag ins, string asmstr, list<dag> pattern,
             InstrItinClass itin = NoItinerary>
   : SNESInst<outs, ins, asmstr, pattern> {
  field bits<32> Inst;

  let bits<8> op;
  let Inst{31-24} = op;

  let bits<24> value;
  let Inst{23-0} = value;
}

