#pragma once
#include <iostream>
#include <cstdarg>
#ifdef ERROR
#undef ERROR
#endif // ERROR

enum LEVEL {
	INFO,
	ERROR,
	WARNING,
	SUCCES
};

namespace logger {
	__forceinline void LogMessage(LEVEL level, const char* format, ...) {
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

		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		printf("\033[0m\n");
	}
}

#define LOG(level, x, ...) logger::LogMessage(level, x, __VA_ARGS__)