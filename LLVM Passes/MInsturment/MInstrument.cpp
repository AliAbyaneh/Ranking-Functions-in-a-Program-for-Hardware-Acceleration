/*
The credit for the core of this instumentation pass goes to https://github.com/banach-space/llvm-tutor
Thanks to examples in https://github.com/banach-space/llvm-tutor, DawnCC, and llvm.org

*/


#include "MInstrument.h"
#include <string>
#include <limits.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

using namespace llvm;


Constant *CreateGlobalCounter(Module &M, StringRef GlobalVarName) {
  auto &CTX = M.getContext();

  // This will insert a declaration into M
  Constant *NewGlobalVar =
      M.getOrInsertGlobal(GlobalVarName, IntegerType::getInt32Ty(CTX));

  // This will change the declaration into definition (and initialise to 0)
  GlobalVariable *NewGV = M.getNamedGlobal(GlobalVarName);
  NewGV->setLinkage(GlobalValue::CommonLinkage);
  NewGV->setAlignment(4);
  NewGV->setInitializer(llvm::ConstantInt::get(CTX, APInt(32, 0)));

  return NewGlobalVar;
}

char MInstrument::ID = 0;
// static RegisterPass<MInstrument> X("MInstrument", "Hello world pass");

INITIALIZE_PASS_BEGIN(MInstrument, "MInstrument",
                      "Insturment the code", false, false)
INITIALIZE_PASS_DEPENDENCY(DependenceAnalysisWrapperPass)
INITIALIZE_PASS_END(MInstrument, "MInstrument",
                    "Insturment the code", false, false)


