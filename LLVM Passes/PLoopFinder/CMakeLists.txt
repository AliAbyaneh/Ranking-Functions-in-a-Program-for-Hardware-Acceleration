
add_llvm_library( LLVMPLoopFinder MODULE BUILDTREE_ONLY
  PLoopFinder.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
