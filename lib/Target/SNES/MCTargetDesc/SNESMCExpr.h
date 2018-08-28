//===-- SNESMCExpr.h - SNES specific MC expression classes --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SNES_MCEXPR_H
#define LLVM_SNES_MCEXPR_H

#include "llvm/MC/MCExpr.h"

#include "MCTargetDesc/SNESFixupKinds.h"

namespace llvm {

/// A expression in SNES machine code.
class SNESMCExpr : public MCTargetExpr {
public:
  /// Specifies the type of an expression.
  enum VariantKind {
    VK_SNES_None,

    VK_SNES_HI8,  ///< Corresponds to `hi8()`.
    VK_SNES_LO8,  ///< Corresponds to `lo8()`.
    VK_SNES_HH8,  ///< Corresponds to `hlo8() and hh8()`.
    VK_SNES_HHI8, ///< Corresponds to `hhi8()`.

    VK_SNES_PM_LO8, ///< Corresponds to `pm_lo8()`.
    VK_SNES_PM_HI8, ///< Corresponds to `pm_hi8()`.
    VK_SNES_PM_HH8  ///< Corresponds to `pm_hh8()`.
  };

public:
  /// Creates an SNES machine code expression.
  static const SNESMCExpr *create(VariantKind Kind, const MCExpr *Expr,
                                 bool isNegated, MCContext &Ctx);

  /// Gets the type of the expression.
  VariantKind getKind() const { return Kind; }
  /// Gets the name of the expression.
  const char *getName() const;
  const MCExpr *getSubExpr() const { return SubExpr; }
  /// Gets the fixup which corresponds to the expression.
  SNES::Fixups getFixupKind() const;
  /// Evaluates the fixup as a constant value.
  bool evaluateAsConstant(int64_t &Result) const;

  bool isNegated() const { return Negated; }
  void setNegated(bool negated = true) { Negated = negated; }

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  bool evaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout,
                                 const MCFixup *Fixup) const override;

  void visitUsedExpr(MCStreamer &streamer) const override;

  MCFragment *findAssociatedFragment() const override {
    return getSubExpr()->findAssociatedFragment();
  }

  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const override {}

  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }

public:
  static VariantKind getKindByName(StringRef Name);

private:
  int64_t evaluateAsInt64(int64_t Value) const;

  const VariantKind Kind;
  const MCExpr *SubExpr;
  bool Negated;

private:
  explicit SNESMCExpr(VariantKind Kind, const MCExpr *Expr, bool Negated)
      : Kind(Kind), SubExpr(Expr), Negated(Negated) {}
  ~SNESMCExpr() {}
};

} // end namespace llvm

#endif // LLVM_SNES_MCEXPR_H
