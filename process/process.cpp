#include "process.hpp"
#include <TlHelp32.h>
#include "../log/log.hpp"
#include <filesystem>
#include <ranges>

#ifdef x86
#define OPTIONAL_HEADER IMAGE_OPTIONAL_HEADER32
#else
#define OPTIONAL_HEADER IMAGE_OPTIONAL_HEADER64
#endif

#ifdef x86
#define DECIDE_BYTE 3
#define REGISTRY_ADDRESS_INLINE 6
#define REGISTRY_ADDRESS 6
#define INTERNAL_CALL_RVA 5
#define OPCODE_AND_INTERNAL_CALL_RVA 9
#else // Tell me about a game that does NOT inline CreateInterfaceInternal on 64 bit
#define DECIDE_BYTE 1 // not checked
#define REGISTRY_ADDRESS_INLINE 3
#define REGISTRY_ADDRESS 6 // not checked
#define INTERNAL_CALL_RVA 5 // not checked
#define OPCODE_AND_INTERNAL_CALL_RVA 9 // not checked
#endif

// Not the opcode for call; if this is the "decide" byte of the function tho, CreateInterfaceInternal wasn't inlined
#define CALL_BYTE 0x5D
constexpr uint8_t PointerSize = sizeof(void*);
constexpr uint8_t RvaSize = sizeof(DWORD);

Process::Process(std::string_view procName)
	: name(procName)
{
	std::filesystem::create_directory("logs");

	fileName = std::string("logs//");
	fileName.append(procName);
	fileName += "_log.txt";

	dumpFile.open(fileName);
	if (!dumpFile.is_open())
		LOG(ERROR, "Failed to open log file!");

#ifdef AUTO_OPEN_PROC
	GetProcessID();
	if (!OpenHandle()) {
		LOG(ERROR, "Failed to open handle to process %s!", procName);
		return;
	}
#endif // AUTO_OPEN_PROC

#ifdef AUTO_BROWSE_MODULES
	PopulateModules();
	BrowseEAT();
#endif // AUTO_BROWSE_MODULES
}

void Process::GetProcessID() {
	if (id)
		return;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!snapshot) {
		LOG(ERROR, "Failed to open snapshot handle!");
		return;
	}

	PROCESSENTRY32 pe{ .dwSize = sizeof(PROCESSENTRY32) };
	if (!Process32First(snapshot, &pe)) {
		LOG(ERROR, "Failed to parse the snapshot!");
		return;
	}

	while (Process32Next(snapshot, &pe)) {
		if (std::ranges::equal(name, std::string_view(pe.szExeFile), [](unsigned char lhs, unsigned char rhs) { return std::tolower(lhs) == std::tolower(rhs); })) {
			LOG(SUCCES, "Found %s, id: %d", name, pe.th32ProcessID);
			id = pe.th32ProcessID;
			return;
		}
	}

}

bool Process::OpenHandle() {
	handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, id);
	return handle;
}

void Process::PopulateModules() {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, id);
	if (!snapshot) {
		LOG(ERROR, "Failed to get module list!");
		return;
	}

	MODULEENTRY32 me{ .dwSize = sizeof(MODULEENTRY32) };

	if (!Module32First(snapshot, &me)) {
		LOG(ERROR, "Failed to get the first module in list");
		return;
	}

	while (Module32Next(snapshot, &me)) {
		// I am lazy and I do not wanna fix this (it doesn't have a traditional interfacelist)
		if (!strcmp(me.szModule, "crashhandler.dll"))
			continue;

		modules.emplace(me.szModule, reinterpret_cast<uintptr_t>(me.hModule));
	}



	if (modules.empty())
		LOG(ERROR, "Failed to grab modules for %s process!", name);
}

