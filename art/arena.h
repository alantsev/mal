#pragma once

#include <vector>
#include <memory>

///////////////////////////////
class arena final
{
public:
	arena (arena&&) = default;
	arena& operator = (arena&&) = default;

	void* allocate (uint32_t bytes);
	void* allocate_alligned (uint32_t bytes);

private:
	arena (const arena&) = delete;
	arena& operator = (const arena&) = delete;

	uint8_t* allocate_fallback (uint32_t bytes);
	uint8_t* append_new_buffer (uint32_t bytes);

	using buffer_t = std::unique_ptr<uint8_t[]>;
	std::vector<buffer_t> m_buffers;
	uint8_t* m_current = nullptr;
	uint32_t m_remaining_bytes = 0;
};
