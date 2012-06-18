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
#include <llvm/Instructions.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/IRBuilder.h>

using namespace llvm;

extern LLVMContext context;