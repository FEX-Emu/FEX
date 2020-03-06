#include <llvm-c/Core.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/InitializePasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>

#include <chrono>
#include <cstdio>

int main(int argc, char **argv) {
  printf("LLVM Test\n");
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  auto ContextRef = LLVMContextCreate();
  auto Con = *llvm::unwrap(&ContextRef);
  auto MainModule = new llvm::Module("Main Module", *Con);
  auto IRBuilder = new llvm::IRBuilder<>(*Con);

  using namespace llvm;

  Type *i64 = Type::getInt64Ty(*Con);
  auto FunctionType = FunctionType::get(Type::getVoidTy(*Con),
    {
      i64,
    }, false);


  legacy::PassManager PM;
  PassManagerBuilder PMBuilder;
  PMBuilder.OptLevel = 3;
  PMBuilder.populateModulePassManager(PM);

  std::string Empty;
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 10000; ++i)
  {

    auto Func = Function::Create(FunctionType,
        Function::ExternalLinkage,
        Empty,
        MainModule);
    Func->setCallingConv(CallingConv::C);

    {
      auto Entry = BasicBlock::Create(*Con, Empty, Func);
      IRBuilder->SetInsertPoint(Entry);

      auto ExitBlock = BasicBlock::Create(*Con, Empty, Func);

      IRBuilder->SetInsertPoint(ExitBlock);
      IRBuilder->CreateRetVoid();

      IRBuilder->SetInsertPoint(Entry);
      IRBuilder->CreateBr(ExitBlock);
    }

    //printf("i: %d\n", i);
    PM.run(*MainModule);

    auto FunctionList = &MainModule->getFunctionList();
    FunctionList->clear();
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = end - start;

  printf("Took %ld(%ldms) nanoseconds\n", diff.count(), std::chrono::duration_cast<std::chrono::milliseconds>(diff).count());
  return 0;
}
