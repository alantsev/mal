#include "arena.h"

namespace
{
	const static uint32_t MAX_BLOCK_SIZE = 8 * 1024;
}

///////////////////////////////
/// arena class
///////////////////////////////
void* 
arena::allocate (uint32_t bytes)
{
	if (bytes > m_remaining_bytes)
		return allocate_fallback (bytes);

	void * retVal = m_current;
	m_current += bytes;
	m_remaining_bytes -= bytes;

	return retVal;
}

///////////////////////////////
void* 
arena::allocate_alligned (uint32_t bytes)
{
	constexpr uint32_t allign = sizeof (void*);
	static_assert ((allign & (allign - 1)) == 0, "size of pointer should be power of 2");

	const size_t offset = MAX_BLOCK_SIZE - m_remaining_bytes;

	// offset = offset + (allign - offset % allign) % allign;
	size_t shift = (allign - offset & (allign - 1)) & (allign - 1);
	size_t needed = shift + bytes;
	if (needed > m_remaining_bytes)
		return allocate_fallback (bytes);

	void * retVal = m_current + offset;
	m_current += needed;
	m_remaining_bytes -= needed;
	return retVal;
}

///////////////////////////////
uint8_t*
arena::allocate_fallback (uint32_t bytes)
{
	if (bytes > MAX_BLOCK_SIZE / 4)
	{
		return append_new_buffer (bytes);
	}

	m_current = append_new_buffer (MAX_BLOCK_SIZE);
	m_remaining_bytes = MAX_BLOCK_SIZE;
	return m_current;
}

///////////////////////////////
uint8_t*
arena::append_new_buffer (uint32_t bytes)
{
	m_buffers.emplace_back (new uint8_t[bytes]);
	return m_buffers.back ().get ();
}
