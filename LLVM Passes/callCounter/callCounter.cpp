/*
Developed by Ali Abyaneh, thanks to constructive examples in llvm tutur, DawnCC, and llvm.org
*/

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include <iostream>
#include <fstream>

using namespace llvm;
cl::opt<std::string> Filename(cl::Positional, cl::desc("<input file>"), cl::init("-"));

namespace{

  struct callCounter : public FunctionPass{
      static char ID;
      callCounter():FunctionPass(ID){}

      bool runOnFunction(Function &F) override{
        std::map<std::string, int> call_map;

        // errs() << F.getName() << "\n";
        for(Function::iterator bb = F.begin(), e = F.end(); bb!=e; bb++){
          // errs()<<"BasicBlock name =" << bb->getName() << "\n";
          // errs()<<"BasicBlock size =" << bb->size() << "\n\n";
          for(BasicBlock::iterator instruction = bb->begin(), iend = bb->end(); instruction!=iend; instruction++){

            if (CallInst *callInst = dyn_cast<CallInst>(instruction)){
              if (Function *calledFunction = callInst->getCalledFunction()){

                if(call_map.find((std::string)calledFunction->getName()) == call_map.end())
                  call_map[(std::string)calledFunction->getName()] = 1;
                else
                  call_map[(std::string)calledFunction->getName()]++;
                }
            }
          }
        }
        std::map<std::string,int>::iterator i = call_map.begin();
        std::map<std::string,int>::iterator e = call_map.end();
        std::ofstream ofs (Filename, std::ofstream::app);
        ofs << (std::string)F.getName() << "\n";

        while(i!=e){
          ofs << i->first << " ::: " << i->second << "\n";
          i++;
        }
        ofs << "#\n";
        ofs.close();
        call_map.clear();
        return false;
      }
  };
}

char callCounter::ID = 0;
static RegisterPass<callCounter> X("callCounter", "Count calls");
