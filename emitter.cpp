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
#define WIN32
#define _WINDOWS
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_WARNINGS
#define _SCL_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_WARNINGS
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
#define _HAS_EXCEPTIONS 0
#define ENABLE_X86_JIT

#pragma warning(disable:4275)
#pragma warning(disable:4624)
#pragma warning(disable:4996)
#pragma warning(disable:4355)

#define DEBUG_TYPE "jit"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/CodeGen/JITCodeEmitter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineCodeInfo.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRelocation.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/ExecutionEngine/JITMemoryManager.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetJITInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/Debug.h"
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
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ValueMap.h"
#include <algorithm>
#ifndef NDEBUG
#include <iomanip>
#endif
using namespace llvm;

STATISTIC(NumBytes, "Number of bytes of machine code compiled");
STATISTIC(NumRelos, "Number of relocations applied");
STATISTIC(NumRetries, "Number of retries with more memory");


// A declaration may stop being a declaration once it's fully read from bitcode.
// This function returns true if F is fully read and is still a declaration.
static bool isNonGhostDeclaration(const Function *F) {
  return F->isDeclaration() && !F->isMaterializable();
}

//===----------------------------------------------------------------------===//
// JIT lazy compilation code.
//
namespace {
  class JITEmitter;

  /// JITEmitter - The JIT implementation of the MachineCodeEmitter, which is
  /// used to output functions to memory for execution.
  class JITEmitter : public JITCodeEmitter {
    JITMemoryManager *MemMgr;

    // When outputting a function stub in the context of some other function, we
    // save BufferBegin/BufferEnd/CurBufferPtr here.
    uint8_t *SavedBufferBegin, *SavedBufferEnd, *SavedCurBufferPtr;

    // When reattempting to JIT a function after running out of space, we store
    // the estimated size of the function we're trying to JIT here, so we can
    // ask the memory manager for at least this much space.  When we
    // successfully emit the function, we reset this back to zero.
    uintptr_t SizeEstimate;

    /// Relocations - These are the relocations that the function needs, as
    /// emitted.
    std::vector<MachineRelocation> Relocations;

    /// MBBLocations - This vector is a mapping from MBB ID's to their address.
    /// It is filled in by the StartMachineBasicBlock callback and queried by
    /// the getMachineBasicBlockAddress callback.
    std::vector<uintptr_t> MBBLocations;

    /// ConstantPool - The constant pool for the current function.
    ///
    MachineConstantPool *ConstantPool;

    /// ConstantPoolBase - A pointer to the first entry in the constant pool.
    ///
    void *ConstantPoolBase;

    /// ConstPoolAddresses - Addresses of individual constant pool entries.
    ///
    SmallVector<uintptr_t, 8> ConstPoolAddresses;

    /// JumpTable - The jump tables for the current function.
    ///
    MachineJumpTableInfo *JumpTable;

    /// JumpTableBase - A pointer to the first entry in the jump table.
    ///
    void *JumpTableBase;

    /// LabelLocations - This vector is a mapping from Label ID's to their
    /// address.
    DenseMap<MCSymbol*, uintptr_t> LabelLocations;

    /// MMI - Machine module info for exception informations
    MachineModuleInfo* MMI;

    // CurFn - The llvm function being emitted.  Only valid during
    // finishFunction().
    const Function *CurFn;

    /// Information about emitted code, which is passed to the
    /// JITEventListeners.  This is reset in startFunction and used in
    /// finishFunction.
    JITEvent_EmittedFunctionDetails EmissionDetails;

    struct EmittedCode {
      void *FunctionBody;  // Beginning of the function's allocation.
      void *Code;  // The address the function's code actually starts at.
      void *ExceptionTable;
      EmittedCode() : FunctionBody(0), Code(0), ExceptionTable(0) {}
    };
    struct EmittedFunctionConfig : public ValueMapConfig<const Function*> {
      typedef JITEmitter *ExtraData;
      static void onDelete(JITEmitter *, const Function*);
      static void onRAUW(JITEmitter *, const Function*, const Function*);
    };
    ValueMap<const Function *, EmittedCode,
             EmittedFunctionConfig> EmittedFunctions;

