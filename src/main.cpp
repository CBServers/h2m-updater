#include <std_include.hpp>

#include "updater/updater.hpp"
#include <utils/io.hpp>
#include <utils/nt.hpp>
#include <utils/question.hpp>

bool check_if_has_mwr()
{
	if (!utils::io::file_exists("h1_mp64_ship.exe"))
	{
		return utils::question::ask_y_n_question("A valid MWR install not found. Are you sure you want to continue downloading H2M-Mod?");
	}

	return true;
}

void start_h2m()
{
	if (!utils::io::file_exists("h2m-mod.exe"))
	{
		throw std::runtime_error("h2m-mod.exe not found!");
	}

	if (utils::nt::is_process_running("h2m-mod.exe"))
	{
		throw std::runtime_error("h2m-mod.exe is already running!");
	}

	if (!utils::nt::start_process("h2m-mod.exe"))
	{
		throw std::runtime_error("Failed to start h2m-mod.exe!");
	}
}

int main(const int argc, const char** argv)
{
	try
	{
		if (!check_if_has_mwr())
		{
			return 0;
		}

		updater::run();

		if (utils::question::ask_y_n_question("Files are up to date. Would you like to run H2M-Mod?"))
		{
			start_h2m();
		}
	}
	catch (std::exception& e)
	{
		MessageBoxA(NULL, e.what(), "Error", MB_OK | MB_ICONERROR);
		return -1;
	}
	catch (...)
	{
		return -1;
	}

	return 0;
}