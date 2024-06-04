#pragma once

struct InterfaceReg {
	InterfaceReg() = default;
	InterfaceReg(uintptr_t createFn, uintptr_t name, uintptr_t next)
		: createFN(createFn), name(name), next(next) {}

	uintptr_t createFN;
	uintptr_t name;
	uintptr_t next;
};