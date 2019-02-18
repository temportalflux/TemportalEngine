#ifndef TYPES_ARCH_H
#define TYPES_ARCH_H

// macros for 32 or 64 bit (should be exclusive)
#if (WIN64 || _x64 || _M_X64 || __LP64__ || __LLP64__ || __arm64__ || __x86_64__)
#define BIT_64	1
#define ARCH	64
#else	// !(WIN64 || x64 || _M_X64 || __LP64__ || __LLP64__ || __arm64__ || __x86_64__)
#define BIT_32	1
#define ARCH	32
#endif	// (WIN64 || x64 || _M_X64 || __LP64__ || __LLP64__ || __arm64__ || __x86_64__)

#endif //TYPES_ARCH_H