set(LLVM_TARGET_DEFINITIONS SNES.td)

tablegen(LLVM SNESGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM SNESGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM SNESGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM SNESGenCallingConv.inc -gen-callingconv)
add_public_tablegen_target(SNESCommonTableGen)

add_llvm_target(SNESCodeGen
  SNESInstrInfo.cpp
  SNESFrameLowering.cpp
  SNESRegisterInfo.cpp
  SNESSubtarget.cpp
  SNESTargetMachine.cpp
  SNESTargetObjectFile.cpp
)

add_dependencies(LLVMSNESCodeGen SNESCommonTableGen intrinsics_gen)
add_subdirectory(TargetInfo)
