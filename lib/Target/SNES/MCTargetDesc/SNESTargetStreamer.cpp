//===-- SNESTargetStreamer.cpp - SNES Target Streamer Methods ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides SNES specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "SNESTargetStreamer.h"

namespace llvm {

SNESTargetStreamer::SNESTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

SNESTargetAsmStreamer::SNESTargetAsmStreamer(MCStreamer &S)
    : SNESTargetStreamer(S) {}

} // end namespace llvm

