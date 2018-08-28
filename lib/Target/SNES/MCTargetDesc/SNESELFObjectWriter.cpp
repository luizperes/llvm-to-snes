//===-- SNESELFObjectWriter.cpp - SNES ELF Writer ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/SNESFixupKinds.h"
#include "MCTargetDesc/SNESMCTargetDesc.h"

#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

/// Writes SNES machine code into an ELF32 object file.
class SNESELFObjectWriter : public MCELFObjectTargetWriter {
public:
  SNESELFObjectWriter(uint8_t OSABI);

  virtual ~SNESELFObjectWriter() {}

  unsigned getRelocType(MCContext &Ctx,
                        const MCValue &Target,
                        const MCFixup &Fixup,
                        bool IsPCRel) const override;
};

SNESELFObjectWriter::SNESELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(false, OSABI, ELF::EM_SNES, true, false) {}

unsigned SNESELFObjectWriter::getRelocType(MCContext &Ctx,
                                          const MCValue &Target,
                                          const MCFixup &Fixup,
                                          bool IsPCRel) const {
// TODO: check the code below
/*
  switch ((unsigned) Fixup.getKind()) {
  case FK_Data_1:
  case FK_Data_4:
    llvm_unreachable("unsupported relocation type");
  case FK_Data_2:
    return ELF::R_SNES_16_PM;
  case SNES::fixup_32:
    return ELF::R_SNES_32;
  case SNES::fixup_7_pcrel:
    return ELF::R_SNES_7_PCREL;
  case SNES::fixup_13_pcrel:
    return ELF::R_SNES_13_PCREL;
  case SNES::fixup_16:
    return ELF::R_SNES_16;
  case SNES::fixup_16_pm:
    return ELF::R_SNES_16_PM;
  case SNES::fixup_lo8_ldi:
    return ELF::R_SNES_LO8_LDI;
  case SNES::fixup_hi8_ldi:
    return ELF::R_SNES_HI8_LDI;
  case SNES::fixup_hh8_ldi:
    return ELF::R_SNES_HH8_LDI;
  case SNES::fixup_lo8_ldi_neg:
    return ELF::R_SNES_LO8_LDI_NEG;
  case SNES::fixup_hi8_ldi_neg:
    return ELF::R_SNES_HI8_LDI_NEG;
  case SNES::fixup_hh8_ldi_neg:
    return ELF::R_SNES_HH8_LDI_NEG;
  case SNES::fixup_lo8_ldi_pm:
    return ELF::R_SNES_LO8_LDI_PM;
  case SNES::fixup_hi8_ldi_pm:
    return ELF::R_SNES_HI8_LDI_PM;
  case SNES::fixup_hh8_ldi_pm:
    return ELF::R_SNES_HH8_LDI_PM;
  case SNES::fixup_lo8_ldi_pm_neg:
    return ELF::R_SNES_LO8_LDI_PM_NEG;
  case SNES::fixup_hi8_ldi_pm_neg:
    return ELF::R_SNES_HI8_LDI_PM_NEG;
  case SNES::fixup_hh8_ldi_pm_neg:
    return ELF::R_SNES_HH8_LDI_PM_NEG;
  case SNES::fixup_call:
    return ELF::R_SNES_CALL;
  case SNES::fixup_ldi:
    return ELF::R_SNES_LDI;
  case SNES::fixup_6:
    return ELF::R_SNES_6;
  case SNES::fixup_6_adiw:
    return ELF::R_SNES_6_ADIW;
  case SNES::fixup_ms8_ldi:
    return ELF::R_SNES_MS8_LDI;
  case SNES::fixup_ms8_ldi_neg:
    return ELF::R_SNES_MS8_LDI_NEG;
  case SNES::fixup_lo8_ldi_gs:
    return ELF::R_SNES_LO8_LDI_GS;
  case SNES::fixup_hi8_ldi_gs:
    return ELF::R_SNES_HI8_LDI_GS;
  case SNES::fixup_8:
    return ELF::R_SNES_8;
  case SNES::fixup_8_lo8:
    return ELF::R_SNES_8_LO8;
  case SNES::fixup_8_hi8:
    return ELF::R_SNES_8_HI8;
  case SNES::fixup_8_hlo8:
    return ELF::R_SNES_8_HLO8;
  case SNES::fixup_sym_diff:
    return ELF::R_SNES_SYM_DIFF;
  case SNES::fixup_16_ldst:
    return ELF::R_SNES_16_LDST;
  case SNES::fixup_lds_sts_16:
    return ELF::R_SNES_LDS_STS_16;
  case SNES::fixup_port6:
    return ELF::R_SNES_PORT6;
  case SNES::fixup_port5:
    return ELF::R_SNES_PORT5;
  default:
    llvm_unreachable("invalid fixup kind!");
  }
*/

  return ELF::EM_SNES;
}

MCObjectWriter *createSNESELFObjectWriter(raw_pwrite_stream &OS, uint8_t OSABI) {
  MCELFObjectTargetWriter *MOTW = new SNESELFObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, true);
}

} // end of namespace llvm

