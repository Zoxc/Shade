//===-- JITEmitter.cpp - Write machine code to executable memory ----------===//
//
//                     The LLVM Compiler Infrastructure
#include "engine.hpp"

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Function.h"
#include "llvm/Module.h"

using namespace llvm;

namespace Shade
{

Engine::Engine(Module *m, const TargetData &td) : ExecutionEngine(m), module(m)
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

  error((StringRef("Unable to find function: ") + F->getName()).str());
}

void *Engine::getPointerToFunction(const std::string &Name)
{
	auto f = module->getFunction(Name);

	if(!f)
		goto throw_error;

	return getPointerToFunction(f);

throw_error:
	 error("Unable to find function: " + Name);
}

std::vector<std::string> Engine::modules;

void *Engine::getPointerToNamedFunction(const std::string &Name, bool AbortOnFailure)
{
	for(auto i = modules.begin(); i != modules.end(); ++i)
	{
		HMODULE module = GetModuleHandleA(i->c_str());

		if(!module)
			win32_error("Unable to get module handle of '" + *i + "'");

		void *result = GetProcAddress(module, Name.c_str());

		if(result)
			return result;
	}
  if(AbortOnFailure)
	  error("Linking error: Unknown external function '" + Name + "'");

  return 0;
}
};