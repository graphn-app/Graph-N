#include "pch.h"
#include "core/base/types.h"

//
//void* allocate_memory(size_t elems, size_t elem_size)
//{
//	void* ptr = std::calloc(elems, elem_size);
//	if (!ptr)
//		throw std::bad_alloc();
//	return ptr;
//}
//
//void deallocate_memory(void* p, size_t elems, size_t elem_size)
//{
//	if (p == nullptr)
//		return;
//
//	::RtlSecureZeroMemory(p, elems * elem_size);
//	std::free(p);
//}
