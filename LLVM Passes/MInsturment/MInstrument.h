#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#define FLOATINGPOINT 2
#define MULTORDIV 4

using namespace llvm;


namespace {
  struct MInstrument : public ModulePass{
      static char ID;
      llvm::DependenceInfo* dependenceInfo;
      bool runOnModule(Module &M) override;
      MInstrument():ModulePass(ID){}
      void getAnalysisUsage(AnalysisUsage &AU) const override {
        AU.addRequiredTransitive<LoopInfoWrapperPass>();
        AU.setPreservesAll();
      }
      void instrumentLoop(llvm::Loop *loop, llvm::StringMap<Constant *> &LoopNameMap, llvm::StringMap<Constant *> &LoopCounterNameMap, std::string FNAME, Module &M);
  };

}
namespace llvm {
class PassRegistry;
void initializeMInstrumentPass(llvm::PassRegistry &);
}

namespace {
class MInstrumentInitializer {
public:
  MInstrumentInitializer() {
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
    llvm::initializeMInstrumentPass(Registry);

  }
};
static MInstrumentInitializer MInstrumentInit;
}
