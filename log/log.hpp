#pragma once
#include <iostream>
#include <cstdarg>
#include <print>
#include <string_view>
#ifdef ERROR
#undef ERROR
#endif // ERROR

enum LEVEL : uint8_t {
	INFO,
	ERROR,
	WARNING,
	SUCCES
};

namespace logger {
	__forceinline void LogMessage(LEVEL level, std::string_view format, auto&& ...args) {
		switch (level) {
		case LEVEL::INFO:
			printf("\033[36m[*]> ");
			break;
		case LEVEL::ERROR:
			printf("\033[31m[-]> ");
			break;
		case LEVEL::WARNING:
			printf("\033[93m[!]> ");
			break;
		case LEVEL::SUCCES:
			printf("\033[92m[+]> ");
			break;
		}

		std::cout << std::vformat(format, std::make_format_args(args...)) << "\033[0m\n";
	}
}

#define LOG(level, x, ...) logger::LogMessage(level, x, __VA_ARGS__)