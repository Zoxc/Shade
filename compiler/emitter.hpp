#include "../shade.hpp"

namespace llvm
{
	class TargetMachine;
	class TargetData;
}

#include <vector>
#include "llvm/CodeGen/JITCodeEmitter.h"
#include "llvm/CodeGen/MachineRelocation.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/ValueMap.h"

namespace Shade
{
	class Engine;
	class RemoteHeap;

  class Emitter : public llvm::JITCodeEmitter {
    // When reattempting to JIT a function after running out of space, we store
    // the estimated size of the function we're trying to JIT here, so we can
    // ask the memory manager for at least this much space.  When we
    // successfully emit the function, we reset this back to zero.
    uintptr_t SizeEstimate;

    /// ConstantPool - The constant pool for the current function.
    ///
    llvm::MachineConstantPool *ConstantPool;

    /// ConstantPoolBase - A pointer to the first entry in the constant pool.
    ///
    void *ConstantPoolBase;

    /// ConstPoolAddresses - Addresses of individual constant pool entries.
    ///
    llvm::SmallVector<uintptr_t, 8> ConstPoolAddresses;

    /// JumpTable - The jump tables for the current function.
    ///
    llvm::MachineJumpTableInfo *JumpTable;

    /// JumpTableBase - A pointer to the first entry in the jump table.
    ///
    void *JumpTableBase;

    /// LabelLocations - This vector is a mapping from Label ID's to their
    /// address.
    llvm::DenseMap<llvm::MCSymbol*, size_t> LabelLocations;
	
	llvm::DenseMap<llvm::GlobalValue *, void *> IndirectSymMap;

	size_t code_size;

	Engine &engine;

	RemoteHeap &code_section;
	RemoteHeap &data_section;

    struct EmittedCode {
	  const llvm::Function *Function;
      void *FunctionBody;  // Beginning of the function's allocation.
      void *AlignedStart;
      void *Code;  // The address the function's code actually starts at.
	  void *End;
	  void *Target;
	  size_t Size;
      std::vector<llvm::MachineRelocation> Relocations;
	  std::vector<size_t> JumpTableOffsets;
	  unsigned JumpTableEntrySize;
	  
		/// MBBLocations - This vector is a mapping from MBB ID's to their address.
		/// It is filled in by the StartMachineBasicBlock callback and queried by
		/// the getMachineBasicBlockAddress callback.
		std::vector<uintptr_t> MBBLocations;


      EmittedCode() : FunctionBody(0), Code(0), Target(0), End(0), AlignedStart(0) {}
    };
    struct EmittedFunctionConfig : public llvm::ValueMapConfig<const llvm::Function*> {
      typedef Emitter *ExtraData;
      static void onDelete(Emitter *, const llvm::Function*);
      static void onRAUW(Emitter *, const llvm::Function*, const llvm::Function*);
    };
    llvm::ValueMap<const llvm::Function *, EmittedCode,
             EmittedFunctionConfig> EmittedFunctions;
	
	EmittedCode *CurrentCode;

	llvm::ValueMap<const llvm::GlobalValue *, void *> GlobalOffsets;

	llvm::TargetMachine &TM;
	const llvm::TargetData &TD;
	
	/// startFunctionBody - When we start JITing a function, the JIT calls this
	/// method to allocate a block of free RWX memory, which returns a pointer to
	/// it.  If the JIT wants to request a block of memory of at least a certain
	/// size, it passes that value as ActualSize, and this method returns a block
	/// with at least that much space.  If the JIT doesn't know ahead of time how
	/// much space it will need to emit the function, it passes 0 for the
	/// ActualSize.  In either case, this method is required to pass back the size
	/// of the allocated block through ActualSize.  The JIT will be careful to
	/// not write more than the returned ActualSize bytes of memory.
	uint8_t *startFunctionBody(const llvm::Function *F,
										uintptr_t &ActualSize);
	
