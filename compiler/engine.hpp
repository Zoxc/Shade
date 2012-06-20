#include "../shade.hpp"

namespace llvm
{
	class TargetMachine;
	class TargetData;
}

#include <vector>
#include "llvm/CodeGen/JITCodeEmitter.h"
#include "llvm/CodeGen/MachineRelocation.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/ValueMap.h"

namespace Shade
{
  class Engine : public llvm::ExecutionEngine {
public:
	static std::vector<std::string> modules;

	Engine(llvm::Module *m, const llvm::TargetData &td);
	~Engine() {}

  /// @name ExecutionEngine interface implementation
  /// @{

  llvm::DenseMap<const llvm::Function*, void *> FunctionMap;
  
  /// runFunction - Execute the specified function with the specified arguments,
  /// and return the result.
  virtual llvm::GenericValue runFunction(llvm::Function *F, const std::vector<llvm::GenericValue> &ArgValues);
  
  /// recompileAndRelinkFunction - This method is used to force a function which
  /// has already been compiled to be compiled again, possibly after it has been
  /// modified.  Then the entry to the old copy is overwritten with a branch to
  /// the new copy.  If there was no old copy, this acts just like
  /// VM::getPointerToFunction().
  virtual void *recompileAndRelinkFunction(llvm::Function *F);

  /// freeMachineCodeForFunction - Release memory in the ExecutionEngine
  /// corresponding to the machine code emitted to execute this function, useful
  /// for garbage-collecting generated code.
  virtual void freeMachineCodeForFunction(llvm::Function *F);

  virtual void *getPointerToBasicBlock(llvm::BasicBlock *BB);

  virtual void *getPointerToFunction(llvm::Function *F);

  /// getPointerToNamedFunction - This method returns the address of the
  /// specified function by using the dlsym function call.  As such it is only
  /// useful for resolving library symbols, not code generated symbols.
  ///
  /// If AbortOnFailure is false and no function with the given name is
  /// found, this function silently returns a null pointer. Otherwise,
  /// it prints a message to stderr and aborts.
  ///
  virtual void *getPointerToNamedFunction(const std::string &Name,
                                          bool AbortOnFailure = true);
};
};

