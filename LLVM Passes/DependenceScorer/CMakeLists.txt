
add_llvm_library( LLVMDependenceScorer MODULE BUILDTREE_ONLY
  DependenceScorer.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
