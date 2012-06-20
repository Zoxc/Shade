//===-- JITEmitter.cpp - Write machine code to executable memory ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a MachineCodeEmitter object that is used by the JIT to
// write machine code to memory and remember where relocatable values are.
//
//===----------------------------------------------------------------------===//

#include "emitter.hpp"
#include "engine.hpp"
#include "disassembler.hpp"
#include "remote-heap.hpp"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineCodeInfo.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/ExecutionEngine/JITMemoryManager.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetJITInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MutexGuard.h"
#include "llvm/Support/ValueHandle.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Disassembler.h"
#include "llvm/Support/Memory.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ValueMap.h"
#include <algorithm>
#ifndef NDEBUG
#include <iomanip>
#endif

using namespace llvm;

// A declaration may stop being a declaration once it's fully read from bitcode.
// This function returns true if F is fully read and is still a declaration.
static bool isNonGhostDeclaration(const Function *F) {
  return F->isDeclaration() && !F->isMaterializable();
}

namespace Shade
{
Emitter::Emitter(Engine &engine, llvm::TargetMachine &TM, RemoteHeap &code_section, RemoteHeap &data_section)
	: SizeEstimate(0), code_size(0), engine(engine), TM(TM), code_section(code_section), data_section(data_section), TD(*TM.getTargetData()),
    EmittedFunctions(this) {
}
void Emitter::addRelocation(const MachineRelocation &MR) {
    CurrentCode->Relocations.push_back(MR);
}

void Emitter::StartMachineBasicBlock(MachineBasicBlock *MBB) {
	MBB->getBasicBlock();
    if (CurrentCode->MBBLocations.size() <= (unsigned)MBB->getNumber())
    CurrentCode->MBBLocations.resize((MBB->getNumber()+1)*2);
    CurrentCode->MBBLocations[MBB->getNumber()] = getCurrentPCValue();

    DEBUG(dbgs() << "JIT: Emitting BB" << MBB->getNumber() << " at ["
                << (void*) getCurrentPCValue() << "]\n");
}

uintptr_t Emitter::getMachineBasicBlockAddress(int index) const{
    assert(CurrentCode->MBBLocations.size() > (unsigned)index &&
            CurrentCode->MBBLocations[index] && "MBB not emitted!");
    assert(CurrentCode->Target && "Target not emitted!");

    return (uintptr_t)CurrentCode->Target + CurrentCode->MBBLocations[index] - (uintptr_t)CurrentCode->AlignedStart;
}

uintptr_t Emitter::getMachineBasicBlockAddress(MachineBasicBlock *MBB) const{
	return getMachineBasicBlockAddress(MBB->getNumber());
}

void Emitter::emitLabel(MCSymbol *Label) {
    LabelLocations[Label] = getCurrentPCValue();
}

DenseMap<MCSymbol*, uintptr_t> *Emitter::getLabelLocations() {
    return &LabelLocations;
}

uintptr_t Emitter::getLabelAddress(MCSymbol *Label) const {
    assert(LabelLocations.count(Label) && "Label not emitted!");
    return LabelLocations.find(Label)->second;
}

void *Emitter::getGlobalVariableAddress(const GlobalVariable *V)
{
	auto result = GlobalOffsets.find(V);

	if(result != GlobalOffsets.end())
		return (void *)result->second;
	
	// If the global is external, just remember the address.
	if (V->isDeclaration() || V->hasAvailableExternallyLinkage()) {
		report_fatal_error("Could not resolve external global address: "
						+ V->getName());
		return 0;
	}

	Type *GlobalType = V->getType()->getElementType();
	
	size_t S = TD.getTypeAllocSize(GlobalType);
	size_t A = TD.getPreferredAlignment(V);

	void *remote = data_section.allocate(S, NextPowerOf2(A - 1));
	void *local = alloca(S);

	memset(local, 0, S);
	
	if (!V->isThreadLocal())
		engine.InitializeMemory(V->getInitializer(), local);

	write(remote, local, S);

	GlobalOffsets[V] = remote;

	return remote;
}

void *Emitter::getGlobalAddress(const GlobalValue *V)
{
	auto result = GlobalOffsets.find(V);

	if(result != GlobalOffsets.end())
		return result->second;

	report_fatal_error("Global hasn't had an address allocated yet!");
}

//===----------------------------------------------------------------------===//
// JITEmitter code.
//
void *Emitter::getPointerToGlobal(GlobalValue *V, void *Reference,
                                     bool MayNeedFarStub) {
  if (GlobalVariable *GV = dyn_cast<GlobalVariable>(V))
    return getGlobalVariableAddress(GV);

  if (GlobalAlias *GA = dyn_cast<GlobalAlias>(V))
    return getGlobalAddress(GA->resolveAliasedGlobal(false));

  // If we have already compiled the function, return a pointer to its body.
  Function *F = cast<Function>(V);

  auto fresult = EmittedFunctions.find(F);

  if(fresult != EmittedFunctions.end())
	  return fresult->second.Target;

  return engine.getPointerToFunction(F);
}

void *Emitter::getGlobalValueIndirectSym(GlobalValue *GV, void *GVAddress) {
  // If we already have a stub for this global variable, recycle it.
	auto gresult = IndirectSymMap.find(GV);

	if(gresult != IndirectSymMap.end())
		return gresult->second;

	size_t size = sizeof(void *);
	
	void *remote = data_section.allocate(size, size);
	void **local = (void **)alloca(size);

	*local = GVAddress;

	write(remote, local, size);

	IndirectSymMap[GV] = remote;

	return remote;
}

void *Emitter::getPointerToGVIndirectSym(GlobalValue *V, void *Reference) {
  // Make sure GV is emitted first, and create a stub containing the fully
  // resolved address.
  void *GVAddress = getPointerToGlobal(V, Reference, false);
  void *StubAddr = getGlobalValueIndirectSym(V, GVAddress);
  return StubAddr;
}

void Emitter::processDebugLoc(DebugLoc DL, bool BeforePrintingInsn) {
}

static unsigned GetConstantPoolSizeInBytes(MachineConstantPool *MCP,
                                           const TargetData *TD) {
  const std::vector<MachineConstantPoolEntry> &Constants = MCP->getConstants();
  if (Constants.empty()) return 0;

  unsigned Size = 0;
  for (unsigned i = 0, e = Constants.size(); i != e; ++i) {
    MachineConstantPoolEntry CPE = Constants[i];
    unsigned AlignMask = CPE.getAlignment() - 1;
    Size = (Size + AlignMask) & ~AlignMask;
    Type *Ty = CPE.getType();
    Size += TD->getTypeAllocSize(Ty);
  }
  return Size;
}

void Emitter::startFunction(MachineFunction &F) {
  DEBUG(dbgs() << "JIT: Starting CodeGen of Function "
        << F.getFunction()->getName() << "\n");

  uintptr_t ActualSize = 0;

  if (SizeEstimate > 0) {
    // SizeEstimate will be non-zero on reallocation attempts.
    ActualSize = SizeEstimate;
  }

  BufferBegin = CurBufferPtr = startFunctionBody(F.getFunction(), ActualSize);
  BufferEnd = BufferBegin+ActualSize;

  EmittedCode &code = EmittedFunctions[F.getFunction()];
  code.Function = F.getFunction();

  CurrentCode = &code;

  code.FunctionBody = BufferBegin;

  // Ensure the constant pool/jump table info is at least 4-byte aligned.
  emitAlignment(16);
  
  code.AlignedStart = CurBufferPtr;

  emitConstantPool(F.getConstantPool());

  if (MachineJumpTableInfo *MJTI = F.getJumpTableInfo())
    initJumpTableInfo(MJTI);

  // About to start emitting the machine code for the function.
  emitAlignment(std::max(F.getFunction()->getAlignment(), 8U));
  code.Code = CurBufferPtr;
}

bool Emitter::finishFunction(MachineFunction &F) {
  if (CurBufferPtr == BufferEnd) {
    // We must call endFunctionBody before retrying, because
    // deallocateMemForFunction requires it.
    endFunctionBody(F.getFunction(), BufferBegin, CurBufferPtr);
    retryWithMoreMemory(F);
    return true;
  }

  endFunctionBody(F.getFunction(), BufferBegin, CurBufferPtr);

	// Now that we've succeeded in emitting the function, reset the
	// SizeEstimate back down to zero.
	SizeEstimate = 0;
  
  CurrentCode->End = CurBufferPtr;
  CurrentCode->Size = CurBufferPtr - (uint8_t *)CurrentCode->AlignedStart;
  CurrentCode->Target = code_section.allocate(CurrentCode->Size, 16);

  engine.FunctionMap[CurrentCode->Function] = (void *)((uintptr_t)CurrentCode->Target + (uintptr_t)CurrentCode->Code - (uintptr_t)CurrentCode->AlignedStart);

  BufferBegin = CurBufferPtr = 0;
  
  if (MachineJumpTableInfo *MJTI = F.getJumpTableInfo())
    emitJumpTableInfo(MJTI);

  DEBUG(dbgs() << "JIT: Finished CodeGen of [" << (void*)CurrentCode->Code
        << "] Function: " << F.getFunction()->getName()
		<< ": " << (CurrentCode->Size) << " bytes of text, "
        << CurrentCode->Relocations.size() << " relocations\n");

  ConstPoolAddresses.clear();

	for (unsigned i = 0, e = CurrentCode->Relocations.size(); i != e; ++i)
	{
		MachineRelocation &MR = CurrentCode->Relocations[i];

		if(MR.isBasicBlock())
			CurrentCode->Relocations[i] = MachineRelocation::getBB(MR.getMachineCodeOffset(), MR.getRelocationType(), (llvm::MachineBasicBlock *)MR.getBasicBlock()->getNumber(), MR.getConstantVal());
	}

  return false;
}

void Emitter::resolveRelocations()
{
	for(auto code = EmittedFunctions.begin(); code != EmittedFunctions.end(); ++code)
	{
		CurrentCode = &code->second;

		if (!CurrentCode->Relocations.empty()) {
		// Resolve the relocations to concrete pointers.
		for (unsigned i = 0, e = CurrentCode->Relocations.size(); i != e; ++i) {
		  MachineRelocation &MR = CurrentCode->Relocations[i];
		  void *ResultPtr = 0;
		  if (!MR.letTargetResolve()) {
			if (MR.isExternalSymbol()) {
				abort();
			  ResultPtr = engine.getPointerToNamedFunction(MR.getExternalSymbol(), false);
			  DEBUG(dbgs() << "JIT: Map \'" << MR.getExternalSymbol() << "\' to ["
						   << ResultPtr << "]\n");
			} else if (MR.isGlobalValue()) {
			  ResultPtr = getPointerToGlobal(MR.getGlobalValue(),
											 BufferBegin+MR.getMachineCodeOffset(),
											 MR.mayNeedFarStub());
			} else if (MR.isIndirectSymbol()) {
			  ResultPtr = getPointerToGVIndirectSym(
				  MR.getGlobalValue(), BufferBegin+MR.getMachineCodeOffset());
			} else if (MR.isBasicBlock()) {
			  ResultPtr = (void*)getMachineBasicBlockAddress((int)MR.getBasicBlock());
			} else if (MR.isConstantPoolIndex()) {
			  ResultPtr =
				(void*)getConstantPoolEntryAddress(MR.getConstantPoolIndex());
			} else {
			  assert(MR.isJumpTableIndex());
			  ResultPtr=(void*)getJumpTableEntryAddress(MR.getJumpTableIndex());
			}

			if(MR.getRelocationType() == 0) // X86::reloc_pcrel_word
			{
				uintptr_t offset = (uintptr_t)CurrentCode->AlignedStart - (uintptr_t)CurrentCode->FunctionBody;
				uintptr_t fake_remote = (uintptr_t)CurrentCode->Target - offset;
				ResultPtr = (void *)((uintptr_t)ResultPtr + (uintptr_t)CurrentCode->FunctionBody - fake_remote);
			}

			MR.setResultPointer(ResultPtr);
		  }
		}

		TM.getJITInfo()->relocate(CurrentCode->FunctionBody, &CurrentCode->Relocations[0], CurrentCode->Relocations.size(), nullptr);
	  }

		Shade::disassemble_code(CurrentCode->Code, (uint8_t *)CurrentCode->Target + ((uint8_t *)CurrentCode->Code - (uint8_t *)CurrentCode->AlignedStart), (uint8_t *)CurrentCode->End - (uint8_t *)CurrentCode->Code);
		write(CurrentCode->Target, CurrentCode->AlignedStart, CurrentCode->Size);
	}
}

void Emitter::retryWithMoreMemory(MachineFunction &F) {
  DEBUG(dbgs() << "JIT: Ran out of space for native code.  Reattempting.\n");
  ConstPoolAddresses.clear();
  deallocateMemForFunction(F.getFunction());
  // Try again with at least twice as much free space.
  SizeEstimate = (uintptr_t)(2 * (BufferEnd - BufferBegin));
}

/// deallocateMemForFunction - Deallocate all memory for the specified
/// function body.  Also drop any references the function has to stubs.
/// May be called while the Function is being destroyed inside ~Value().
void Emitter::deallocateMemForFunction(const Function *F) {
  ValueMap<const Function *, EmittedCode, EmittedFunctionConfig>::iterator
    Emitted = EmittedFunctions.find(F);
  if (Emitted != EmittedFunctions.end()) {
    deallocateFunctionBody(Emitted->second.FunctionBody);

    EmittedFunctions.erase(Emitted);
  }
}


void *Emitter::allocateSpace(uintptr_t Size, unsigned Alignment) {
  if (BufferBegin)
    return JITCodeEmitter::allocateSpace(Size, Alignment);

  // create a new memory block if there is no active one.
  // care must be taken so that BufferBegin is invalidated when a
  // block is trimmed
  BufferBegin = CurBufferPtr = (uint8_t *)malloc(Size + Alignment);
  BufferEnd = BufferBegin + Size;
  emitAlignment(Alignment);
  return CurBufferPtr;
}

void Emitter::emitConstantPool(MachineConstantPool *MCP) {
  const std::vector<MachineConstantPoolEntry> &Constants = MCP->getConstants();
  if (Constants.empty()) return;

  unsigned Size = GetConstantPoolSizeInBytes(MCP, &TD);
  unsigned Align = MCP->getConstantPoolAlignment();
  ConstantPoolBase = allocateSpace(Size, Align);
  ConstantPool = MCP;

  if (ConstantPoolBase == 0) return;  // Buffer overflow.

  DEBUG(dbgs() << "JIT: Emitted constant pool at [" << ConstantPoolBase
               << "] (size: " << Size << ", alignment: " << Align << ")\n");

  // Initialize the memory for all of the constant pool entries.
  unsigned Offset = 0;
  for (unsigned i = 0, e = Constants.size(); i != e; ++i) {
    MachineConstantPoolEntry CPE = Constants[i];
    unsigned AlignMask = CPE.getAlignment() - 1;
    Offset = (Offset + AlignMask) & ~AlignMask;

    uintptr_t CAddr = (uintptr_t)ConstantPoolBase + Offset;
    ConstPoolAddresses.push_back(CAddr);
    if (CPE.isMachineConstantPoolEntry()) {
      // FIXME: add support to lower machine constant pool values into bytes!
      report_fatal_error("Initialize memory with machine specific constant pool"
                        "entry has not been implemented!");
    }
    //TheJIT->InitializeMemory(CPE.Val.ConstVal, (void*)CAddr);
    DEBUG(dbgs() << "JIT:   CP" << i << " at [0x";
          dbgs().write_hex(CAddr) << "]\n");

    Type *Ty = CPE.Val.ConstVal->getType();
    Offset += TD.getTypeAllocSize(Ty);
  }
}

void Emitter::initJumpTableInfo(MachineJumpTableInfo *MJTI) {
  if (TM.getJITInfo()->hasCustomJumpTables())
    return;
  if (MJTI->getEntryKind() == MachineJumpTableInfo::EK_Inline)
    return;

  const std::vector<MachineJumpTableEntry> &JT = MJTI->getJumpTables();
  if (JT.empty()) return;

  unsigned NumEntries = 0;
  for (unsigned i = 0, e = JT.size(); i != e; ++i)
    NumEntries += JT[i].MBBs.size();

  unsigned EntrySize = MJTI->getEntrySize(TD);

  // Just allocate space for all the jump tables now.  We will fix up the actual
  // MBB entries in the tables after we emit the code for each block, since then
  // we will know the final locations of the MBBs in memory.
  JumpTable = MJTI;
  JumpTableBase = allocateSpace(NumEntries * EntrySize,
                             MJTI->getEntryAlignment(TD));
}

void Emitter::emitJumpTableInfo(MachineJumpTableInfo *MJTI) {
  if (TM.getJITInfo()->hasCustomJumpTables())
    return;

  const std::vector<MachineJumpTableEntry> &JT = MJTI->getJumpTables();
  if (JT.empty() || JumpTableBase == 0) return;


  switch (MJTI->getEntryKind()) {
  case MachineJumpTableInfo::EK_Inline:
    return;
  case MachineJumpTableInfo::EK_BlockAddress: {
    // EK_BlockAddress - Each entry is a plain address of block, e.g.:
    //     .word LBB123
    assert(MJTI->getEntrySize(TD) == sizeof(void*) &&
           "Cross JIT'ing?");

    // For each jump table, map each target in the jump table to the address of
    // an emitted MachineBasicBlock.
    intptr_t *SlotPtr = (intptr_t*)JumpTableBase;

    for (unsigned i = 0, e = JT.size(); i != e; ++i) {
      const std::vector<MachineBasicBlock*> &MBBs = JT[i].MBBs;
      // Store the address of the basic block for this jump table slot in the
      // memory we allocated for the jump table in 'initJumpTableInfo'
      for (unsigned mi = 0, me = MBBs.size(); mi != me; ++mi)
        *SlotPtr++ = getMachineBasicBlockAddress(MBBs[mi]);
    }
    break;
  }

  case MachineJumpTableInfo::EK_Custom32:
  case MachineJumpTableInfo::EK_GPRel32BlockAddress:
  case MachineJumpTableInfo::EK_LabelDifference32: {
    assert(MJTI->getEntrySize(TD) == 4&&"Cross JIT'ing?");
    // For each jump table, place the offset from the beginning of the table
    // to the target address.
    int *SlotPtr = (int*)JumpTableBase;

    for (unsigned i = 0, e = JT.size(); i != e; ++i) {
      const std::vector<MachineBasicBlock*> &MBBs = JT[i].MBBs;
      // Store the offset of the basic block for this jump table slot in the
      // memory we allocated for the jump table in 'initJumpTableInfo'
      uintptr_t Base = (uintptr_t)SlotPtr;
      for (unsigned mi = 0, me = MBBs.size(); mi != me; ++mi) {
        uintptr_t MBBAddr = getMachineBasicBlockAddress(MBBs[mi]);
        /// FIXME: USe EntryKind instead of magic "getPICJumpTableEntry" hook.
        *SlotPtr++ = TM.getJITInfo()->getPICJumpTableEntry(MBBAddr, Base);
      }
    }
    break;
  }
  case MachineJumpTableInfo::EK_GPRel64BlockAddress:
    report_fatal_error(
           "JT Info emission not implemented for GPRel64BlockAddress yet.");
  }
}

void *Emitter::allocIndirectGV(const GlobalValue *GV,
                                  const uint8_t *Buffer, size_t Size,
                                  unsigned Alignment) {
	abort();
 uint8_t *IndGV =  0;//MemMgr->allocateStub(GV, Size, Alignment);
  memcpy(IndGV, Buffer, Size);
  return IndGV;
}

// getConstantPoolEntryAddress - Return the address of the 'ConstantNum' entry
// in the constant pool that was last emitted with the 'emitConstantPool'
// method.
//
uintptr_t Emitter::getConstantPoolEntryAddress(unsigned ConstantNum) const {
  assert(ConstantNum < ConstantPool->getConstants().size() &&
         "Invalid ConstantPoolIndex!");
  return ConstPoolAddresses[ConstantNum];
}

// getJumpTableEntryAddress - Return the address of the JumpTable with index
// 'Index' in the jumpp table that was last initialized with 'initJumpTableInfo'
//
uintptr_t Emitter::getJumpTableEntryAddress(unsigned Index) const {
  const std::vector<MachineJumpTableEntry> &JT = JumpTable->getJumpTables();
  assert(Index < JT.size() && "Invalid jump table index!");

  unsigned EntrySize = JumpTable->getEntrySize(TD);

  unsigned Offset = 0;
  for (unsigned i = 0; i < Index; ++i)
    Offset += JT[i].MBBs.size();

   Offset *= EntrySize;

  return (uintptr_t)((char *)JumpTableBase + Offset);
}

uint8_t *Emitter::startFunctionBody(const Function *F, uintptr_t &ActualSize)
{
	if(ActualSize == 0)
		ActualSize = 0x100;

	void *result = std::malloc(ActualSize);

	return (uint8_t *)result;
}

void Emitter::endFunctionBody(const Function *F, uint8_t *FunctionStart, uint8_t *FunctionEnd)
{
}

uint8_t *Emitter::memAllocateSpace(intptr_t Size, unsigned Alignment)
{
	return (uint8_t *)std::malloc(Size);
}

void Emitter::deallocateFunctionBody(void *Body)
{
}

void *Emitter::allocateGlobal(uintptr_t Size, unsigned Alignment) {
	abort();
	return 0;
}

void Emitter::EmittedFunctionConfig::onDelete(
  Emitter *Emitter, const Function *F) {
  Emitter->deallocateMemForFunction(F);
}
void Emitter::EmittedFunctionConfig::onRAUW(
  Emitter *, const Function*, const Function*) {
  report_fatal_error("The JIT doesn't know how to handle a"
                   " RAUW on a value it has emitted.");
}
};