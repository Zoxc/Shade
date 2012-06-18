#include "shade.hpp"
#include "emitter.hpp"
#include "memory-manager.hpp"

LLVMContext context;

d3c_error_t shade_error(d3c_error_type_t error, const char *message)
{
	auto result = new d3c_error;

	result->num = error;
	result->message = strdup(message);

	return result;
}

extern "C" D3C_EXPORT void D3C_API d3c_free_error(d3c_error_t error)
{
	if(error->message)
		free((void *)error->message);

	delete error;
}

extern "C" D3C_EXPORT d3c_error_t D3C_API d3c_init()
{
	try
	{
		InitializeNativeTarget();

		DebugFlag = true;

		OwningPtr<MemoryBuffer> buffer;
		
		LLVM_ERROR(MemoryBuffer::getFile("test.bc", buffer));

		auto module = ParseBitcodeFile(buffer.get(), context);
		auto &functions = module->getFunctionList();

		ConstantInt *number = ConstantInt::get(context, APInt(32, rand()));

		for(auto i = functions.begin(); i != functions.end(); ++i)
		{
			auto &blocks = (*i).getBasicBlockList();

			for(auto j = blocks.begin(); j != blocks.end(); ++j)
			{
				//BinaryOperator::Create(Instruction::Add, number, number, "dummy", j);
			}
		}

		module->dump();

		EngineBuilder engine_builder(module);

		engine_builder.setEngineKind(EngineKind::JIT);
		engine_builder.setRelocationModel(Reloc::Static);
		engine_builder.setCodeModel(CodeModel::Small);
		engine_builder.setOptLevel(CodeGenOpt::None);

		TargetMachine *target = engine_builder.selectTarget();
		FunctionPassManager pass_manager(module);

		pass_manager.add(new TargetData(*target->getTargetData()));

		auto mm = new CodeGen::MemoryManager;
		auto rce = createEmitter(mm, *target);

		if(target->addPassesToEmitMachineCode(pass_manager, *rce))
		{
			llvm::report_fatal_error("Target does not support machine code emission!");
		}

		for(auto i = functions.begin(); i != functions.end(); ++i)
		{
			pass_manager.run(*i);
		}
		
		delete module;
		delete target;

		return 0;
	} catch(d3c_error_t error)
	{
		
		return error;
	}
}
