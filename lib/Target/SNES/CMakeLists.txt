set(LLVM_TARGET_DEFINITIONS SNES.td)

tablegen(LLVM SNESGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM SNESGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM SNESGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM SNESGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM SNESGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM SNESGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM SNESGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM SNESGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM SNESGenCallingConv.inc -gen-callingconv)
tablegen(LLVM SNESGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM SNESGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM SNESGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM SNESGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM SNESGenSubtargetInfo.inc -gen-subtarget)
add_public_tablegen_target(SNESCommonTableGen)

add_llvm_target(SNESCodeGen
  SNESAsmPrinter.cpp
  SNESExpandPseudoInsts.cpp
  SNESFrameLowering.cpp
  SNESInstrInfo.cpp
  SNESInstrumentFunctions.cpp
  SNESISelDAGToDAG.cpp
  SNESISelLowering.cpp
  SNESMCInstLower.cpp
  SNESRelaxMemOperations.cpp
  SNESRegisterInfo.cpp
  SNESSubtarget.cpp
  SNESTargetMachine.cpp
  SNESTargetObjectFile.cpp

  DEPENDS
  intrinsics_gen
)

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(InstPrinter)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