    DebugLoc PrevDL;
	TargetMachine &TM;

  public:
    JITEmitter(JITMemoryManager *JMM, TargetMachine &TM)
      : SizeEstimate(0), MMI(0), CurFn(0), TM(TM),
        EmittedFunctions(this) {
      MemMgr = JMM;
    }
    ~JITEmitter() {
      delete MemMgr;
    }

    /// classof - Methods for support type inquiry through isa, cast, and
    /// dyn_cast:
    ///
    static inline bool classof(const MachineCodeEmitter*) { return true; }

    virtual void startFunction(MachineFunction &F);
    virtual bool finishFunction(MachineFunction &F);

    void emitConstantPool(MachineConstantPool *MCP);
    void initJumpTableInfo(MachineJumpTableInfo *MJTI);
    void emitJumpTableInfo(MachineJumpTableInfo *MJTI);

    void startGVStub(const GlobalValue* GV,
                     unsigned StubSize, unsigned Alignment = 1);
    void startGVStub(void *Buffer, unsigned StubSize);
    void finishGVStub();
    virtual void *allocIndirectGV(const GlobalValue *GV,
                                  const uint8_t *Buffer, size_t Size,
                                  unsigned Alignment);

    /// allocateSpace - Reserves space in the current block if any, or
    /// allocate a new one of the given size.
    virtual void *allocateSpace(uintptr_t Size, unsigned Alignment);

    /// allocateGlobal - Allocate memory for a global.  Unlike allocateSpace,
    /// this method does not allocate memory in the current output buffer,
    /// because a global may live longer than the current function.
    virtual void *allocateGlobal(uintptr_t Size, unsigned Alignment);

    virtual void addRelocation(const MachineRelocation &MR) {
      Relocations.push_back(MR);
    }

    virtual void StartMachineBasicBlock(MachineBasicBlock *MBB) {
      if (MBBLocations.size() <= (unsigned)MBB->getNumber())
        MBBLocations.resize((MBB->getNumber()+1)*2);
      MBBLocations[MBB->getNumber()] = getCurrentPCValue();
      DEBUG(dbgs() << "JIT: Emitting BB" << MBB->getNumber() << " at ["
                   << (void*) getCurrentPCValue() << "]\n");
    }

    virtual uintptr_t getConstantPoolEntryAddress(unsigned Entry) const;
    virtual uintptr_t getJumpTableEntryAddress(unsigned Entry) const;

    virtual uintptr_t getMachineBasicBlockAddress(MachineBasicBlock *MBB) const{
      assert(MBBLocations.size() > (unsigned)MBB->getNumber() &&
             MBBLocations[MBB->getNumber()] && "MBB not emitted!");
      return MBBLocations[MBB->getNumber()];
    }

    /// retryWithMoreMemory - Log a retry and deallocate all memory for the
    /// given function.  Increase the minimum allocation size so that we get
    /// more memory next time.
    void retryWithMoreMemory(MachineFunction &F);

    /// deallocateMemForFunction - Deallocate all memory for the specified
    /// function body.
    void deallocateMemForFunction(const Function *F);

    virtual void processDebugLoc(DebugLoc DL, bool BeforePrintingInsn);

    virtual void emitLabel(MCSymbol *Label) {
      LabelLocations[Label] = getCurrentPCValue();
    }

    virtual DenseMap<MCSymbol*, uintptr_t> *getLabelLocations() {
      return &LabelLocations;
    }

    virtual uintptr_t getLabelAddress(MCSymbol *Label) const {
      assert(LabelLocations.count(Label) && "Label not emitted!");
      return LabelLocations.find(Label)->second;
    }

    virtual void setModuleInfo(MachineModuleInfo* Info) {
    }