bool MInstrument::runOnModule(Module &M){

  llvm::StringMap<Constant *> CallCounterMap;
  llvm::StringMap<Constant *> FuncNameMap;
  llvm::LLVMContext &CTX = M.getContext();
  llvm::StringMap< llvm::StringMap<Constant *> > LoopNameMap;
  llvm::StringMap< llvm::StringMap<Constant *> > LoopCounterNameMap;

  for(auto &F : M){

    IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());

    std::string CounterName = "CounterFor_" + std::string(F.getName());
    Constant *Var = CreateGlobalCounter(M, CounterName);
    CallCounterMap[F.getName()] = Var;


    auto FuncName = Builder.CreateGlobalStringPtr(F.getName());
    FuncNameMap[F.getName()] = FuncName;


    LoadInst *Load2 = Builder.CreateLoad(Var);
    Value *Inc2 = Builder.CreateAdd(Builder.getInt32(1), Load2);
    Builder.CreateStore(Inc2, Var);
    llvm::DominatorTreeBase<BasicBlock, false>* DTB = new llvm::DominatorTreeBase<BasicBlock, false>();
    DTB->recalculate(F);
    llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop>* KLoop = new llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop>();
    KLoop->releaseMemory();
    KLoop->analyze(*DTB);
    llvm::StringMap<Constant *> LN;
    llvm::StringMap<Constant *> LCN;
    for(llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop>::iterator li = KLoop->begin(), liend = KLoop->end(); li!=liend; li++){

      instrumentLoop(*li, LN, LCN, std::string(F.getName()), M);
    }
    LoopNameMap[F.getName()] = LN;
    LoopCounterNameMap[F.getName()] = LCN;
  }

  PointerType *PrintfArgTy = PointerType::getUnqual(Type::getInt8Ty(CTX));
  FunctionType *PrintfTy =
      FunctionType::get(IntegerType::getInt32Ty(CTX), PrintfArgTy,
                        /*IsVarArgs=*/true);

  FunctionCallee Printf = M.getOrInsertFunction("printf", PrintfTy);


  Function *PrintfF = dyn_cast<Function>(Printf.getCallee());
  PrintfF->setDoesNotThrow();
  PrintfF->addParamAttr(0, Attribute::NoCapture);
  PrintfF->addParamAttr(0, Attribute::ReadOnly);


  llvm::Constant *ResultFormatStr =
      llvm::ConstantDataArray::getString(CTX, "%-20s %-10lu\n");

  Constant *ResultFormatStrVar =
      M.getOrInsertGlobal("ResultFormatStrIR", ResultFormatStr->getType());
  dyn_cast<GlobalVariable>(ResultFormatStrVar)->setInitializer(ResultFormatStr);


  std::string out = "";

  llvm::Constant *ResultHeaderStr =
      llvm::ConstantDataArray::getString(CTX, out.c_str());

  Constant *ResultHeaderStrVar =
      M.getOrInsertGlobal("ResultHeaderStrIR", ResultHeaderStr->getType());
  dyn_cast<GlobalVariable>(ResultHeaderStrVar)->setInitializer(ResultHeaderStr);

  FunctionType *PrintfWrapperTy =
      FunctionType::get(llvm::Type::getVoidTy(CTX), {},
                        /*IsVarArgs=*/false);
  Function *PrintfWrapperF = dyn_cast<Function>(
      M.getOrInsertFunction("printf_wrapper", PrintfWrapperTy).getCallee());

  // Create the entry basic block for printf_wrapper ...
  llvm::BasicBlock *RetBlock =
      llvm::BasicBlock::Create(CTX, "enter", PrintfWrapperF);
  IRBuilder<> Builder(RetBlock);

  llvm::Value *ResultHeaderStrPtr =
      Builder.CreatePointerCast(ResultHeaderStrVar, PrintfArgTy);
  llvm::Value *ResultFormatStrPtr =
      Builder.CreatePointerCast(ResultFormatStrVar, PrintfArgTy);

  Builder.CreateCall(Printf, {ResultHeaderStrPtr});

  LoadInst *LoadCounter;
  for (auto &item : CallCounterMap) {
    LoadCounter = Builder.CreateLoad(item.second);
    Builder.CreateCall(
        Printf, {ResultFormatStrPtr, FuncNameMap[item.first()], LoadCounter});
    for(auto &item2: LoopCounterNameMap[item.first()])
    {
      LoadCounter = Builder.CreateLoad(item2.second);
      Builder.CreateCall(
          Printf, {ResultFormatStrPtr, LoopNameMap[item.first()][item2.first()], LoadCounter});

    }
  }

  Builder.CreateRetVoid();


  appendToGlobalDtors(M, PrintfWrapperF, /*Priority=*/0);

  return true;

 return true;
}

void MInstrument::instrumentLoop(llvm::Loop *loop, llvm::StringMap<Constant *> &LoopNameMap, llvm::StringMap<Constant *> &LoopCounterNameMap, std::string FNAME, Module &M){
  for(llvm::Loop::block_iterator BB = loop->block_begin(), BBend = loop->block_end(); BB!=BBend; BB++){
    if((*BB)->getName().find("body")!=std::string::npos){
      IRBuilder<> Builder(&*(*BB)->getFirstInsertionPt());
      std::string CounterName = "CounterFor_" + FNAME + "_" + std::string((*BB)->getName());
      Constant *Var = CreateGlobalCounter(M, CounterName);

      // Create a global variable to hold the name of this function
      auto LoopName = Builder.CreateGlobalStringPtr(FNAME + ":" + std::string((*BB)->getName()));
      LoopCounterNameMap[FNAME + ":" + std::string((*BB)->getName())] = Var;
      LoopNameMap[FNAME + ":" + std::string((*BB)->getName())] = LoopName;
      LoadInst *Load2 = Builder.CreateLoad(Var);
      Value *Inc2 = Builder.CreateAdd(Builder.getInt32(1), Load2);
      Builder.CreateStore(Inc2, Var);

    }

  }

  const std::vector<llvm::Loop*> innerLoops =  loop->getSubLoops();
  for (auto it = innerLoops.begin() ; it != innerLoops.end(); ++it){
    instrumentLoop(*it, LoopNameMap, LoopCounterNameMap, FNAME, M);
  }
}
