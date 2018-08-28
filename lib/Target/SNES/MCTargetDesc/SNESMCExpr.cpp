//===-- SNESMCExpr.cpp - SNES specific MC expression classes ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SNESMCExpr.h"

#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCValue.h"

namespace llvm {

namespace {

const struct ModifierEntry {
  const char * const Spelling;
  SNESMCExpr::VariantKind VariantKind;
} ModifierNames[] = {
    {"lo8", SNESMCExpr::VK_SNES_LO8},       {"hi8", SNESMCExpr::VK_SNES_HI8},
    {"hh8", SNESMCExpr::VK_SNES_HH8}, // synonym with hlo8
    {"hlo8", SNESMCExpr::VK_SNES_HH8},      {"hhi8", SNESMCExpr::VK_SNES_HHI8},

    {"pm_lo8", SNESMCExpr::VK_SNES_PM_LO8}, {"pm_hi8", SNESMCExpr::VK_SNES_PM_HI8},
    {"pm_hh8", SNESMCExpr::VK_SNES_PM_HH8},
};

} // end of anonymous namespace

const SNESMCExpr *SNESMCExpr::create(VariantKind Kind, const MCExpr *Expr,
                                   bool Negated, MCContext &Ctx) {
  return new (Ctx) SNESMCExpr(Kind, Expr, Negated);
}

void SNESMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  assert(Kind != VK_SNES_None);

  if (isNegated())
    OS << '-';

  OS << getName() << '(';
  getSubExpr()->print(OS, MAI);
  OS << ')';
}

bool SNESMCExpr::evaluateAsConstant(int64_t &Result) const {
  MCValue Value;

  bool isRelocatable =
      getSubExpr()->evaluateAsRelocatable(Value, nullptr, nullptr);

  if (!isRelocatable)
    return false;

  if (Value.isAbsolute()) {
    Result = evaluateAsInt64(Value.getConstant());
    return true;
  }

  return false;
}

bool SNESMCExpr::evaluateAsRelocatableImpl(MCValue &Result,
                                          const MCAsmLayout *Layout,
                                          const MCFixup *Fixup) const {
  MCValue Value;
  bool isRelocatable = SubExpr->evaluateAsRelocatable(Value, Layout, Fixup);

  if (!isRelocatable)
    return false;

  if (Value.isAbsolute()) {
    Result = MCValue::get(evaluateAsInt64(Value.getConstant()));
  } else {
    if (!Layout) return false;

    MCContext &Context = Layout->getAssembler().getContext();
    const MCSymbolRefExpr *Sym = Value.getSymA();
    MCSymbolRefExpr::VariantKind Modifier = Sym->getKind();
    if (Modifier != MCSymbolRefExpr::VK_None)
      return false;

    Sym = MCSymbolRefExpr::create(&Sym->getSymbol(), Modifier, Context);
    Result = MCValue::get(Sym, Value.getSymB(), Value.getConstant());
  }

  return true;
}

int64_t SNESMCExpr::evaluateAsInt64(int64_t Value) const {
  if (Negated)
    Value *= -1;

  switch (Kind) {
  case SNESMCExpr::VK_SNES_LO8:
    break;
  case SNESMCExpr::VK_SNES_HI8:
    Value >>= 8;
    break;
  case SNESMCExpr::VK_SNES_HH8:
    Value >>= 16;
    break;
  case SNESMCExpr::VK_SNES_HHI8:
    Value >>= 24;
    break;
  case SNESMCExpr::VK_SNES_PM_LO8:
    Value >>= 1;
    break;
  case SNESMCExpr::VK_SNES_PM_HI8:
    Value >>= 9;
    break;
  case SNESMCExpr::VK_SNES_PM_HH8:
    Value >>= 17;
    break;

  case SNESMCExpr::VK_SNES_None:
    llvm_unreachable("Uninitialized expression.");
  }
  return static_cast<uint64_t>(Value) & 0xff;
}

SNES::Fixups SNESMCExpr::getFixupKind() const {
  SNES::Fixups Kind = SNES::Fixups::LastTargetFixupKind;

  switch (getKind()) {
  case VK_SNES_LO8:
    Kind = isNegated() ? SNES::fixup_lo8_ldi_neg : SNES::fixup_lo8_ldi;
    break;
  case VK_SNES_HI8:
    Kind = isNegated() ? SNES::fixup_hi8_ldi_neg : SNES::fixup_hi8_ldi;
    break;
  case VK_SNES_HH8:
    Kind = isNegated() ? SNES::fixup_hh8_ldi_neg : SNES::fixup_hh8_ldi;
    break;
  case VK_SNES_HHI8:
    Kind = isNegated() ? SNES::fixup_ms8_ldi_neg : SNES::fixup_ms8_ldi;
    break;

  case VK_SNES_PM_LO8:
    Kind = isNegated() ? SNES::fixup_lo8_ldi_pm_neg : SNES::fixup_lo8_ldi_pm;
    break;
  case VK_SNES_PM_HI8:
    Kind = isNegated() ? SNES::fixup_hi8_ldi_pm_neg : SNES::fixup_hi8_ldi_pm;
    break;
  case VK_SNES_PM_HH8:
    Kind = isNegated() ? SNES::fixup_hh8_ldi_pm_neg : SNES::fixup_hh8_ldi_pm;
    break;

  case VK_SNES_None:
    llvm_unreachable("Uninitialized expression");
  }

  return Kind;
}

void SNESMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

const char *SNESMCExpr::getName() const {
  const auto &Modifier = std::find_if(
      std::begin(ModifierNames), std::end(ModifierNames),
      [this](ModifierEntry const &Mod) { return Mod.VariantKind == Kind; });

  if (Modifier != std::end(ModifierNames)) {
    return Modifier->Spelling;
  }
  return nullptr;
}

SNESMCExpr::VariantKind SNESMCExpr::getKindByName(StringRef Name) {
  const auto &Modifier = std::find_if(
      std::begin(ModifierNames), std::end(ModifierNames),
      [&Name](ModifierEntry const &Mod) { return Mod.Spelling == Name; });

  if (Modifier != std::end(ModifierNames)) {
    return Modifier->VariantKind;
  }
  return VK_SNES_None;
}

} // end of namespace llvm

