#include "SNESELFStreamer.h"

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Support/FormattedStream.h"

#include "SNESMCTargetDesc.h"

namespace llvm {

static unsigned getEFlagsForFeatureSet(const FeatureBitset &Features) {
  unsigned EFlags = 0;

// TODO: check the code below
/*
  // Set architecture
  if (Features[SNES::ELFArchSNES1])
    EFlags |= ELF::EF_SNES_ARCH_SNES1;
  else if (Features[SNES::ELFArchSNES2])
    EFlags |= ELF::EF_SNES_ARCH_SNES2;
  else if (Features[SNES::ELFArchSNES25])
    EFlags |= ELF::EF_SNES_ARCH_SNES25;
  else if (Features[SNES::ELFArchSNES3])
    EFlags |= ELF::EF_SNES_ARCH_SNES3;
  else if (Features[SNES::ELFArchSNES31])
    EFlags |= ELF::EF_SNES_ARCH_SNES31;
  else if (Features[SNES::ELFArchSNES35])
    EFlags |= ELF::EF_SNES_ARCH_SNES35;
  else if (Features[SNES::ELFArchSNES4])
    EFlags |= ELF::EF_SNES_ARCH_SNES4;
  else if (Features[SNES::ELFArchSNES5])
    EFlags |= ELF::EF_SNES_ARCH_SNES5;
  else if (Features[SNES::ELFArchSNES51])
    EFlags |= ELF::EF_SNES_ARCH_SNES51;
  else if (Features[SNES::ELFArchSNES6])
    EFlags |= ELF::EF_SNES_ARCH_SNES6;
  else if (Features[SNES::ELFArchTiny])
    EFlags |= ELF::EF_SNES_ARCH_SNESTINY;
  else if (Features[SNES::ELFArchXMEGA1])
    EFlags |= ELF::EF_SNES_ARCH_XMEGA1;
  else if (Features[SNES::ELFArchXMEGA2])
    EFlags |= ELF::EF_SNES_ARCH_XMEGA2;
  else if (Features[SNES::ELFArchXMEGA3])
    EFlags |= ELF::EF_SNES_ARCH_XMEGA3;
  else if (Features[SNES::ELFArchXMEGA4])
    EFlags |= ELF::EF_SNES_ARCH_XMEGA4;
  else if (Features[SNES::ELFArchXMEGA5])
    EFlags |= ELF::EF_SNES_ARCH_XMEGA5;
  else if (Features[SNES::ELFArchXMEGA6])
    EFlags |= ELF::EF_SNES_ARCH_XMEGA6;
  else if (Features[SNES::ELFArchXMEGA7])
    EFlags |= ELF::EF_SNES_ARCH_XMEGA7;
*/

  return EFlags;
}

SNESELFStreamer::SNESELFStreamer(MCStreamer &S,
                               const MCSubtargetInfo &STI)
    : SNESTargetStreamer(S) {

  MCAssembler &MCA = getStreamer().getAssembler();
  unsigned EFlags = MCA.getELFHeaderEFlags();

  EFlags |= getEFlagsForFeatureSet(STI.getFeatureBits());

  MCA.setELFHeaderEFlags(EFlags);
}

} // end namespace llvm
