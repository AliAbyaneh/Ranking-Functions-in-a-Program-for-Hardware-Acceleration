
add_llvm_library( LLVMMInstrument MODULE BUILDTREE_ONLY
  MInstrument.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
