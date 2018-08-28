//===----- SNESELFStreamer.h - SNES Target Streamer --------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_ELF_STREAMER_H
#define LLVM_SNES_ELF_STREAMER_H

#include "SNESTargetStreamer.h"

namespace llvm {

/// A target streamer for an SNES ELF object file.
class SNESELFStreamer : public SNESTargetStreamer {
public:
  SNESELFStreamer(MCStreamer &S, const MCSubtargetInfo &STI);

  MCELFStreamer &getStreamer() {
    return static_cast<MCELFStreamer &>(Streamer);
  }
};

} // end namespace llvm

#endif
