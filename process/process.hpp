#pragma once
#include <Windows.h>
#include <unordered_map>
#include <string>
#include "../interfacelist/interfacelist.hpp"
#include <fstream>
#include <array>

#define AUTO_OPEN_PROC 1
#define AUTO_BROWSE_MODULES 1

class Process {
public:
	Process() = default;
	Process(std::string_view procName);

	void GetProcessID();
	bool OpenHandle();

	bool IsHandleOpen() const noexcept { return handle; };

	void PopulateModules();
	std::unordered_map<std::string, uintptr_t>& GetModules() noexcept { return modules; };

	void BrowseEAT();
	uintptr_t GetExport(const std::string& moduleName, const std::string& exportName);

	InterfaceReg GetInterfaceList(const std::string& moduleName);

	void WalkInterfaces();

	void WalkInterfaceRegistry(const char* moduleName, const InterfaceReg* const reg);

	std::string GetFilename() const { return fileName; };

	template <typename T>
	const T Read(uintptr_t address);

	template <typename T>
	bool Write(uintptr_t address, const T& data);

	bool ReadString(uintptr_t address, char* buffer, int size);

	const size_t GetWrittenInterfaceCount() const { return interfaceCount; };

	void CloseLogFile() { dumpFile.close(); };
private:
	HANDLE handle{};
	std::string_view name{};
	DWORD id{};
	size_t interfaceCount{};
	std::ofstream dumpFile{};
	std::string fileName{};

	std::unordered_map<std::string, uintptr_t> modules{};
	std::unordered_map<std::string, std::unordered_map<std::string, uintptr_t>> exports{};
};

template<typename T>
inline const T Process::Read(uintptr_t address) {
	T buffer;

	// TODO: Check for failure
	ReadProcessMemory(handle, reinterpret_cast<void*>(address), &buffer, sizeof(T), nullptr);
	return buffer;
}

