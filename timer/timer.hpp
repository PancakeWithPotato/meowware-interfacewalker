#pragma once
#include <chrono>
#include <print>

struct Timer {
	Timer(const char* name)
		: name(name) {}

	~Timer() {
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<float> duration{ end - start };
		std::println("[{}] Operation took {:.2f} seconds! \n", name, duration.count());
	}

private:
	std::chrono::steady_clock::time_point start{ std::chrono::high_resolution_clock::now() };
	const char* name;
};