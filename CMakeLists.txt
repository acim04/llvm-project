add_llvm_library(LLVMMyPasses MODULE
  LoopUnswitching.cpp
  LoopPeeling.cpp
  LoopPeeling.h

  PLUGIN_TOOL
  opt
)