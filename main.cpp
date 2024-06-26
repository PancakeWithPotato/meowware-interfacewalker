#include "log/log.hpp"
#include "process/process.hpp"
#include "timer/timer.hpp"

inline void EnableVirtualTerminalProcessing() {
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD mode = 0;
	GetConsoleMode(console, &mode);

	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(console, mode);
}

int main(int argc, char** argv) {
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

	LOG(INFO, "Selected process: {}", processName.data());

	{
		Timer timer("Interface Walking");
		Process process(processName);
		if (!process.IsHandleOpen())
			return 1;

		process.WalkInterfaces();

		LOG(SUCCES, "Finished interface walking!");
		LOG(INFO, "Written {} interfaces to {}!", process.GetWrittenInterfaceCount(), process.GetFilename().data());
	}

	std::cout << "Press ENTER to exit..\n";
	std::cin.get();
}


