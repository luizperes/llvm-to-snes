//===-- SNESTargetStreamer.h - SNES Target Streamer --------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_TARGET_STREAMER_H
#define LLVM_SNES_TARGET_STREAMER_H

#include "llvm/MC/MCELFStreamer.h"

namespace llvm {
class MCStreamer;

/// A generic SNES target output stream.
class SNESTargetStreamer : public MCTargetStreamer {
public:
  explicit SNESTargetStreamer(MCStreamer &S);
};

/// A target streamer for textual SNES assembly code.
class SNESTargetAsmStreamer : public SNESTargetStreamer {
public:
  explicit SNESTargetAsmStreamer(MCStreamer &S);
};

} // end namespace llvm

#endif // LLVM_SNES_TARGET_STREAMER_H
