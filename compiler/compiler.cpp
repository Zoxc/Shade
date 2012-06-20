#include "../shade.hpp"
#include "compiler.hpp"
#include "disassembler.hpp"
#include "emitter.hpp"
#include "engine.hpp"
#include "remote-heap.hpp"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
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
#include <llvm/Support/CommandLine.h>

using namespace llvm;

Shade::RemoteHeap code_section(PAGE_EXECUTE_READ);
Shade::RemoteHeap data_section(PAGE_READWRITE);

static void fatal_error_handler(void *user_data, const std::string& reason)
{
	throw Shade::error("Fatal LLVM Error: " + reason);
}

void Shade::compile_module()
{
	srand(GetTickCount());

	install_fatal_error_handler(fatal_error_handler);

	Engine::modules.push_back("user32.dll");
	Engine::modules.push_back("kernel32.dll");
	Engine::modules.push_back("ntdll.dll");

	InitializeNativeTarget();

	const char *argv[] = {"", "-debug-pass=Executions"};

	cl::ParseCommandLineOptions(1, argv);

	//DebugFlag = true;

	OwningPtr<MemoryBuffer> buffer;
		
	LLVM_ERROR(MemoryBuffer::getFile("external.bc", buffer));

	Module *module = ParseBitcodeFile(buffer.get(), getGlobalContext());
	auto &functions = module->getFunctionList();
	
	for(auto i = functions.begin(); i != functions.end(); ++i)
	{
		auto &blocks = (*i).getBasicBlockList();

		for(auto j = blocks.begin(); j != blocks.end(); ++j)
		{
			GlobalVariable *dummy = new GlobalVariable(*module, IntegerType::get(getGlobalContext(), 32), false, GlobalValue::PrivateLinkage, ConstantInt::get(getGlobalContext(), APInt(32, Prelude::align(rand(), 4))), "__shade"); 

			auto &list = j->getInstList();
			
			if(list.empty())
				continue;

			auto pos = list.begin();

			if(pos->getOpcode() == Instruction::PHI)
				++pos;

			IRBuilder<> b(&*pos);
			
			ConstantInt *number = ConstantInt::get(getGlobalContext(), APInt(32,  Prelude::align(rand(), 4) % 256));

			static Instruction::BinaryOps operators[] = {
				Instruction::Add,
				Instruction::Sub,
				Instruction::Mul,
				Instruction::And,
				Instruction::Or,
				Instruction::Xor,
				Instruction::Shl
			};

			size_t operator_size = sizeof(operators) / sizeof(Instruction::BinaryOps);
			
			auto dummy_val = b.CreateLoad(dummy, false, "");
			auto dummy_result = b.Insert(BinaryOperator::Create(operators[rand() % operator_size], dummy_val, number));
			b.CreateStore(dummy_result, dummy, false);
		}
	}
	
	module->dump();

	verifyModule(*module); 
	
	EngineBuilder engine_builder(module);

	engine_builder.setEngineKind(EngineKind::JIT);
	engine_builder.setRelocationModel(Reloc::Static);
	engine_builder.setCodeModel(CodeModel::Small);
	engine_builder.setOptLevel(CodeGenOpt::Default);
	
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

	// "llvm.global_ctors" Array of constructors

	auto init = (void (*)(int a))engine.getPointerToFunction(module->getFunction("init"));

	init(2);
}