	/// endFunctionBody - This method is called when the JIT is done codegen'ing
	/// the specified function.  At this point we know the size of the JIT
	/// compiled function.  This passes in FunctionStart (which was returned by
	/// the startFunctionBody method) and FunctionEnd which is a pointer to the
	/// actual end of the function.  This method should mark the space allocated
	/// and remember where it is in case the client wants to deallocate it.
	void endFunctionBody(const llvm::Function *F, uint8_t *FunctionStart,
								uint8_t *FunctionEnd);
	
	/// memAllocateSpace - Allocate a memory block of the given size.  This method
	/// cannot be called between calls to startFunctionBody and endFunctionBody.
	uint8_t *memAllocateSpace(intptr_t Size, unsigned Alignment);
	
	/// deallocateFunctionBody - Free the specified function body.  The argument
	/// must be the return value from a call to startFunctionBody() that hasn't
	/// been deallocated yet.  This is never called when the JIT is currently
	/// emitting a function.
	void deallocateFunctionBody(void *Body);

	void *getGlobalAddress(const llvm::GlobalValue *V);
	void *getGlobalVariableAddress(const llvm::GlobalVariable *V);
	void *getGlobalValueIndirectSym(llvm::GlobalValue *GV, void *GVAddress);
  public:
    Emitter(Engine &engine, llvm::TargetMachine &TM, RemoteHeap &code_section, RemoteHeap &data_section);
    ~Emitter() {
    }
	
    /// classof - Methods for support type inquiry through isa, cast, and
    /// dyn_cast:
    ///
    static inline bool classof(const llvm::MachineCodeEmitter*) { return true; }

    virtual void startFunction(llvm::MachineFunction &F);
    virtual bool finishFunction(llvm::MachineFunction &F);
	
	void resolveRelocations();

    void emitConstantPool(llvm::MachineConstantPool *MCP);
    void initJumpTableInfo(llvm::MachineJumpTableInfo *MJTI);
    void emitJumpTableInfo(llvm::MachineJumpTableInfo *MJTI);

    virtual void *allocIndirectGV(const llvm::GlobalValue *GV,
                                  const uint8_t *Buffer, size_t Size,
                                  unsigned Alignment);

    /// allocateSpace - Reserves space in the current block if any, or
    /// allocate a new one of the given size.
    virtual void *allocateSpace(uintptr_t Size, unsigned Alignment);

    /// allocateGlobal - Allocate memory for a global.  Unlike allocateSpace,
    /// this method does not allocate memory in the current output buffer,
    /// because a global may live longer than the current function.
    virtual void *allocateGlobal(uintptr_t Size, unsigned Alignment);

    virtual void addRelocation(const llvm::MachineRelocation &MR);

    virtual void StartMachineBasicBlock(llvm::MachineBasicBlock *MBB);

    virtual uintptr_t getConstantPoolEntryAddress(unsigned Entry) const;
    virtual uintptr_t getJumpTableEntryAddress(unsigned Entry) const;
	
	uintptr_t getMachineBasicBlockAddress(int index) const;
    virtual uintptr_t getMachineBasicBlockAddress(llvm::MachineBasicBlock *MBB) const;

    /// retryWithMoreMemory - Log a retry and deallocate all memory for the
    /// given function.  Increase the minimum allocation size so that we get
    /// more memory next time.
    void retryWithMoreMemory(llvm::MachineFunction &F);

    /// deallocateMemForFunction - Deallocate all memory for the specified
    /// function body.
    void deallocateMemForFunction(const llvm::Function *F);
	
	bool earlyResolveAddresses() const { return false; }

    virtual void processDebugLoc(llvm::DebugLoc DL, bool BeforePrintingInsn);

    virtual void emitLabel(llvm::MCSymbol *Label);

    virtual llvm::DenseMap<llvm::MCSymbol*, uintptr_t> *getLabelLocations();

    virtual uintptr_t getLabelAddress(llvm::MCSymbol *Label) const;

    virtual void setModuleInfo(llvm::MachineModuleInfo* Info) {
    }

  private:
    void *getPointerToGlobal(llvm::GlobalValue *GV, void *Reference,
                             bool MayNeedFarStub);
    void *getPointerToGVIndirectSym(llvm::GlobalValue *V, void *Reference);
  };
};

