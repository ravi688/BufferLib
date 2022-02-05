

#pragma once

#include <buffer.h>
#include <memory.h> //memcmp

namespace BufferLib
{
	template<typename T>
	class Buffer
	{
	private:
		mutable BUFFER handle;

		static bool defaultComparer(void* value, void* inbufferValue) { return !memcmp(value, inbufferValue, sizeof(T)); }
	public: 

		template<size_t N>
		Buffer(const T (&array)[N])
		{
			handle = buf_create(sizeof(T), N, 0);
			for(size_t i = 0; i < N; i++)
				buf_push(&handle, (T*)(&array[i]));
		}

		Buffer(size_t capacity = 0, size_t offset = 0)
		{
			handle = buf_create(sizeof(T), capacity, offset);
		}
		Buffer(const Buffer& buffer);

		~Buffer() { buf_free(&handle); }

		inline operator T*() { return (T*)buf_get_ptr(&handle); }

		inline void setOnPostResize(void (*onPostResize)(void)) const { buf_set_on_post_resize(&handle, onPostResize); }
		inline void setOnPreResize(void (*onPreResize)(void)) const { buf_set_on_pre_resize(&handle, onPreResize); }
		inline void setOnFree(void (*onFree)(void)) const { buf_set_on_free(&handle, onFree); }
		inline void pushPseudo(const size_t& count) const { buf_push_pseudo(&handle, count); }
		inline void push(const T& value) const { buf_push(&handle, (void*)&value); }
		inline const T pop() const { T value; buf_pop(&handle, (void*)&value); return value; }
		inline void popPsuedo(const size_t& count) const { buf_pop_pseudo(&handle, count); }
		inline void insertPseudo(const size_t& index, const size_t& count) const { buf_insert_pseudo(&handle, index, count); }
		inline void removePseudo(const size_t& index, const size_t& count) const { buf_remove_pseudo(&handle, index, count); }
		inline void setAutoManaged(bool isAutomanaged) const { return buf_set_auto_managed(&handle, isAutoManaged); }
		inline bool isAutoManaged() const { return buf_is_auto_managed(&handle); }
		inline void log() const { buf_log(&handle); }
		inline void traverseElements(size_t startIndex, size_t endIndex, void (*visitor)(void* elementPtr, void* user_data), void* user_data) const { buf_traverse_elements(&handle, startIndex, endIndex, visitor, user_data); }
		inline constexpr bool isStackAllocated() { return true; }
		inline constexpr bool isHeapAllocated() { return false; }
		inline void free() const { buf_free(&handle); }
		inline const Buffer& getClone() const { return buf_get_clone(&handle); }
		inline void moveTo(const Buffer& buffer) const { buf_move_to(&handle, &buffer); }
		inline void copyTo(const Buffer& buffer) const { buf_copy_to(&handle, &buffer); }
		inline size_t getOffset() const { return buf_get_offset(&handle); }
		inline size_t getCapacity() const { return buf_get_capacity(&handle); }
		inline size_t getElementCount() const { return buf_get_element_count(&handle); }
		inline size_t getElementSize() const { return buf_get_element_size(&handle); }
		inline T* getPtr() const { return (T*)buf_get_ptr(&handle); }
		inline const T getAt(const size_t& index) const { T value; buf_get_at(&handle, index, (void*)&value); return value; }
		inline T* getPtrAt(const size_t& index) const { return (T*)buf_get_ptr_at(&handle, index); }
		inline void setAt(const size_t& index, const T& value) const { buf_set_at(&handle, index, (void*)&value); }
		inline void* getOffsetBytes() const { return buf_get_offset_bytes(&handle); }
		inline void setOffsetBytes(void* bytes) const { return buf_set_offset_bytes(&handle, bytes); }
		inline void getSize() const { return buf_get_buffer_size(&handle); }
		inline void resize(const size_t& size) const { buf_resize(&handle, size); }
		inline void clear(T value = 0) const { buf_clear(&handle, (void*)&value); }
		inline void clearBuffer(T value) const { buf_clear_buffer(&handle, (void*)&value); }
		inline void insertAtNoAlloc(const size_t& index, const T& value) { buf_insert_at_noalloc(&handle, index, (void*)&value); }
		inline void insertAt(const size_t& index, const T& value) { buf_insert_at(&handle, index, (void*)&value); }
		inline void removeAtNoShift(const size_t& index) const { buf_remove_at_noshift(&handle, index); }
		inline void removeAt(const size_t& index) const { buf_remove_at(&handle, index); }
		inline void removeNoShift(const T& value, bool (*comparer)(void*, void*) = defaultComparer) const { buf_remove_noshift(&handle, (void*)&value, comparer); }
		inline void remove(const T& value, bool (*comparer)(void*, void*) = defaultComparer) const { buf_remove(&handle, (void*)&value, comparer); }
		inline void fit() const { buf_fit(&handle); }
		inline size_t findIndexOf(const T& value, bool (*comparer)(void*, void*) = defaultComparer) const { return (size_t)buf_find_index_of(&handle, (void*)&value, comparer); }
		inline void pushv(const T* values, const size_t& count) const { buf_pushv(&handle, (void*)values, count); }
		inline void setOffset(const size_t& offset) const { buf_set_offset(&handle, offset); }
		inline void setCapacity(const size_t& capacity) const { buf_set_capacity(&handle, capacity); }
		inline void setElementCount(const size_t& elementCount) const { buf_set_element_count(&handle, elementCount); }
		inline void setElementSize(const size_t& elementSize) const { buf_set_element_size(&handle, elementSize); }
		inline void setPtr(T* ptr) const { buf_set_ptr(&handle, (void*)ptr); }
		inline const T& peek() const { return *(T*)buf_peek_ptr(&handle); }
		inline T* peekPtr() const { return (T*)buf_peek_ptr(&handle); }
		inline const T operator [](const size_t& index) const { return getAt(index); }

		const T* end() const { return (const T*)peekPtr() + 1; }
		const T* begin() const { return (const T*)getPtr(); }
	};
}

