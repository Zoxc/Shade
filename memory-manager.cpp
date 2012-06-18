#include "memory-manager.hpp"

namespace CodeGen
{
MemoryManager::~MemoryManager()
{
}

void MemoryManager::setMemoryWritable()
{
}

void MemoryManager::setMemoryExecutable()
{
}

void MemoryManager::setPoisonMemory(bool poison)
{
}

void *MemoryManager::getPointerToNamedFunction(const std::string &Name, bool AbortOnFailure)
{
	// Lookup kernel32.dll pointers

	if(AbortOnFailure)
		abort();

	return 0; 
}

void MemoryManager::AllocateGOT()
{
}

uint8_t *MemoryManager::getGOTBase() const
{
	abort();
	return 0;
}

uint8_t *MemoryManager::startFunctionBody(const Function *F,
                                     uintptr_t &ActualSize)
{
	void *result = std::malloc(0x1000);

	ActualSize = result ? 0x1000 : 0;

	return (uint8_t *)result;
}

uint8_t *MemoryManager::allocateStub(const GlobalValue* F, unsigned StubSize,
                                unsigned Alignment)
{
	abort();
	return 0;
}

 void MemoryManager::endFunctionBody(const Function *F, uint8_t *FunctionStart,
                               uint8_t *FunctionEnd)
{
}

uint8_t *MemoryManager::allocateCodeSection(uintptr_t Size, unsigned Alignment,
                                       unsigned SectionID)
{
	return (uint8_t *)std::malloc(Size);
}

uint8_t *MemoryManager::allocateDataSection(uintptr_t Size, unsigned Alignment,
                                       unsigned SectionID)
{
	return (uint8_t *)std::malloc(Size);
}

uint8_t *MemoryManager::allocateSpace(intptr_t Size, unsigned Alignment)
{
	return (uint8_t *)std::malloc(Size);
}

uint8_t *MemoryManager::allocateGlobal(uintptr_t Size, unsigned Alignment)
	
{
	return (uint8_t *)std::malloc(Size);
}

void MemoryManager::deallocateFunctionBody(void *Body)
{
}

uint8_t* MemoryManager::startExceptionTable(const Function* F,
                                       uintptr_t &ActualSize)
{
	abort();
	return 0;
}

void MemoryManager::endExceptionTable(const Function *F, uint8_t *TableStart,
                                 uint8_t *TableEnd, uint8_t* FrameRegister)
{
}

void MemoryManager::deallocateExceptionTable(void *ET)
{
}

};