void Process::BrowseEAT() {
	for (const auto& [moduleName, moduleAddress] : modules) {
		const auto dHeader = Read<IMAGE_DOS_HEADER>(moduleAddress);
		const auto ntHeader = Read<IMAGE_NT_HEADERS>(moduleAddress + dHeader.e_lfanew);

		if (ntHeader.Signature != IMAGE_NT_SIGNATURE)
			continue;

		OPTIONAL_HEADER optionalHeader = ntHeader.OptionalHeader;

		const IMAGE_EXPORT_DIRECTORY exportDirectory = Read<IMAGE_EXPORT_DIRECTORY>(moduleAddress + optionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		LOG(WARNING, "Module %s has %d exports", moduleName.data(), exportDirectory.NumberOfNames);
		const uintptr_t namesTable = moduleAddress + exportDirectory.AddressOfNames;
		const uintptr_t functionsTable = moduleAddress + exportDirectory.AddressOfFunctions;
		const uintptr_t ordinalsTable = moduleAddress + exportDirectory.AddressOfNameOrdinals;

		for (unsigned int i = 0; i < exportDirectory.NumberOfNames; i++) {
			const auto nameRva = Read<DWORD>(namesTable + i * RvaSize);

			std::array<char, 125> exportName;
			ReadString(moduleAddress + nameRva, exportName.data(), exportName.size());

			// Technically the "proper" way to do it, works flawlessly without this tho, and just adding i * RvaSize but oh well
			const auto ordinal = Read<uint16_t>(ordinalsTable + i * sizeof(uint16_t));

			const auto exportAddress = Read<DWORD>(functionsTable + ordinal * RvaSize);

			exports[moduleName].emplace(exportName.data(), exportAddress);
		}

	}


}

uintptr_t Process::GetExport(const std::string& module, const std::string& exportName) {
	if (!exports.contains(module))
		return 0;

	if (!exports.at(module).contains(exportName))
		return 0;

	return exports.at(module).at(exportName);
}

InterfaceReg Process::GetInterfaceList(const std::string& moduleName) {
	const uintptr_t moduleAddress = modules[moduleName];
	if (!moduleAddress)
		return InterfaceReg();

	uintptr_t createInterface = GetExport(moduleName, "CreateInterface");
	if (!createInterface) {
		LOG(WARNING, "Module %s doesn't export CreateInterface!", moduleName.data());
		return InterfaceReg();
	}

	createInterface += moduleAddress; // Get the VA

	const char decideByte = Read<char>(createInterface + DECIDE_BYTE);
	if (decideByte != CALL_BYTE) {
		// Inlined CreateInterfaceInternal
		LOG(INFO, "%s inlines CreateInterfaceInternal!", moduleName.data());
		// I lowkey hate this part :sob:
#ifdef x86
		const uintptr_t interfaceRegistryAddress = Read<uintptr_t>(Read<uintptr_t>(createInterface + REGISTRY_ADDRESS_INLINE));
		return InterfaceReg(Read<uintptr_t>(interfaceRegistryAddress), Read<uintptr_t>(interfaceRegistryAddress + PointerSize),
			Read<uintptr_t>(interfaceRegistryAddress + 2 * PointerSize));
#else
		const DWORD interfaceRegistryRva = Read<DWORD>(createInterface + REGISTRY_ADDRESS_INLINE);
		const uintptr_t interfaceRegistryAddress = Read<uintptr_t>(createInterface + 7 + interfaceRegistryRva);
		return InterfaceReg(Read<uintptr_t>(interfaceRegistryAddress), Read<uintptr_t>(interfaceRegistryAddress + PointerSize),
			Read<uintptr_t>(interfaceRegistryAddress + 2 * PointerSize));
#endif
	}

	// Call to CreateInterfaceInternal is not inlined
	const uintptr_t internalRva = Read<DWORD>(createInterface + INTERNAL_CALL_RVA);
	const uintptr_t internalAddress = createInterface + OPCODE_AND_INTERNAL_CALL_RVA + internalRva;
	const uintptr_t interfaceRegistryAddress = Read<uintptr_t>(Read<uintptr_t>(internalAddress + REGISTRY_ADDRESS));

	if (!interfaceRegistryAddress)
		return InterfaceReg();


	return InterfaceReg(Read<uintptr_t>(interfaceRegistryAddress), Read<uintptr_t>(interfaceRegistryAddress + PointerSize),
		Read<uintptr_t>(interfaceRegistryAddress + 2 * PointerSize));
}

void Process::WalkInterfaces() {
	for (const auto& [moduleName, moduleAddress] : modules) {
		const InterfaceReg reg = GetInterfaceList(moduleName);

		if (reg.createFN)
			WalkInterfaceRegistry(moduleName.data(), &reg);
	}
}

void Process::WalkInterfaceRegistry(const char* moduleName, const InterfaceReg* const reg) {
	InterfaceReg current = *reg;
	dumpFile << "MODULE: " << moduleName << '\n';

	do
	{
		std::array<char, 125> interfaceName;
		ReadString(current.name, interfaceName.data(), interfaceName.size());

		LOG(SUCCES, "[%s] Interface Name: %s, next interface: 0x%X", moduleName, interfaceName.data(), current.next);

		interfaceCount++;
		dumpFile << "    -" << interfaceName.data() << '\n';

		if (!current.next)
			break;

		// Read the next one
		current = InterfaceReg(current.next, Read<uintptr_t>(current.next + PointerSize), Read<uintptr_t>(current.next + 2 * PointerSize));
	} while (1 /*lol this code is ugly*/);
	dumpFile << "\n\n";
}

bool Process::ReadString(uintptr_t address, char* buffer, int size) {
	return ReadProcessMemory(handle, reinterpret_cast<void*>(address), buffer, size, nullptr);
}