  private:
    void *getPointerToGlobal(GlobalValue *GV, void *Reference,
                             bool MayNeedFarStub);
    void *getPointerToGVIndirectSym(GlobalValue *V, void *Reference);
  };
}

//===----------------------------------------------------------------------===//
// JITEmitter code.
//
void *JITEmitter::getPointerToGlobal(GlobalValue *V, void *Reference,
                                     bool MayNeedFarStub) {
  // If we have already compiled the function, return a pointer to its body.
  Function *F = cast<Function>(V);

  // If we know the target can handle arbitrary-distance calls, try to
  // return a direct pointer.
  if (!MayNeedFarStub) {
    // If we have code, go ahead and return that.
    //void *ResultPtr = TheJIT->getPointerToGlobalIfAvailable(F);
   // if (ResultPtr) return ResultPtr;

    // If this is an external function pointer, we can force the JIT to
    // 'compile' it, which really just adds it to the map.
   // if (isNonGhostDeclaration(F) || F->hasAvailableExternallyLinkage())
   //   return TheJIT->getPointerToFunction(F);
  }

  return 0;
}

void *JITEmitter::getPointerToGVIndirectSym(GlobalValue *V, void *Reference) {
  // Make sure GV is emitted first, and create a stub containing the fully
  // resolved address.
  void *GVAddress = getPointerToGlobal(V, Reference, false);
  //void *StubAddr = Resolver.getGlobalValueIndirectSym(V, GVAddress);
  return GVAddress;//StubAddr;
}

