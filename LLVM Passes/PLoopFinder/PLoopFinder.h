#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
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
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include <iostream>
#include <fstream>

// #define DEBUG_PLOOPFINDER
#define FLOATINGPOINT 2
#define MULTORDIV 4

using namespace llvm;

typedef std::vector<Loop*> LoopNest;
typedef std::vector<const Value*> LoopNestBounds;


namespace ParallelLoopInformation{
  class PLoopInformation;
  struct PLoopFinder : public FunctionPass{
      static char ID;
      llvm::DependenceInfo* dependenceInfo;
      llvm::ScalarEvolution* scalarEvolution;
      std::vector<PLoopInformation*> loopInfo;
      llvm::DependenceInfo *DI;
      llvm::LoopInfo *LInfo;

      bool traverseLoopGraphFindDependencies(llvm::Loop*, PLoopInformation*);
      bool findDataDependency(llvm::BasicBlock*);
      void printDependencyInfo(llvm::Instruction*, llvm::Instruction*, llvm::Dependence*);
      PLoopFinder():FunctionPass(ID){}
      int getLoopIterations(llvm::Loop *loop);
      double computationScore(llvm::BasicBlock*);
      bool runOnFunction(Function &F) override;
      //-da(dependency analysis) is an analysis pass, analysis passes doesn't change bitcode,
      //they just gather information to be used by the other analysis or transformation passes.
      // So if you want to use its information, you should write a new transform and add "DependenceAnalysis"
      //as a "RequiredAnalysis" to a virtual function named "getAnalysisUsage" inside your pass,
      // then you will have "DependenceAnalysis" as an abject.
      void getAnalysisUsage(AnalysisUsage &AU) const override {
        AU.addRequiredTransitive<llvm::DependenceAnalysisWrapperPass>();
        AU.addRequired<DominatorTreeWrapperPass>();
        AU.addRequiredTransitive<LoopInfoWrapperPass>();
        AU.addRequired<ScalarEvolutionWrapperPass>();
        AU.addRequired<SCEVAAWrapperPass>();
        AU.addPreservedID(LCSSAID);
        AU.setPreservesAll();
      }
  };
  class PLoopInformation{
    private:
      int nestingLevel;
      static const int NonAvailable = -1;
      int iterations = this->NonAvailable;
      bool controlDependent = false;
      int computationSize = 0;
      bool dataDependent = false;
      PLoopInformation* outerLoop = nullptr;
      std::vector<PLoopInformation*> innerLoops;
      std::string LoopName;
    public:
      PLoopInformation(){};
      PLoopInformation(PLoopInformation* OL){
        this->outerLoop = OL;
      }
      void dump(std::ofstream &ofs){
        ofs << LoopName << "\n";
        ofs << nestingLevel << "\n";
        ofs << computationSize << "\n";
        ofs << dataDependent << "\n";
        for (size_t i = 0; i < innerLoops.size(); i++) {
          // ofs << "New Inner Loop(s)\n";
          innerLoops[i]->dump(ofs);
          // ofs << "End Inner Loop(s)\n";
        }

      }
      void setDataDependent(){
        this->dataDependent = true;
        if(this->outerLoop != nullptr)
          this->outerLoop->setComputationSize(computationSize);
      }
      void setComputationSize(int s){
        this->computationSize += s;
      }
      void setNestingLevel(int n){
        this->nestingLevel = n;
        if(this->outerLoop != nullptr)
          this->outerLoop->setNestingLevel(n);
      }
      void setControlDependent(){
        this->controlDependent = true;
      }
      void setIterations(int i){
        this->iterations = i;
      }
      bool isConfused(){
        return this->dataDependent;
      }
      void addInnerLoop(PLoopInformation* l){
        this->innerLoops.push_back(l);
      }
      void setLoopName(std::string n){
        LoopName = n;
      }
  };
}
namespace llvm {
  class PassRegistry;
  void initializePLoopFinderPass(llvm::PassRegistry &);
}

namespace {
  class PLoopFinderInitializer {
public:
  PLoopFinderInitializer() {
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
    llvm::initializePLoopFinderPass(Registry);
  }
};
  static PLoopFinderInitializer PLoopFinderInit;
}
