/*
Developed by Ali Abyaneh, thanks to constructive examples in llvm tutur, DawnCC, and llvm.org
*/

#include "PLoopFinder.h"
#include <string>
#include <limits.h>
#include <unistd.h>

std::string getexepath()
{
  char result[ PATH_MAX ];
  ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
  return std::string( result, (count > 0) ? count : 0 );
}

using namespace llvm;
using namespace ParallelLoopInformation;

cl::opt<std::string> Filename(cl::Positional, cl::desc("<input file>"), cl::init("-"));

char PLoopFinder::ID = 0;
// static RegisterPass<PLoopFinder> X("PLoopFinder", "Hello world pass");

INITIALIZE_PASS_BEGIN(PLoopFinder, "PLoopFinder",
                      "Score Loops", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_DEPENDENCY(SCEVAAWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DependenceAnalysisWrapperPass)
INITIALIZE_PASS_END(PLoopFinder, "PLoopFinder",
                    "Score Loops", false, false)


bool PLoopFinder::runOnFunction(Function &F){
  llvm::DominatorTreeBase<BasicBlock, false>* DTB = new llvm::DominatorTreeBase<BasicBlock, false>();
  DTB->recalculate(F);
  llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop>* KLoop = new llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop>();
  KLoop->releaseMemory();
  KLoop->analyze(*DTB);
  DI = &getAnalysis<DependenceAnalysisWrapperPass>().getDI();
  LInfo = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  this->scalarEvolution= &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  // llvm::DependenceAnalysisWrapperPass &DA = getAnalysis<DependenceAnalysisWrapperPass>(F);
  this->dependenceInfo = DI;
  for(llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop>::iterator li = KLoop->begin(), liend = KLoop->end(); li!=liend; li++){
    PLoopInformation* loopInfo = new PLoopInformation();
    this->traverseLoopGraphFindDependencies(*li, loopInfo);
    this->loopInfo.push_back(loopInfo);
    // else
    //     delete loopInfo;

  }
  std::ofstream ofs (Filename, std::ofstream::app);
  ofs << (std::string)F.getName() << "\n";
  for (std::vector<PLoopInformation*>::iterator it = this->loopInfo.begin(); it != this->loopInfo.end(); it++) {
    ofs << "New Loop\n";
    (*it)->dump(ofs);
    ofs << "END Loop\n";
  }
  ofs << "#END\n";
  return false;
}

bool PLoopFinder::traverseLoopGraphFindDependencies(llvm::Loop *loop, PLoopInformation* loopInfo){

  // llvm::outs()  << loop->getLoopDepth() << "\n\n";
  loopInfo->setNestingLevel(loop->getLoopDepth());
  loopInfo->setIterations(this->getLoopIterations(loop));
  bool body_checked = false;
  for(llvm::Loop::block_iterator BB = loop->block_begin(), BBend = loop->block_end(); BB!=BBend; BB++){
    if((*BB)->getName().find("cond")!=std::string::npos){
      continue;
    }
    else if((*BB)->getName().find("body")!=std::string::npos && !body_checked){
      loopInfo->setLoopName(std::string((*BB)->getName()));
      body_checked = true;
      loopInfo->setComputationSize(this->computationScore((*BB)));
      if (this->findDataDependency((*BB)))
        continue;

    }
    else if((*BB)->getName().find("inc")!=std::string::npos){
      continue;
    }
  }

  const std::vector<llvm::Loop*> innerLoops =  loop->getSubLoops();
  for (auto it = innerLoops.begin() ; it != innerLoops.end(); ++it){
    PLoopInformation* innerLoopInfo = new PLoopInformation(loopInfo);
    loopInfo->addInnerLoop(innerLoopInfo);
    if(this->traverseLoopGraphFindDependencies(*it, innerLoopInfo)){
      innerLoopInfo->setDataDependent();
      return true;
    }
  }
  return false;
}

bool PLoopFinder::findDataDependency(llvm::BasicBlock* basicBlock){
  for(llvm::BasicBlock::iterator instruction_f = basicBlock->begin(), iend = basicBlock->end(); instruction_f!=iend; instruction_f++){
    for(auto instruction_s = instruction_f; ++instruction_s != iend; ){
      if((instruction_s->getOpcode() != llvm::Instruction::Load) ||
                                        (instruction_f->getOpcode()!=  Instruction::Load)){
        std::unique_ptr<llvm::Dependence> depends = this->dependenceInfo->depends(&(*instruction_f), &(*instruction_s), false);
        if(depends == nullptr)
          depends = this->dependenceInfo->depends(&(*instruction_s), &(*instruction_f), false);
        if(depends != nullptr){
          #ifdef DEBUG_PLOOPFINDER
            this->printDependencyInfo(&(*instruction_s), &(*instruction_f), &(*depends));
          #endif
          return true;
        }
        depends.release();
      }
    }
  }
  return false;
}

void PLoopFinder::printDependencyInfo(llvm::Instruction* i1, llvm::Instruction* i2, llvm::Dependence* depends){
  i1->print(errs());
  llvm::outs()<< "\n";
  i2->print(errs());
  llvm::outs()<< "\n";
  llvm::outs()  << "is input            "<< depends->isInput() <<"\n";
  llvm::outs()  << "is output           "<< depends->isOutput() <<"\n";
  llvm::outs()  << "is Flow             "<< depends->isFlow() <<"\n";
  llvm::outs()  << "is Anti             "<< depends->isAnti() <<"\n";
  llvm::outs()  << "is Ordered          "<< depends->isOrdered() <<"\n";
  llvm::outs()  << "is unOrdered        "<< depends->isUnordered() <<"\n";
  llvm::outs()  << "is Loop Independent "<< depends->isLoopIndependent() <<"\n";
  llvm::outs()  << "is Confused         "<< depends->isConfused() <<"\n";
  llvm::outs()  << "Level               "<< depends->getLevels() <<"\n";
  llvm::outs()  << "is Consistent       "<< depends->isConsistent() <<"\n\n";
}

int PLoopFinder::getLoopIterations(llvm::Loop *loop){

  return -1;
  // To Do later!!!
  llvm::Optional<llvm::Loop::LoopBounds> olb = loop->getBounds((*this->scalarEvolution));

  llvm::outs() << "!!!!!\n";

  ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  llvm::outs() << SE.getSmallConstantTripCount(loop) << " #####\n";
  if(!olb.hasValue())
    return -1;
  llvm::Loop::LoopBounds *lb = olb.getPointer();
  //Get the initial value of the loop induction variable.
  llvm::Value &initValue  = lb->getInitialIVValue();
  //	Get the step that the loop induction variable gets updated by in each loop iteration.
  llvm::Value* stepValue = lb->getStepValue();
  //Get the final value of the loop induction variable.
  llvm::Value &finalValue = lb->getFinalIVValue();
  initValue.print(llvm::outs());
  stepValue->print(llvm::outs());
  finalValue.print(llvm::outs());
// To be implemented later!
return 1;
}

double PLoopFinder::computationScore(llvm::BasicBlock* bb){
  int comp_instruction = 0;
  int total_instruction = 0;
  for(BasicBlock::iterator instruction = bb->begin(), iend = bb->end(); instruction!=iend; instruction++){
    total_instruction++;
    switch (instruction->getOpcode()) {
      // // Standard binary operators...
      case llvm::Instruction::Add : comp_instruction++; break;
      case llvm::Instruction::FAdd: comp_instruction+= FLOATINGPOINT;break;
      case llvm::Instruction::Sub : comp_instruction++;break;
      case llvm::Instruction::FSub: comp_instruction+= FLOATINGPOINT;break;
      case llvm::Instruction::Mul : comp_instruction+= MULTORDIV;break;
      case llvm::Instruction::FMul: comp_instruction+= FLOATINGPOINT*MULTORDIV;break;
      case llvm::Instruction::UDiv: comp_instruction+= MULTORDIV;break;
      case llvm::Instruction::SDiv: comp_instruction+= MULTORDIV;break;
      case llvm::Instruction::FDiv: comp_instruction+= FLOATINGPOINT*MULTORDIV;break;
      case llvm::Instruction::URem: comp_instruction+= MULTORDIV;break;
      case llvm::Instruction::SRem: comp_instruction+= MULTORDIV;break;
      case llvm::Instruction::FRem: comp_instruction+= FLOATINGPOINT*MULTORDIV;break;
      // // Logical operators...
      case llvm::Instruction::And: comp_instruction++;break;
      case llvm::Instruction::Or : comp_instruction++;break;
      case llvm::Instruction::Xor: comp_instruction++;break;
      default: break;
    }

  }
  return comp_instruction;
}
