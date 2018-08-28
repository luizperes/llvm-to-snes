add_llvm_library(LLVMSNESDesc
  SNESAsmBackend.cpp
  SNESELFObjectWriter.cpp
  SNESELFStreamer.cpp
  SNESMCAsmInfo.cpp
  SNESMCCodeEmitter.cpp
  SNESMCExpr.cpp
  SNESMCTargetDesc.cpp
  SNESTargetStreamer.cpp
)

add_dependencies(LLVMSNESDesc SNESCommonTableGen)

