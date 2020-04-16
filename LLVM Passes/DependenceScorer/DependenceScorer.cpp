/*
Developed by Ali Abyaneh, thanks to constructive examples in llvm tutur, DawnCC, and llvm.org
*/

#include "DependenceScorer.h"
#include <string>
#include <limits.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

using namespace llvm;

cl::opt<std::string> Filename(cl::Positional, cl::desc("<input file>"), cl::init("-"));

char DependenceScorer::ID = 0;
// static RegisterPass<DependenceScorer> X("DependenceScorer", "Hello world pass");

INITIALIZE_PASS_BEGIN(DependenceScorer, "DependenceScorer",
                      "Score Dependencies", false, false)
INITIALIZE_PASS_DEPENDENCY(DependenceAnalysisWrapperPass)
INITIALIZE_PASS_END(DependenceScorer, "DependenceScorer",
                    "Score Dependencies", false, false)


bool DependenceScorer::runOnFunction(Function &F){
  this->dependenceInfo = &getAnalysis<DependenceAnalysisWrapperPass>().getDI();
  int computationValue = 0;
  int dataDependencyCounter = 0;
  for(Function::iterator bb = F.begin(), e = F.end(); bb!=e; bb++){
    // errs()<<"BasicBlock name =" << bb->getName() << "\n";
    // errs()<<"BasicBlock size =" << bb->size() << "\n\n";
    computationValue += this->computationScore(&(*bb));
    dataDependencyCounter += this->findDataDependency(&(*bb));
  }
  std::ofstream ofs (Filename, std::ofstream::app);
  ofs << (std::string)F.getName() << "\n";
  ofs << "computation value : " << computationValue << "\n";
  ofs << "data Dependency : " << dataDependencyCounter << "\n";
  ofs << "#\n";
  ofs.close();
  return false;
}

double DependenceScorer::computationScore(llvm::BasicBlock* bb){
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



int DependenceScorer::findDataDependency(llvm::BasicBlock* basicBlock){
  int dataDependencyCounter = 0;
  for(llvm::BasicBlock::iterator instruction_f = basicBlock->begin(), iend = basicBlock->end(); instruction_f!=iend; instruction_f++){
    for(auto instruction_s = instruction_f; ++instruction_s != iend; ){
        std::unique_ptr<llvm::Dependence> depends = this->dependenceInfo->depends(&(*instruction_f), &(*instruction_s), false);
        if(depends == nullptr)
          depends = this->dependenceInfo->depends(&(*instruction_s), &(*instruction_f), false);
        if(depends != nullptr){
          if(depends->isInput() || depends->isOutput() || depends->isAnti() || depends->isConfused())
            dataDependencyCounter++;
        }
        depends.release();
      }
    }
  return dataDependencyCounter;
}