void JITEmitter::processDebugLoc(DebugLoc DL, bool BeforePrintingInsn) {
  if (DL.isUnknown()) return;
  if (!BeforePrintingInsn) return;

  const LLVMContext &Context = EmissionDetails.MF->getFunction()->getContext();

  if (DL.getScope(Context) != 0 && PrevDL != DL) {
    JITEvent_EmittedFunctionDetails::LineStart NextLine;
    NextLine.Address = getCurrentPCValue();
    NextLine.Loc = DL;
    EmissionDetails.LineStarts.push_back(NextLine);
  }

  PrevDL = DL;
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

void JITEmitter::startFunction(MachineFunction &F) {
  DEBUG(dbgs() << "JIT: Starting CodeGen of Function "
        << F.getFunction()->getName() << "\n");

  uintptr_t ActualSize = 0;
  // Set the memory writable, if it's not already
  MemMgr->setMemoryWritable();

  if (SizeEstimate > 0) {
    // SizeEstimate will be non-zero on reallocation attempts.
    ActualSize = SizeEstimate;
  }

  BufferBegin = CurBufferPtr = MemMgr->startFunctionBody(F.getFunction(),
                                                         ActualSize);
  BufferEnd = BufferBegin+ActualSize;
  EmittedFunctions[F.getFunction()].FunctionBody = BufferBegin;

  // Ensure the constant pool/jump table info is at least 4-byte aligned.
  emitAlignment(16);

  emitConstantPool(F.getConstantPool());
  if (MachineJumpTableInfo *MJTI = F.getJumpTableInfo())
    initJumpTableInfo(MJTI);

  // About to start emitting the machine code for the function.
  emitAlignment(std::max(F.getFunction()->getAlignment(), 8U));
  EmittedFunctions[F.getFunction()].Code = CurBufferPtr;

  MBBLocations.clear();

  EmissionDetails.MF = &F;
  EmissionDetails.LineStarts.clear();
}

bool JITEmitter::finishFunction(MachineFunction &F) {
  if (CurBufferPtr == BufferEnd) {
    // We must call endFunctionBody before retrying, because
    // deallocateMemForFunction requires it.
    MemMgr->endFunctionBody(F.getFunction(), BufferBegin, CurBufferPtr);
    retryWithMoreMemory(F);
    return true;
  }

  if (MachineJumpTableInfo *MJTI = F.getJumpTableInfo())
    emitJumpTableInfo(MJTI);

  // FnStart is the start of the text, not the start of the constant pool and
  // other per-function data.
  uint8_t *FnStart = BufferBegin;
  //  (uint8_t *)TheJIT->getPointerToGlobalIfAvailable(F.getFunction());

  // FnEnd is the end of the function's machine code.
  uint8_t *FnEnd = CurBufferPtr;

  if (!Relocations.empty()) {
    CurFn = F.getFunction();
    NumRelos += Relocations.size();

    // Resolve the relocations to concrete pointers.
    for (unsigned i = 0, e = Relocations.size(); i != e; ++i) {
      MachineRelocation &MR = Relocations[i];
      void *ResultPtr = 0;
      if (!MR.letTargetResolve()) {
        if (MR.isExternalSymbol()) {
			abort();
          ResultPtr = 0;//TheJIT->getPointerToNamedFunction(MR.getExternalSymbol(), false);
          DEBUG(dbgs() << "JIT: Map \'" << MR.getExternalSymbol() << "\' to ["
                       << ResultPtr << "]\n");

          // If the target REALLY wants a stub for this function, emit it now.
          if (MR.mayNeedFarStub()) {
            //ResultPtr = Resolver.getExternalFunctionStub(ResultPtr);
          }
        } else if (MR.isGlobalValue()) {
          ResultPtr = getPointerToGlobal(MR.getGlobalValue(),
                                         BufferBegin+MR.getMachineCodeOffset(),
                                         MR.mayNeedFarStub());
        } else if (MR.isIndirectSymbol()) {
          ResultPtr = getPointerToGVIndirectSym(
              MR.getGlobalValue(), BufferBegin+MR.getMachineCodeOffset());
        } else if (MR.isBasicBlock()) {
          ResultPtr = (void*)getMachineBasicBlockAddress(MR.getBasicBlock());
        } else if (MR.isConstantPoolIndex()) {
          ResultPtr =
            (void*)getConstantPoolEntryAddress(MR.getConstantPoolIndex());
        } else {
          assert(MR.isJumpTableIndex());
          ResultPtr=(void*)getJumpTableEntryAddress(MR.getJumpTableIndex());
        }

        MR.setResultPointer(ResultPtr);
      }

      // if we are managing the GOT and the relocation wants an index,
      // give it one
      if (MR.isGOTRelative() && MemMgr->isManagingGOT()) {
		  abort();
       /* unsigned idx = Resolver.getGOTIndexForAddr(ResultPtr);
        MR.setGOTIndex(idx);
        if (((void**)MemMgr->getGOTBase())[idx] != ResultPtr) {
          DEBUG(dbgs() << "JIT: GOT was out of date for " << ResultPtr
                       << " pointing at " << ((void**)MemMgr->getGOTBase())[idx]
                       << "\n");
          ((void**)MemMgr->getGOTBase())[idx] = ResultPtr;
        }*/
      }
    }

    CurFn = 0;
	
	TM.getJITInfo()->relocate(BufferBegin, &Relocations[0], Relocations.size(), MemMgr->getGOTBase());
  }

  // Update the GOT entry for F to point to the new code.
  if (MemMgr->isManagingGOT()) {
	  abort();
    /*unsigned idx = Resolver.getGOTIndexForAddr((void*)BufferBegin);
    if (((void**)MemMgr->getGOTBase())[idx] != (void*)BufferBegin) {
      DEBUG(dbgs() << "JIT: GOT was out of date for " << (void*)BufferBegin
                   << " pointing at " << ((void**)MemMgr->getGOTBase())[idx]
                   << "\n");
      ((void**)MemMgr->getGOTBase())[idx] = (void*)BufferBegin;
    }*/
  }

  // CurBufferPtr may have moved beyond FnEnd, due to memory allocation for
  // global variables that were referenced in the relocations.
  MemMgr->endFunctionBody(F.getFunction(), BufferBegin, CurBufferPtr);

  if (CurBufferPtr == BufferEnd) {
    retryWithMoreMemory(F);
    return true;
  } else {
    // Now that we've succeeded in emitting the function, reset the
    // SizeEstimate back down to zero.
    SizeEstimate = 0;
  }

  BufferBegin = CurBufferPtr = 0;

  // Reset the previous debug location.
  PrevDL = DebugLoc();

  DEBUG(dbgs() << "JIT: Finished CodeGen of [" << (void*)FnStart
        << "] Function: " << F.getFunction()->getName()
        << ": " << (FnEnd-FnStart) << " bytes of text, "
        << Relocations.size() << " relocations\n");

  Relocations.clear();
  ConstPoolAddresses.clear();

  // Mark code region readable and executable if it's not so already.
  MemMgr->setMemoryExecutable();

  if (MMI)
    MMI->EndFunction();

  return false;
}

void JITEmitter::retryWithMoreMemory(MachineFunction &F) {
  DEBUG(dbgs() << "JIT: Ran out of space for native code.  Reattempting.\n");
  Relocations.clear();  // Clear the old relocations or we'll reapply them.
  ConstPoolAddresses.clear();
  ++NumRetries;
  deallocateMemForFunction(F.getFunction());
  // Try again with at least twice as much free space.
  SizeEstimate = (uintptr_t)(2 * (BufferEnd - BufferBegin));
}

/// deallocateMemForFunction - Deallocate all memory for the specified
/// function body.  Also drop any references the function has to stubs.
/// May be called while the Function is being destroyed inside ~Value().
void JITEmitter::deallocateMemForFunction(const Function *F) {
  ValueMap<const Function *, EmittedCode, EmittedFunctionConfig>::iterator
    Emitted = EmittedFunctions.find(F);
  if (Emitted != EmittedFunctions.end()) {
    MemMgr->deallocateFunctionBody(Emitted->second.FunctionBody);
    MemMgr->deallocateExceptionTable(Emitted->second.ExceptionTable);

    EmittedFunctions.erase(Emitted);
  }
}


void *JITEmitter::allocateSpace(uintptr_t Size, unsigned Alignment) {
  if (BufferBegin)
    return JITCodeEmitter::allocateSpace(Size, Alignment);

  // create a new memory block if there is no active one.
  // care must be taken so that BufferBegin is invalidated when a
  // block is trimmed
  BufferBegin = CurBufferPtr = MemMgr->allocateSpace(Size, Alignment);
  BufferEnd = BufferBegin+Size;
  return CurBufferPtr;
}

void *JITEmitter::allocateGlobal(uintptr_t Size, unsigned Alignment) {
  // Delegate this call through the memory manager.
  return MemMgr->allocateGlobal(Size, Alignment);
}

void JITEmitter::emitConstantPool(MachineConstantPool *MCP) {
  const std::vector<MachineConstantPoolEntry> &Constants = MCP->getConstants();
  if (Constants.empty()) return;

  unsigned Size = GetConstantPoolSizeInBytes(MCP, TM.getTargetData());
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
    Offset += TM.getTargetData()->getTypeAllocSize(Ty);
  }
}

