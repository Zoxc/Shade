#pragma once
#pragma warning(disable:4996)
#pragma warning(disable:4355)

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define D3C_EXPORTS
#include "d3c.h"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITMemoryManager.h>
#include <llvm/Instructions.h>
#include <llvm/Support/system_error.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Debug.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/PassManager.h>
#include <llvm/Target/TargetData.h>
#include <llvm/CodeGen/JITCodeEmitter.h>


d3c_error_t shade_error(d3c_error_type_t error, const char *message);

#define LLVM_ERROR(expr) do { auto error = (expr); if(error.value()) throw shade_error(D3C_LLVM, error.message().c_str()); } while(0)

using namespace llvm;

extern LLVMContext context;