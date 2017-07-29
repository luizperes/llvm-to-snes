//===-- SNESInstrInfo.td - Target Description for SNES Target -----------===//
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

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//

include "SNESInstrFormats.td"

class NoValueIntr<dag outs, dag ins, string asmstr, list<dag> pattern>
   : InstSNES8<outs, ins, asmstr, pattern> {
}

class ImmediateIntr<dag outs, dag ins, string asmstr, list<dag> pattern>
   : InstSNES16<outs, ins, asmstr, pattern> {
}

class AbsoluteIntr<dag outs, dag ins, string asmstr, list<dag> pattern>
   : InstSNES24<outs, ins, asmstr, pattern> {
}

class AbsolutLongInstr<dag outs, dag ins, string asmstr, list<dag> pattern>
   : InstSNES32<outs, ins, asmstr, pattern> {
}


