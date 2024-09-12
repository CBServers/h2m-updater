#include <std_include.hpp>

#include "updater/updater.hpp"
#include <utils/io.hpp>
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
		return;
	}

	const char* command = "h2m-mod.exe";
	// Initialize the STARTUPINFO structure
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Create the process
	if (!CreateProcess(NULL,   // No module name (use command line)
		(LPSTR)command,        // Command line
		NULL,                  // Process handle not inheritable
		NULL,                  // Thread handle not inheritable
		FALSE,                 // Set handle inheritance to FALSE
		0,                     // No creation flags
		NULL,                  // Use parent's environment block
		NULL,                  // Use parent's starting directory 
		&si,                   // Pointer to STARTUPINFO structure
		&pi)                   // Pointer to PROCESS_INFORMATION structure
		) {
		std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
		return;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

int main(const int argc, const char** argv)
{
	try
	{
		if(check_if_has_mwr())
		{
			updater::run();
		}
		
		start_h2m();
	}
	catch (std::exception& e)
	{
		MessageBoxA(NULL, e.what(), "Error", MB_OK | MB_ICONERROR);
		return -1;
	}

	return 0;
}