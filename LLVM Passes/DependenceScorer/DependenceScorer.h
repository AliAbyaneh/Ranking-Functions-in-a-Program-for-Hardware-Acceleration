#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/DependenceAnalysis.h"

#define FLOATINGPOINT 2
#define MULTORDIV 4

using namespace llvm;


namespace {
  struct DependenceScorer : public FunctionPass{
      static char ID;
      llvm::DependenceInfo* dependenceInfo;
      bool runOnFunction(Function &F) override;
      DependenceScorer():FunctionPass(ID){}
      //-da(dependency analysis) is an analysis pass, analysis passes doesn't change bitcode,
      //they just gather information to be used by the other analysis or transformation passes.
      // So if you want to use its information, you should write a new transform and add "DependenceAnalysis"
      //as a "RequiredAnalysis" to a virtual function named "getAnalysisUsage" inside your pass,
      // then you will have "DependenceAnalysis" as an abject.
      void getAnalysisUsage(AnalysisUsage &AU) const override {
        AU.addRequiredTransitive<llvm::DependenceAnalysisWrapperPass>();
        AU.setPreservesAll();
      }
      double computationScore(llvm::BasicBlock* bb);
      int findDataDependency(llvm::BasicBlock* basicBlock);
  };

}
namespace llvm {
class PassRegistry;
void initializeDependenceScorerPass(llvm::PassRegistry &);
}

namespace {
class DependenceScorerInitializer {
public:
  DependenceScorerInitializer() {
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
    llvm::initializeDependenceScorerPass(Registry);
  }
};
static DependenceScorerInitializer DependenceScorerInit;
}
