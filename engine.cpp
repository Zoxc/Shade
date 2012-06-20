//===-- JITEmitter.cpp - Write machine code to executable memory ----------===//
//
//                     The LLVM Compiler Infrastructure
#include "engine.hpp"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Function.h"

using namespace llvm;

namespace Shade
{

Engine::Engine(Module *m, const TargetData &td) : ExecutionEngine(m)
{
	setTargetData(&td);
}

void *Engine::recompileAndRelinkFunction(llvm::Function *F)
{
    report_fatal_error("unsupported");
}

void Engine::freeMachineCodeForFunction(llvm::Function *F)
{
    report_fatal_error("unsupported");
}

llvm::GenericValue Engine::runFunction(Function *F, const std::vector<llvm::GenericValue> &ArgValues)
{
    report_fatal_error("can't run functions");
}

void *Engine::getPointerToBasicBlock(BasicBlock *BB)
{
    report_fatal_error("address-of-label not supported");
}

void *Engine::getPointerToFunction(Function *F)
{
  auto fresult = FunctionMap.find(F);

  if(fresult != FunctionMap.end())
	  return fresult->second;
  
  if (F->isDeclaration() || F->hasAvailableExternallyLinkage()) {
    bool AbortOnFailure = !F->hasExternalWeakLinkage();
    void *Addr = getPointerToNamedFunction(F->getName(), AbortOnFailure);
    return Addr;
  }

  report_fatal_error("Unknown function");
}

void *Engine::getPointerToNamedFunction(const std::string &Name, bool AbortOnFailure)
{
  if(AbortOnFailure)
	report_fatal_error("External function");

  return 0;
}
};