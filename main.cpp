#include "log.hpp"
#include "process.hpp"
#include "timer.hpp"

static void EnableVirtualTerminalProcessing() {
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD mode = 0;
	GetConsoleMode(console, &mode);

	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(console, mode);
}

int main(int argc, char** argv) {
	// Enable virtual terminal processing!
	EnableVirtualTerminalProcessing();

	LOG(INFO, "Interface dumper/logger by Pancake!");

	std::string processName{};
	if (argc < 2) {
		LOG(WARNING, "No process name was supplied through argv! Please input the process name manually below:");
		std::cin >> processName;
		while (std::cin.get() != '\n');
	}
	else
		processName = argv[1];

	LOG(INFO, "Selected process: %s", processName.data());

	Process process(processName.data());
	if (!process.IsHandleOpen())
		return 1;

	process.WalkInterfaces();

	LOG(SUCCES, "Finished interface walking!");
	LOG(INFO, "Written %d interfaces to %s!", process.GetWrittenInterfaceCount(), std::string("logs/" + processName + "_log.txt").data());

	process.CloseLogFile();

	std::cout << "Press ENTER to exit..\n";
	std::cin.get();
}


