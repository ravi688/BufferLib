#pragma once

#include <bufferlib/buffer.h> // for buffer_t*, buf_get_ptr() and buf_get_element_count()

#include <span>

namespace buflib
{
	template<typename SpanElementType>
	std::span<SpanElementType> GetSpanFromBuffer(buffer_t* buffer) noexcept
	{
		return { static_cast<SpanElementType*>(buf_get_ptr(buffer)), buf_get_element_count(buffer) };
	}
}
