#include "shade.hpp"
#include "compiler.hpp"
#include "disassembler.hpp"
#include "emitter.hpp"
#include "engine.hpp"
#include "remote-heap.hpp"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
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

using namespace llvm;

Shade::RemoteHeap code_section(PAGE_EXECUTE_READ);
Shade::RemoteHeap data_section(PAGE_READWRITE);

void Shade::compile_module()
{
	Engine::modules.push_back("user32.dll");
	Engine::modules.push_back("kernel32.dll");
	Engine::modules.push_back("ntdll.dll");

	InitializeNativeTarget();

	DebugFlag = true;

	OwningPtr<MemoryBuffer> buffer;
		
	LLVM_ERROR(MemoryBuffer::getFile("external.bc", buffer));

	Module *module = ParseBitcodeFile(buffer.get(), getGlobalContext());
	auto &functions = module->getFunctionList();

	ConstantInt *number = ConstantInt::get(getGlobalContext(), APInt(32, rand()));

	for(auto i = functions.begin(); i != functions.end(); ++i)
	{
		auto &blocks = (*i).getBasicBlockList();

		for(auto j = blocks.begin(); j != blocks.end(); ++j)
		{
			auto &list = j->getInstList();
			
			list.insert(list.begin(), BinaryOperator::Create(Instruction::Add, number, number, "dummy"));
		}
	}

	module->dump();

	EngineBuilder engine_builder(module);

	engine_builder.setEngineKind(EngineKind::JIT);
	engine_builder.setRelocationModel(Reloc::Static);
	engine_builder.setCodeModel(CodeModel::Small);
	engine_builder.setOptLevel(CodeGenOpt::None);
	
	OwningPtr<TargetMachine> target(engine_builder.selectTarget());

	FunctionPassManager pass_manager(module);

	pass_manager.add(new TargetData(*target->getTargetData()));
	
	Engine engine(module, *target->getTargetData());
	Emitter emitter(engine, *target, code_section, data_section);

	if(target->addPassesToEmitMachineCode(pass_manager, emitter))
	{
		llvm::report_fatal_error("Target does not support machine code emission!");
	}

	for(auto i = functions.begin(); i != functions.end(); ++i)
	{
		pass_manager.run(*i);
	}

	emitter.resolveRelocations();

	auto init = (void (*)())engine.getPointerToFunction(module->getFunction("init"));

	init();
}
