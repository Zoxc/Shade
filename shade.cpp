#include "shade.hpp"
#include "process.hpp"
#include "d3d.hpp"
#include "compiler/compiler.hpp"
#include "compiler/disassembler.hpp"

#include <sstream>
#include <fstream>

void Shade::write(void *remote, const void *local, size_t size)
{
	if(!WriteProcessMemory(process, remote, local, size, 0))
		win32_error("Unable to write remote memory");
}

void Shade::read(const void *remote, void *local, size_t size)
{
	if(!ReadProcessMemory(process, remote, local, size, 0))
		win32_error("Unable to read remote memory");
}

std::string Shade::win32_error_code(DWORD err_no)
{
	char *msg_buffer;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |	FORMAT_MESSAGE_IGNORE_INSERTS, 0, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg_buffer, 0, 0);
	
	std::stringstream msg;

	msg << "Error #" << err_no << ": " << msg_buffer;

	return msg.str();
}

void Shade::win32_error(DWORD err_no, std::string message)
{
	error(message + "\n" + win32_error_code(err_no));
}

void Shade::win32_error(std::string message)
{
	win32_error(GetLastError(), message);
}

void Shade::error(std::string message)
{
	auto result = new d3c_error;
	
	result->message = strdup(message.c_str());

	throw result;
}

extern "C" D3C_EXPORT void D3C_API d3c_free_error(d3c_error_t error)
{
	if(error->message)
		free((void *)error->message);

	delete error;
}

void Shade::init()
{
	get_debug_privileges();
	
	find_process();
	//create_process();
	allocate_shared_memory();
	get_preset_offset();

	init_disassembler();
	compile_module();

	resume_process();
}

Shade::Error::Type Shade::remote_call(Call::Type type)
{
	shared->error_type = Error::None;
	shared->call_type = type;

	MemoryBarrier();

	signal_event(local.end);
	wait_event(local.start);
	reset_event(local.start);
	
	if(type == Call::Continue)
		return Error::None;

	if(shared->error_type == Error::OutOfMemory)
	{
		TerminateProcess(process, 1);
		error("Remote code ran out of memory");
	} else if(shared->error_type == Error::Unknown)
		error("Unknown error while executing remote call");

	return shared->error_type;
}

static bool write_ui = false;

static void list_element(Shade::Remote::UIElement *element, std::ofstream &fs, std::ofstream &fsv)
{
	std::stringstream out;
	
	out << "UIElement " << element->ptr << "\n\t Hash: " << element->hash << "\n\t Name: " << element->name->c_str() << "\n\t Visible: " << (element->visible ? "True" : "False") << "\n\t Virtual Table: " << element->v_table << "\n";
	
	if(element->text)
		out << "\t Text: " << element->text->c_str() << "\n";
	
	if(element->rect)
	{
		out << "\t Left: " << element->rect->left << "\n"
			<< "\t Top: " << element->rect->top << "\n"
			<< "\t Right: " << element->rect->right << "\n"
			<< "\t Bottom: " << element->rect->bottom << "\n";
	}
	
	fs << out.str();

	if(element->visible)
		fsv << out.str();
	
	for(auto i = element->children.begin(); i != element->children.end(); ++i)
	{
		list_element(*i, fs, fsv);
	}
}

void Shade::loop(d3c_tick_t tick_func)
{
	while(true)
	{
		remote_call(Call::Continue);
		
		if(!write_ui)
		{
			write_ui = true;

			printf("Listing UI\n");

			auto call_error = remote_call(Call::ListUI);

			if(call_error == Error::None)
			{
				std::ofstream fs;
				std::ofstream fsv;
				fs.open("ui-dump.txt");
				fsv.open("ui-visible.txt");

				list_element(shared->data.ui_root, fs, fsv);

				fs.close();
				fsv.close();
			}

			printf("Listing UI Handlers\n");

			call_error = remote_call(Call::ListUIHandlers);

			if(call_error == Error::None)
			{
				std::ofstream fs;
				fs.open("ui-handlers.txt");
				
				for(auto i = shared->data.ui_handlers->begin(); i != shared->data.ui_handlers->end(); ++i)
				{
					fs << "UIHandler " << "\n\t Hash: " << i().hash << "\n\t Name: " << i().name->c_str() << "\n\t Function: " << i().func << "\n";
				}

				fs.close();
			}
			
			printf("Listing RActors Handlers\n");

			call_error = remote_call(Call::ListRActorAssets);

			if(call_error == Error::None)
			{
				std::ofstream fs;
				fs.open("assets-RActors.txt");
				
				for(auto i = shared->data.actors->begin(); i != shared->data.actors->end(); ++i)
				{
					fs << "Actor " << "\n\t Ptr: " << i().ptr << "\n\t Id: " << i().id << "\n\t AcdId: " << i().acd_id << "\n\t Name: " << i().name->c_str() << "\n";
				}

				fs.close();
			}

			printf("Listing ActorCommonData Handlers\n");
			call_error = remote_call(Call::ListCommonDataAssets);

			if(call_error == Error::None)
			{
				std::ofstream fs;
				fs.open("assets-ActorCommonData.txt");
				
				for(auto i = shared->data.acds->begin(); i != shared->data.acds->end(); ++i)
				{
					fs << "ACD " << "\n\t Ptr: " << i().ptr << "\n\t Id: " << i().id << "\n\t OwnerId: " << i().actor_id << "\n\t Name: " << i().name->c_str() << "\n";
				}

				fs.close();
			}

		}

		tick_func();
	}
}

extern "C" D3C_EXPORT d3c_error_t D3C_API d3c_loop(d3c_tick_t tick_func)
{
	return Shade::wrap([&] {
		Shade::loop(tick_func);
	});
}

extern "C" D3C_EXPORT d3c_error_t D3C_API d3c_init()
{
	return Shade::wrap([&] {
		Shade::init();
	});
}