void JITEmitter::initJumpTableInfo(MachineJumpTableInfo *MJTI) {
  if (TM.getJITInfo()->hasCustomJumpTables())
    return;
  if (MJTI->getEntryKind() == MachineJumpTableInfo::EK_Inline)
    return;

  const std::vector<MachineJumpTableEntry> &JT = MJTI->getJumpTables();
  if (JT.empty()) return;

  unsigned NumEntries = 0;
  for (unsigned i = 0, e = JT.size(); i != e; ++i)
    NumEntries += JT[i].MBBs.size();

  unsigned EntrySize = MJTI->getEntrySize(*TM.getTargetData());

  // Just allocate space for all the jump tables now.  We will fix up the actual
  // MBB entries in the tables after we emit the code for each block, since then
  // we will know the final locations of the MBBs in memory.
  JumpTable = MJTI;
  JumpTableBase = allocateSpace(NumEntries * EntrySize,
                             MJTI->getEntryAlignment(*TM.getTargetData()));
}

void JITEmitter::emitJumpTableInfo(MachineJumpTableInfo *MJTI) {
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
    assert(MJTI->getEntrySize(*TM.getTargetData()) == sizeof(void*) &&
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
    assert(MJTI->getEntrySize(*TM.getTargetData()) == 4&&"Cross JIT'ing?");
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
    llvm_unreachable(
           "JT Info emission not implemented for GPRel64BlockAddress yet.");
  }
}

void JITEmitter::startGVStub(const GlobalValue* GV,
                             unsigned StubSize, unsigned Alignment) {
  SavedBufferBegin = BufferBegin;
  SavedBufferEnd = BufferEnd;
  SavedCurBufferPtr = CurBufferPtr;

  BufferBegin = CurBufferPtr = MemMgr->allocateStub(GV, StubSize, Alignment);
  BufferEnd = BufferBegin+StubSize+1;
}

void JITEmitter::startGVStub(void *Buffer, unsigned StubSize) {
  SavedBufferBegin = BufferBegin;
  SavedBufferEnd = BufferEnd;
  SavedCurBufferPtr = CurBufferPtr;

  BufferBegin = CurBufferPtr = (uint8_t *)Buffer;
  BufferEnd = BufferBegin+StubSize+1;
}

void JITEmitter::finishGVStub() {
  assert(CurBufferPtr != BufferEnd && "Stub overflowed allocated space.");
  NumBytes += getCurrentPCOffset();
  BufferBegin = SavedBufferBegin;
  BufferEnd = SavedBufferEnd;
  CurBufferPtr = SavedCurBufferPtr;
}

void *JITEmitter::allocIndirectGV(const GlobalValue *GV,
                                  const uint8_t *Buffer, size_t Size,
                                  unsigned Alignment) {
  uint8_t *IndGV = MemMgr->allocateStub(GV, Size, Alignment);
  memcpy(IndGV, Buffer, Size);
  return IndGV;
}

// getConstantPoolEntryAddress - Return the address of the 'ConstantNum' entry
// in the constant pool that was last emitted with the 'emitConstantPool'
// method.
//
uintptr_t JITEmitter::getConstantPoolEntryAddress(unsigned ConstantNum) const {
  assert(ConstantNum < ConstantPool->getConstants().size() &&
         "Invalid ConstantPoolIndex!");
  return ConstPoolAddresses[ConstantNum];
}

// getJumpTableEntryAddress - Return the address of the JumpTable with index
// 'Index' in the jumpp table that was last initialized with 'initJumpTableInfo'
//
uintptr_t JITEmitter::getJumpTableEntryAddress(unsigned Index) const {
  const std::vector<MachineJumpTableEntry> &JT = JumpTable->getJumpTables();
  assert(Index < JT.size() && "Invalid jump table index!");

  unsigned EntrySize = JumpTable->getEntrySize(*TM.getTargetData());

  unsigned Offset = 0;
  for (unsigned i = 0; i < Index; ++i)
    Offset += JT[i].MBBs.size();

   Offset *= EntrySize;

  return (uintptr_t)((char *)JumpTableBase + Offset);
}

void JITEmitter::EmittedFunctionConfig::onDelete(
  JITEmitter *Emitter, const Function *F) {
  Emitter->deallocateMemForFunction(F);
}
void JITEmitter::EmittedFunctionConfig::onRAUW(
  JITEmitter *, const Function*, const Function*) {
  llvm_unreachable("The JIT doesn't know how to handle a"
                   " RAUW on a value it has emitted.");
}


//===----------------------------------------------------------------------===//
//  Public interface to this file

JITCodeEmitter *createEmitter(JITMemoryManager *JMM, TargetMachine &tm) {
  return new JITEmitter(JMM, tm);
}