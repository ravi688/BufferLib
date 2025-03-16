#include <bufferlib/buffer.h>				//This must be included at the top for preprocessing reasons


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h> /* for PRI* macros */
#if defined(__linux__)
#	include <alloca.h> /* for alloca() */
#endif

#define log_fetal_error printf

#ifdef BUF_DEBUG
#	define GOOD_ASSERT(bool_value, string) do { if(!(bool_value)) {  printf("Assertion Failed: %s\n", string); __builtin_trap(); } } while(false)
	static void check_pre_condition(BUFFER* buffer);
#else
#	define GOOD_ASSERT(...)
#	define check_pre_condition(buffer)
#endif

#define STACK_ALLOCATED_OBJECT  0x1
#define HEAP_ALLOCATED_OBJECT 	 0x2
#define STATIC_BUFFER 0x4
#define READONLY_BUFFER 0x8

static BUFFER* binded_buffer;

static bool buf_is_stack_allocated(BUFFER* buffer);
static bool buf_is_heap_allocated(BUFFER* buffer);
static bool BUFis_stack_allocated();
static bool BUFis_heap_allocated();

uint64_t BUFget_buffer_object_size() { return sizeof(BUFFER); }

void BUFset_on_post_resize(void (*on_post_resize)(void)) { buf_set_on_post_resize(binded_buffer, on_post_resize); }
void buf_set_on_post_resize(BUFFER* buffer, void (*on_post_resize)(void))
{
	check_pre_condition(buffer);
	buffer->on_post_resize = on_post_resize;
}

void BUFset_on_pre_resize(void (*on_pre_resize)(void)) { buf_set_on_pre_resize(binded_buffer, on_pre_resize); }
void buf_set_on_pre_resize(BUFFER* buffer, void (*on_pre_resize)(void))
{
	check_pre_condition(buffer);
	buffer->on_pre_resize = on_pre_resize;
}

static inline void* call_malloc(buffer_t* buffer, buf_ucount_t size)
{
	if(buffer->mem_malloc != NULL)
		return buffer->mem_malloc(size, buffer->user_data);
	return malloc(size);
}

static inline void call_free(buffer_t* buffer, void* ptr)
{
	if(buffer->mem_free != NULL)
	{
		buffer->mem_free(ptr, buffer->user_data);
		return;
	}
	free(ptr);
}

static inline void* call_realloc(buffer_t* buffer, void* old_ptr, buf_ucount_t size)
{
	if(buffer->mem_realloc != NULL)
		return buffer->mem_realloc(old_ptr, size, buffer->user_data);
	return realloc(old_ptr, size);
}

void BUFpush_pseudo(buf_ucount_t count) { buf_push_pseudo(binded_buffer, count); }
void buf_push_pseudo(BUFFER* buffer, buf_ucount_t count)
{
	check_pre_condition(buffer);

	if(count == 0)
		return;

	buf_ensure_capacity(buffer, buffer->element_count + count);

	memset(buffer->bytes + buffer->element_count * buffer->element_size , 0 , buffer->element_size * count);
	
	buffer->element_count += count;
}

void* BUFpush_pseudo_get(buf_ucount_t count) { return buf_push_pseudo_get(binded_buffer, count); }
void* buf_push_pseudo_get(BUFFER* buffer, buf_ucount_t count)
{
	check_pre_condition(buffer);

	if(count == 0)
		return NULL;

	buf_ucount_t offset = buf_get_element_count(buffer);
	buf_push_pseudo(buffer, count);

	return buf_get_ptr_at(buffer, offset);
}

void BUFpop_pseudo(buf_ucount_t count) { buf_pop_pseudo(binded_buffer, count); }
void buf_pop_pseudo(BUFFER* buffer, buf_ucount_t count)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(count <= buffer->element_count, "Buffer Underflow Exception");
	buffer->element_count -= count;
}


static void buf_insert_pseudo_uninitialized(BUFFER* buffer, buf_ucount_t index, buf_ucount_t count)
{
	GOOD_ASSERT(buffer != NULL, "Binded Buffer Is NULL Exception");
	GOOD_ASSERT(index <= buffer->element_count,"Buffer Overflow Exception");

	buf_ensure_capacity(buffer, buffer->element_count + count);

	for(buf_ucount_t i = buffer->element_count; i > index; --i)
	{
		void* dst_ptr = buffer->bytes + (count + i - 1) * buffer->element_size;
		void* src_ptr = dst_ptr - count * buffer->element_size;
		memcpy(dst_ptr, src_ptr, buffer->element_size);
	}

	/* increase the element_count by count */
	buffer->element_count += count;

}

void BUFinsert_pseudo(buf_ucount_t index, buf_ucount_t count) { buf_insert_pseudo(binded_buffer, index, count); }
void buf_insert_pseudo(BUFFER* buffer, buf_ucount_t index, buf_ucount_t count)
{
	buf_insert_pseudo_uninitialized(buffer, index, count);
	/* set the inserted elements to zero */
	memset(buf_get_ptr_at(buffer, index), 0, count * buffer->element_size);
}

void BUFremove_pseudo(buf_ucount_t index, buf_ucount_t count) { buf_remove_pseudo(binded_buffer, index, count);  }
void buf_remove_pseudo(BUFFER* buffer, buf_ucount_t index, buf_ucount_t count)
{
	check_pre_condition(buffer);
	GOOD_ASSERT((index + count) <= buffer->element_count, "You're trying to remove elements out of the bounds of the buffer");

	/* if there is any element left on the right most side */
	if((index + count) < buffer->element_count)
	{
		/* shift the elements to the left */
		void* dst_ptr = buffer->bytes + index * buffer->element_size;
		void* src_ptr = dst_ptr + count * buffer->element_size;
		memmove(dst_ptr, src_ptr, (buffer->element_count - index - count) * buffer->element_size);
	}

	/* zero out the trailing elements (on the left) */
	memset(buffer->bytes + (buffer->element_count - count) * buffer->element_size, 0, count * buffer->element_size);

	buffer->element_count -= count;

}

void BUFset_auto_managed(bool value) { buf_set_auto_managed(binded_buffer, value); }
void buf_set_auto_managed(BUFFER* buffer, bool value)
{
	check_pre_condition(buffer);
	if(value && (buffer->auto_managed_empty_blocks == BUF_INVALID))
			buffer->auto_managed_empty_blocks = BUFcreate(BUF_INVALID, sizeof(void*), 0, 0);
	else if(!value && (buffer->auto_managed_empty_blocks != BUF_INVALID))
				buf_free(buffer->auto_managed_empty_blocks);
 	buffer->is_auto_managed = value;
}

buf_ucount_t BUFget_offset() { buf_get_offset(binded_buffer); }
buf_ucount_t buf_get_offset(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->offset;
}

buf_ucount_t BUFget_capacity() { buf_get_capacity(binded_buffer); }
buf_ucount_t buf_get_capacity(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->capacity;
}

buf_ucount_t BUFget_element_count() {  buf_get_element_count(binded_buffer); }
buf_ucount_t buf_get_element_count(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->element_count;
}

void* buf_get_malloc_callback_user_data(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->user_data;
}

buf_malloc_t BUFget_malloc_callback() { return buf_get_malloc_callback(binded_buffer); }
buf_malloc_t buf_get_malloc_callback(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->mem_malloc;
}

buf_free_t BUFget_free_callback() { return buf_get_free_callback(binded_buffer); }
buf_free_t buf_get_free_callback(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->mem_free;
}

buf_realloc_t BUFget_realloc_callback() { return buf_get_realloc_callback(binded_buffer); }
buf_realloc_t buf_get_realloc_callback(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->mem_realloc;
}

buf_ucount_t BUFget_element_size() { buf_get_element_size(binded_buffer); }
buf_ucount_t buf_get_element_size(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->element_size;
}

void* BUFget_ptr() { return buf_get_ptr(binded_buffer); }
void* buf_get_ptr(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->bytes;
}

void* BUFget_ptr_end() { return buf_get_ptr_end(binded_buffer); }
void* buf_get_ptr_end(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->bytes + buffer->element_count * buffer->element_size;
}

void BUFset_offset(buf_ucount_t offset) { buf_set_offset(binded_buffer, offset); }
void buf_set_offset(BUFFER* buffer, buf_ucount_t offset)
{
	check_pre_condition(buffer);
	buffer->offset = offset;
}

void BUFset_capacity(buf_ucount_t capacity) { buf_set_capacity(binded_buffer, capacity); }
void buf_set_capacity(BUFFER* buffer, buf_ucount_t capacity)
{
	check_pre_condition(buffer);
 	buffer->capacity = capacity;

}

void BUFset_element_count(buf_ucount_t element_count) { buf_set_element_count(binded_buffer, element_count); }
void buf_set_element_count(BUFFER* buffer, buf_ucount_t element_count)
{
	check_pre_condition(buffer);
	buffer->element_count = element_count;
}

void BUFset_element_size(buf_ucount_t element_size) { buf_set_element_size(binded_buffer, element_size); }
void buf_set_element_size(BUFFER* buffer, buf_ucount_t element_size)
{
	check_pre_condition(buffer);
	buffer->element_size = element_size;
}

void BUFset_ptr(void* ptr) { buf_set_ptr(binded_buffer, ptr); }
void buf_set_ptr(BUFFER* buffer, void* ptr)
{
	check_pre_condition(buffer);
	buffer->bytes = ptr;
}

BUFFER* BUFget_binded_buffer()
{
	return binded_buffer;
}

bool BUFis_auto_managed() { return buf_is_auto_managed(binded_buffer); }
bool buf_is_auto_managed(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->is_auto_managed;
}

void BUFbind(BUFFER* buffer)
{
  binded_buffer = buffer;
}

void BUFunbind()
{
	binded_buffer = NULL;
}

void BUFlog() { buf_log(binded_buffer); }
void buf_log(BUFFER* buffer)
{
	printf(
			 "----------------------\n"
			 "ElementCount :\t%u\n"
		   "Capacity :\t%u\n"
		   "ElementSize :\t%u\n"
		   "Offset: \t%u\n"
		   "----------------------\n",
		    buffer->element_count,
		    buffer->capacity,
		    buffer->element_size,
		    buffer->offset
			);
}

void BUFtraverse_elements(buf_ucount_t start, buf_ucount_t end, void (*func)(void* /*element ptr*/, void* /*args ptr*/), void* args) { buf_traverse_elements(binded_buffer, start, end, func, args); }
void buf_traverse_elements(BUFFER* buffer, buf_ucount_t start, buf_ucount_t end, void (*func)(void* /*element ptr*/, void* /*args ptr*/), void* args)
{
	check_pre_condition(buffer);
	GOOD_ASSERT((start <= end) && (start < buffer->element_count) && (end < buffer->element_count), "(start <= end) && (start < buffer->element_count) && (end < buffer->element_count) evaulates to false");
	for(buf_ucount_t i = start; i <= end; i++)
	 		func(buf_getptr_at(buffer, i), args);
}

__attribute__((unused)) static bool BUFis_stack_allocated() {  return buf_is_stack_allocated(binded_buffer); }
__attribute__((unused)) static bool buf_is_stack_allocated(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return (buffer->info & STACK_ALLOCATED_OBJECT) == STACK_ALLOCATED_OBJECT;
}

__attribute__((unused)) static bool BUFis_heap_allocated() { return buf_is_heap_allocated(binded_buffer); }
__attribute__((unused)) static bool buf_is_heap_allocated(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return (buffer->info & HEAP_ALLOCATED_OBJECT) == HEAP_ALLOCATED_OBJECT;
}

void BUFfree() { buf_free(binded_buffer); }
void buf_free(BUFFER* buffer)
{
	check_pre_condition(buffer);
	if((buffer->free != NULL) && (buffer->element_count > 0))
		buf_traverse_elements(buffer, 0, buf_get_element_count(buffer)- 1, (void (*)(void*, void*))(buffer->free), NULL);
	if((buffer->bytes != NULL) && ((buffer->info & STATIC_BUFFER) == 0))
	{
		call_free(buffer, buffer->bytes);
		buffer->bytes = NULL;
	}

  if(buffer->info & HEAP_ALLOCATED_OBJECT)
  { buffer->info = 0x00; call_free(buffer, buffer); }
	buffer = NULL;
}

void BUFfree_except_data() { buf_free_except_data(binded_buffer); }
void buf_free_except_data(BUFFER* buffer)
{
	check_pre_condition(buffer);
  if(buffer->info & HEAP_ALLOCATED_OBJECT)
  { buffer->info = 0x00; call_free(buffer, buffer); }
	buffer = NULL;
}

BUFFER* BUFget_clone()
{
	GOOD_ASSERT(binded_buffer != NULL, "binded buffer is NULL Exception");
	return BUFcopy_construct(binded_buffer);
}
BUFFER buf_get_clone(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buf_copy_construct(buffer);
}

void BUFmove_to(BUFFER* destination) { buf_move_to(binded_buffer, destination); }
void buf_move_to(BUFFER* buffer, BUFFER* destination)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(destination != NULL, "destination buffer is NULL Exception");
	buf_copy_to(buffer, destination);
	buf_free(buffer);
}

void BUFmove(BUFFER* destination) { buf_move(binded_buffer, destination); }
void buf_move(BUFFER* buffer, BUFFER* destination)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(destination != NULL, "destination buffer is NULL Exception");
	memcpy(destination, buffer, sizeof(BUFFER));
	buffer->bytes = NULL;
	buffer->element_size = 0;
	buffer->element_count = 0;
	buffer->capacity = 0;
	buffer->auto_managed_empty_blocks = NULL;
	buffer->offset = 0;
}

void BUFcopy_to(BUFFER* destination) { buf_copy_to(binded_buffer, destination); }
void buf_copy_to(BUFFER* buffer, BUFFER* destination)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(destination != NULL, "destination buffer is NULL Exception");
	GOOD_ASSERT(buffer != destination, "source and destination buffers are referencing to the same memory location");
	GOOD_ASSERT(destination->element_size == buffer->element_size, "element size of the source and destination buffers are not identical");
	GOOD_ASSERT(buf_get_buffer_size(buffer) != 0, "buffer size of the source buffer is zero");
	if(destination->capacity < buffer->element_count)
		buf_resize(destination, buffer->element_count);
	memcpy(destination->bytes, buffer->bytes, buf_get_buffer_size(buffer));
	buf_set_offset(destination, buffer->offset);
	buf_set_capacity(destination, buffer->capacity);
	buf_set_element_count(destination, buffer->element_count);
	buf_set_auto_managed(destination, buffer->is_auto_managed);
	buf_set_on_pre_resize(destination, buffer->on_pre_resize);
	buf_set_on_post_resize(destination, buffer->on_post_resize);
	buf_set_on_free(destination, buffer->free);
}
BUFFER* BUFcopy_construct(BUFFER* source)
{
	GOOD_ASSERT(source != NULL, "source buffer Is NULL Exception");
	BUFFER* buffer = BUFcreate(NULL, source->element_size, source->capacity, source->offset);
	buf_copy_to(source, buffer);
	return buffer;
}

BUFFER buf_copy_construct(BUFFER* source)
{
	GOOD_ASSERT(source != NULL, "source buffer is NULL Exception");
	BUFFER buffer = buf_create(source->element_size, source->capacity, source->offset);
	buf_copy_to(source, &buffer);
	return buffer;
}

void BUFset_on_free(void (*free)(void*)) { buf_set_on_free(binded_buffer, free); }
void buf_set_on_free(BUFFER* buffer, void (*free)(void*))
{
	check_pre_condition(buffer);
	buffer->free = free;
}

BUFFER buf_create_a(buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	BUFFER buffer;
	BUFcreate_object_a(&buffer, _malloc, _free, _realloc, user_data);
	BUFcreate_a(&buffer, element_size, capacity, offset, _malloc, _free, _realloc, user_data);
	return buffer;
}

BUFFER buf_create(buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset)
{
	BUFFER buffer;
	BUFcreate_object(&buffer);
	BUFcreate(&buffer, element_size, capacity, offset);
	return buffer;
}

BUFFER buf_create_m(void* ptr, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	BUFFER buffer;
	BUFcreate_object(&buffer);
	BUFcreate_m(&buffer, ptr, element_size, capacity, offset, _malloc, _free, _realloc, user_data);
	return buffer;
}

BUFFER buf_create_r(void* ptr, buf_ucount_t element_size, buf_ucount_t element_count, buf_ucount_t offset, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	BUFFER buffer;
	BUFcreate_object(&buffer);
	BUFcreate_r(&buffer, ptr, element_size, element_count, offset, _malloc, _free, _realloc, user_data);
	return buffer;
}

bool buf_is_static(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return (buffer->info & STATIC_BUFFER) == STATIC_BUFFER;
}

bool buf_is_readonly(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return (buffer->info & READONLY_BUFFER) == READONLY_BUFFER;
}

static void* buf_default_malloc(buf_ucount_t size, void* user_data)
{
	return malloc(size);
}

static void* buf_default_realloc(void* prev_ptr, buf_ucount_t new_size, void* user_data)
{
	return realloc(prev_ptr, new_size);
}

static void buf_default_free(void* ptr, void* user_data)
{
	free(ptr);
}

static void initialize_buffer_object(buffer_t* buffer, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	buffer->bytes = NULL;
	buffer->info = 0x00;
	buffer->element_size = 0;
	buffer->element_count = 0;
	buffer->capacity = 0;
	buffer->auto_managed_empty_blocks = NULL;
	buffer->is_auto_managed = false;
	buffer->on_pre_resize = NULL;
	buffer->on_post_resize = NULL;
	buffer->offset = 0;
	buffer->free = NULL;
	buffer->mem_malloc = _malloc ? _malloc : buf_default_malloc;
	buffer->mem_free = _free ? _free : buf_default_free;
	buffer->mem_realloc = _realloc ? _realloc : buf_default_realloc;
	buffer->user_data = user_data;
#ifdef BUF_DEBUG
	buffer->is_ptr_queried = false;
#endif /* BUF_DEBUG */	
}

BUFFER* BUFcreate_object_a(void* bytes, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	GOOD_ASSERT(bytes != NULL, "bytes equals to NULL");
	BUFFER* buffer = bytes;
	initialize_buffer_object(buffer, _malloc, _free, _realloc, user_data);
	buffer->info |= STACK_ALLOCATED_OBJECT;
	return buffer;
}

//TODO: Replace the name with BUFcreate_object_from_bytes
BUFFER* BUFcreate_object(void* bytes)
{
	BUFFER* buffer = BUFcreate_object_a(bytes, NULL, NULL, NULL, NULL);
	return buffer;
}

static buffer_t* create_buffer_object(buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	buffer_t* buffer = (_malloc != NULL) ? _malloc(sizeof(BUFFER), user_data) : buf_default_malloc(sizeof(BUFFER), NULL);
	GOOD_ASSERT(buffer != NULL, "Memory Allocation Failure Exception");
	initialize_buffer_object(buffer, _malloc, _free, _realloc, user_data);
	buffer->info |= HEAP_ALLOCATED_OBJECT;
	return buffer;
}

BUFFER* BUFcreate_a(BUFFER* buffer, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	GOOD_ASSERT(((int64_t)element_size) > 0, "element_size cannot be negative or zero");
	GOOD_ASSERT(((int64_t)capacity) >= 0, "capacity cannot be negative");
	GOOD_ASSERT(((int64_t)offset) >= 0, "offset cannot be negative");
	if(buffer == NULL)
		buffer = create_buffer_object(_malloc, _free, _realloc, user_data);
	if((capacity > 0) || (offset > 0))
	{
		buf_ucount_t alloc_size = element_size * capacity + offset;
		buffer->bytes = (void*)((_malloc != NULL) ? _malloc(alloc_size, user_data) : malloc(alloc_size));
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	}
	else
		buffer->bytes = NULL;
	buffer->element_size = element_size;
	buffer->capacity = capacity;
	buffer->element_count = 0;
	buffer->offset = offset;
	return buffer;
}

BUFFER* BUFcreate(BUFFER* buffer, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset)
{
	return BUFcreate_a(buffer, element_size, capacity, offset, NULL, NULL, NULL, NULL);
}

BUFFER* BUFcreate_m(BUFFER* buffer, void* ptr, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	GOOD_ASSERT(((int64_t)element_size) > 0, "element_size cannot be negative or zero");
	GOOD_ASSERT(((int64_t)capacity) > 0, "capacity cannot be zero");
	GOOD_ASSERT(((int64_t)offset) >= 0, "offset cannot be negative");
	GOOD_ASSERT(ptr != NULL, "pointer to map to can't be NULL");
	if(buffer == NULL)
		buffer = create_buffer_object(_malloc, _free, _realloc, user_data);
	buffer->info |= STATIC_BUFFER;
	buffer->bytes = ptr;
	buffer->element_size = element_size;
	buffer->capacity = capacity;
	buffer->element_count = 0;
	buffer->offset = offset;
	return buffer;
}

BUFFER* BUFcreate_r(BUFFER* buffer, void* ptr, buf_ucount_t element_size, buf_ucount_t element_count, buf_ucount_t offset, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	buffer = BUFcreate_m(buffer, ptr, element_size, element_count, offset, _malloc, _free, _realloc, user_data);
	buffer->info |= READONLY_BUFFER;
	buffer->element_count = element_count;
	return buffer;
}

void BUFget_at(buf_ucount_t index, void* out_value) { buf_get_at(binded_buffer, index, out_value); }
void buf_get_at(BUFFER* buffer, buf_ucount_t index, void* out_value)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(index < buffer->element_count,"index >= buffer->element_count, Index Out of Range Exception");
	memcpy(out_value , buffer->bytes + index * buffer->element_size, buffer->element_size);
}

#if defined(BUF_DEBUG) && defined(BUF_ENABLE_BUFFER_RESIZE_WARNING)
#	define PTR_QUERIED(buffer) (buffer)->is_ptr_queried = true
#	define WARN_IF_PTR_QUERIED(buffer) if((buffer)->is_ptr_queried) { (buffer)->is_ptr_queried = false; printf("Warning: Bufferlib: buffer has been resized since a pointer into the buffer was queried!\n"); }
#else
#	define PTR_QUERIED(buffer)
#	define WARN_IF_PTR_QUERIED(buffer)
#endif

void* BUFgetptr_at(buf_ucount_t index) { return buf_getptr_at(binded_buffer, index); }
void* buf_getptr_at(BUFFER* buffer, buf_ucount_t index)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(index < buffer->element_count,"index >= buffer->element_count, Index Out of Range Exception");
	PTR_QUERIED(buffer);
	return buffer->bytes + index * buffer->element_size;
}

#define __WRITE_OPERATION__ GOOD_ASSERT(!buf_is_readonly(buffer), "Buffer is readonly")

void BUFset_at(buf_ucount_t index , void* in_value) { buf_set_at(binded_buffer, index, in_value); }
void buf_set_at(BUFFER* buffer, buf_ucount_t index , void* in_value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(index < buffer->element_count,"Index Out of Range Exception");
	memcpy(buffer->bytes + index * buffer->element_size, in_value , buffer->element_size);
}

void BUFset_at_n(buf_ucount_t index , void* in_value, buf_ucount_t max_size) { buf_set_at_n(binded_buffer, index, in_value, max_size); }
void buf_set_at_n(BUFFER* buffer, buf_ucount_t index , void* in_value, buf_ucount_t max_size)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(index < buffer->element_count,"Index Out of Range Exception");
	GOOD_ASSERT(max_size <= buffer->element_size, "Overwrite to next element exception");
	memcpy(buffer->bytes + index * buffer->element_size, in_value , max_size);
}

void* BUFget_offset_bytes() { return buf_get_offset_bytes(binded_buffer); }
void* buf_get_offset_bytes(BUFFER* buffer)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->offset != 0, "buffer->offset equals to Zero!");
	PTR_QUERIED(buffer);
	return buffer->bytes + buffer->capacity * buffer->element_size;
}

void BUFset_offset_bytes(void* offset_bytes) { buf_set_offset_bytes(binded_buffer, offset_bytes); }
void buf_set_offset_bytes(BUFFER* buffer, void* offset_bytes)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(offset_bytes != NULL, "offset_bytes is NULL Exception");
	GOOD_ASSERT(buffer->offset != 0, "buffer->offset equals to Zero!");
	memcpy(buf_get_offset_bytes(buffer), offset_bytes, buffer->offset);
}

buf_ucount_t BUFget_buffer_size() { buf_get_buffer_size(binded_buffer); }
buf_ucount_t buf_get_buffer_size(BUFFER* buffer)
{
	check_pre_condition(buffer);
	return buffer->capacity * buffer->element_size  + buffer->offset;
}

void BUFresize(buf_ucount_t new_capacity) { buf_resize(binded_buffer, new_capacity); }
void buf_resize(BUFFER* buffer, buf_ucount_t new_capacity)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	if(new_capacity == buffer->capacity)
		return;
	GOOD_ASSERT((buffer->info & STATIC_BUFFER) == 0, "Buffer is static, it can't be resized");
	buf_ucount_t new_buffer_size = new_capacity * buffer->element_size + buffer->offset;
	buf_ucount_t buffer_size = buffer->capacity * buffer->element_size + buffer->offset;
	void* new_buffer = (new_buffer_size == 0) ? NULL : call_malloc(buffer, new_buffer_size);
	GOOD_ASSERT((new_buffer_size == 0) || (new_buffer != NULL), "Memory Allocation Failure");
	if(buffer->bytes != NULL)
	{
		if(new_buffer_size > buffer_size)
		{
			if((buffer_size - buffer->offset) != 0)
			//copy only the elments, excluding the offset
			memcpy(new_buffer, buffer->bytes, buffer_size - buffer->offset);
			if(buffer->offset != 0)
			//copy the offset bytes at the end of the new buffer
			memcpy(new_buffer + new_buffer_size - buffer->offset, buffer->bytes + buffer_size - buffer->offset, buffer->offset);
			//set the intermediate bytes to zero
			memset(new_buffer + buffer_size - buffer->offset, 0, new_buffer_size - buffer_size);
		}
		else if(new_buffer_size != 0)//if ((new_buffer_size != 0) && (new_buffer_size < buffer_size))
		{
			//copy only the elements, excluding the offset
			memcpy(new_buffer, buffer->bytes, new_buffer_size - buffer->offset);
			if(buffer->offset != 0)
			//copy the offset bytes at the end of the new buffer
			memcpy(new_buffer + new_buffer_size - buffer->offset, buffer->bytes + buffer_size - buffer->offset, buffer->offset);
		}
	}
	else GOOD_ASSERT(buffer_size == 0, "buffer->bytes is NULL but buffer_size is not 0, something is very bad!");
	if(buffer->bytes != NULL)
		call_free(buffer, buffer->bytes);
	buffer->bytes = new_buffer;
	if(buffer->element_count >= new_capacity)
		buffer->element_count = new_capacity;
	buffer->capacity = new_capacity;
	WARN_IF_PTR_QUERIED(buffer);
}

void BUFensure_capacity(buf_ucount_t min_capacity) { buf_ensure_capacity(binded_buffer, min_capacity); }
void buf_ensure_capacity(BUFFER* buffer, buf_ucount_t min_capacity)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	if(min_capacity <= buf_get_capacity(buffer))
		return;
	buf_ucount_t capacity = buf_get_capacity(buffer);
	if(capacity == 0) capacity = 1;
	while(capacity < min_capacity)
		capacity *= 2;
	buf_resize(buffer, capacity);
}

void BUFclear_buffer(void* clear_value) { buf_clear_buffer(binded_buffer, clear_value); }
void buf_clear_buffer(BUFFER* buffer, void* clear_value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	if(clear_value == NULL)
		memset(buffer->bytes, 0, buffer->capacity * buffer->element_size + buffer->offset);
	else
	{
		buf_ucount_t capacity = buffer->capacity;
		while(capacity > 0)
		{
			--capacity;
			memcpy(buffer->bytes + capacity * buffer->element_size, clear_value, buffer->element_size);
		}
		memset(buffer->bytes + buffer->capacity * buffer->element_size, 0, buffer->offset);
	}
	buffer->element_count = 0;
}

void BUFclear(void* clear_value) { buf_clear(binded_buffer, clear_value); }
void buf_clear(BUFFER* buffer, void* clear_value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	if((buffer->free != NULL) && (buffer->element_count > 0))
		buf_traverse_elements(buffer, 0, buf_get_element_count(buffer) - 1, (void (*)(void*, void*))(buffer->free), NULL);

	if(clear_value == NULL)
		memset(buffer->bytes , 0 , buffer->element_count * buffer->element_size) ;
	else
	{
		buf_ucount_t element_count = buffer->element_count;
		while(element_count > 0)
		{
			--element_count;
			memcpy(buffer->bytes + element_count * buffer->element_size, clear_value , buffer->element_size);
		}
	}
 	buffer->element_count = 0;

}

void BUFclear_fast() { buf_clear_fast(binded_buffer); }
void buf_clear_fast(BUFFER* buffer)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	buffer->element_count = 0;
}

void BUFinsert_at_noalloc(buf_ucount_t index , void* in_value , void* out_value) { buf_insert_at_noalloc(binded_buffer, index, in_value, out_value); }
void buf_insert_at_noalloc(BUFFER* buffer, buf_ucount_t index , void* in_value , void* out_value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(index < buffer->capacity,"Buffer Overflow Exception");
	GOOD_ASSERT(buffer->element_count > index ,"Index should be less than buffer->element_count");
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size) ;
	memcpy(buffer->bytes + index * buffer->element_size, in_value , buffer->element_size) ;
}

void BUFinsert_at(buf_ucount_t index , void* in_value) { buf_insert_at(binded_buffer, index, in_value); }
void buf_insert_at(BUFFER* buffer, buf_ucount_t index , void* in_value)
{
	__WRITE_OPERATION__;
	GOOD_ASSERT(buffer != NULL, "Binded Buffer Is NULL Exception");
	GOOD_ASSERT(index <= buffer->element_count,"Buffer Overflow Exception");

	buf_insert_pseudo_uninitialized(buffer, index, 1);

	// copy in_value to the inserted block
	memcpy(buf_get_ptr_at(buffer, index), in_value, buffer->element_size);

	WARN_IF_PTR_QUERIED(buffer);
}

static bool ptr_comparer(void* ptr1, void* ptr2) { return *((uint8_t*)ptr1) == *((uint8_t*)ptr2); }

bool BUFremove_at_noshift(buf_ucount_t index , void* out_value) { return buf_remove_at_noshift(binded_buffer, index, out_value); }
bool buf_remove_at_noshift(BUFFER* buffer, buf_ucount_t index , void* out_value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	GOOD_ASSERT(index < buffer->element_count,"index >= buffer->element_count, Index Out of Range Exception");
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size) ;
	if(buf_is_auto_managed(buffer))
	{
		void* ptr = buffer->bytes + index * buffer->element_size;
		if(buf_find_index_of(buffer->auto_managed_empty_blocks, &ptr, ptr_comparer) == BUF_INVALID_INDEX) /*if ptr is not found in the auto_managed_empty_blocks BUFFER*/
			buf_push(buffer->auto_managed_empty_blocks, &ptr);
	}
	memset(buffer->bytes + index * buffer->element_size , 0 , buffer->element_size);
	return true;
}

bool BUFremove_at(buf_ucount_t index , void* out_value) { return buf_remove_at(binded_buffer, index, out_value); }
bool buf_remove_at(BUFFER* buffer, buf_ucount_t index , void* out_value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	GOOD_ASSERT(index < buffer->element_count,"Index Out of Range Exception");
	--(buffer->element_count);
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size);

	void* dst_ptr = buffer->bytes + index * buffer->element_size;
	if(index < buffer->element_count)
	{
		buf_ucount_t num_shift_elements = (buffer->element_count - index) ;
		while(num_shift_elements > 0)
		{
			memcpy(dst_ptr , dst_ptr + buffer->element_size, buffer->element_size);
			dst_ptr += buffer->element_size;
			--num_shift_elements;
		}
	}
	memset(dst_ptr , 0 , buffer->element_size);
	return true;
}

bool BUFremove_noshift(void* object, bool (*comparer)(void*, void*)) { return buf_remove_noshift(binded_buffer, object, comparer); }
bool buf_remove_noshift(BUFFER* buffer, void* object, bool (*comparer)(void*, void*))
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	void* cursor = buffer->bytes;
	for(buf_ucount_t i = 0; i < buffer->element_count; i++, cursor += buffer->element_size)
	{
		if(comparer(object, cursor))
		{
			memset(cursor, 0, buffer->element_size);
			if(buf_is_auto_managed(buffer))
				if(buf_find_index_of(buffer->auto_managed_empty_blocks, &cursor, ptr_comparer) == BUF_INVALID_INDEX) /*if ptr is not found in the auto_managed_empty_blocks BUFFER*/
					buf_push(buffer->auto_managed_empty_blocks, &cursor);
			return true;
		}
	}
	return false;
}

bool BUFremove(void* object, bool (*comparer)(void*, void*)) { return buf_remove(binded_buffer, object, comparer); }
bool buf_remove(BUFFER* buffer, void* object, bool (*comparer)(void*, void*))
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	void* cursor = buffer->bytes;
	for(buf_ucount_t i = 0; i < buffer->element_count; i++, cursor += buffer->element_size)
	{
		if(comparer(object, cursor))
		{
			/* NOTICE: do not use memcpy here, as overlapping can't be handled by memcpy */
			memmove(cursor, cursor + buffer->element_size, (buffer->element_count - i - 1) * buffer->element_size);
			memset(buf_peek_ptr(buffer), 0, buffer->element_size);
			--(buffer->element_count);
			return true;
		}
	}
	return false;
}

void BUFfit() { buf_fit(binded_buffer); }
void buf_fit(BUFFER* buffer)
{
	//TODO: Replace this with BUFresize(binded_buffer->element_count)
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	buffer->bytes = call_realloc(buffer, buffer->bytes , buffer->element_count * buffer->element_size);
	GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	buffer->capacity = buffer->element_count;
	WARN_IF_PTR_QUERIED(buffer);
}

void* BUFpeek_ptr() { return buf_peek_ptr(binded_buffer); }
void* buf_peek_ptr(BUFFER* buffer)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	PTR_QUERIED(buffer);
	return buffer->bytes + (buffer->element_count - 1) * buffer->element_size;
}

void BUFpeek(void* out_value) { buf_peek(binded_buffer, out_value); }
void buf_peek(BUFFER* buffer, void* out_value)
{
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	memcpy(out_value, buffer->bytes + (buffer->element_count - 1) * buffer->element_size , buffer->element_size);
}

void BUFpop(void* out_value) { buf_pop(binded_buffer, out_value); }
void buf_pop(BUFFER* buffer, void* out_value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	--(buffer->element_count);
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + buffer->element_count * buffer->element_size , buffer->element_size);
	if(buffer->free != NULL)
		buffer->free(buffer->bytes + buffer->element_count * buffer->element_size);
}

void* BUFpop_get_ptr() { return buf_pop_get_ptr(binded_buffer); }
void* buf_pop_get_ptr(BUFFER* buffer)
{
	__WRITE_OPERATION__;
	void* ptr = buf_peek_ptr(buffer);
	buf_pop_pseudo(buffer, 1);
	PTR_QUERIED(buffer);
	return ptr;
}


buf_ucount_t BUFfind_index_of(void* value, bool (*comparer)(void*, void*)) { return buf_find_index_of(binded_buffer, value, comparer); }
buf_ucount_t buf_find_index_of(BUFFER* buffer, void* value, bool (*comparer)(void*, void*))
{
	check_pre_condition(buffer);
	void* cursor = buffer->bytes;
	for(buf_ucount_t i = 0; i < buffer->element_count; i++, cursor += buffer->element_size)
		if(comparer(value, cursor))
			return i;
	return BUF_INVALID_INDEX;
}

void BUFpush(void* in_value) { buf_push(binded_buffer, in_value); }
void buf_push(BUFFER* buffer, void* in_value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	buf_ensure_capacity(buffer, buffer->element_count + 1);
	buffer->element_count += 1;
	buf_set_at(buffer, buffer->element_count - 1, in_value);
	WARN_IF_PTR_QUERIED(buffer);
}

void BUFpushv(void* in_value, buf_ucount_t count) { buf_pushv(binded_buffer, in_value, count); }
void buf_pushv(BUFFER* buffer, void* in_value, buf_ucount_t count)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	buf_ensure_capacity(buffer, buffer->element_count + count);
	buffer->element_count += count;
	for(buf_ucount_t i = 0; i < count; i++, in_value += buffer->element_size)
		buf_set_at(buffer, buffer->element_count - count + i, in_value);
	WARN_IF_PTR_QUERIED(buffer);
}

void buf_push_n(BUFFER* buffer, void* in_value, buf_ucount_t max_size)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	buf_ensure_capacity(buffer, buffer->element_count + 1);
	buffer->element_count += 1;
	buf_set_at_n(buffer, buffer->element_count - 1, in_value, max_size);
	WARN_IF_PTR_QUERIED(buffer);
}

void buf_vprintf(BUFFER* buffer, char* stage_buffer, const char* format_string, va_list args)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	if(stage_buffer != NULL)
	{
		vsprintf(stage_buffer, format_string, args);
		buf_pushv(buffer, stage_buffer, strlen(stage_buffer));
	}
	else
	{
		buf_ucount_t offset = buf_get_element_count(buffer);
		va_list args2;
		va_copy(args2, args);
		buf_ucount_t count = vsnprintf(NULL, 0, format_string, args);
		buf_push_pseudo(buffer, count + 1);
		buf_ucount_t result = vsnprintf(buf_get_ptr(buffer) + offset, count + 1, format_string, args2);
		GOOD_ASSERT(result == count, "Fetal error while formating string");
	}
	WARN_IF_PTR_QUERIED(buffer);
}

void buf_printf(BUFFER* buffer, char* stage_buffer, const char* format_string, ...)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	va_list args;
	va_start(args, format_string);
	buf_vprintf(buffer, stage_buffer, format_string, args);
	va_end(args);
	WARN_IF_PTR_QUERIED(buffer);
}

void buf_push_string(BUFFER* buffer, const char* string)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	buf_pushv(buffer, (char*)string, strlen(string));
	WARN_IF_PTR_QUERIED(buffer);
}

void buf_push_char(BUFFER* buffer, char value)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	buf_push(buffer, &value);
	WARN_IF_PTR_QUERIED(buffer);
}

void _buf_get_at_s(BUFFER* buffer, buf_ucount_t index, void* out_value, uint32_t out_value_size)
{
	GOOD_ASSERT(out_value_size == buf_get_element_size(buffer), "Out value size doesn't match with the Buffer's element size");
	buf_get_at(buffer, index, out_value);
}

BUF_API void buf_push_u8(	BUFFER* buffer, u8 value)
{
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
}
BUF_API void buf_push_u16(	BUFFER* buffer, u16 value)
{
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
}
BUF_API void buf_push_u32(	BUFFER* buffer, u32 value)
{
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
}
BUF_API void buf_push_u64(	BUFFER* buffer, u64 value)
{
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
}
BUF_API void buf_push_s8(	BUFFER* buffer, s8 value)
{
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
}
BUF_API void buf_push_s16(	BUFFER* buffer, s16 value)
{
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
}
BUF_API void buf_push_s32(	BUFFER* buffer, s32 value)
{
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
}
BUF_API void buf_push_s64(	BUFFER* buffer, s64 value)
{
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
}

static buf_ucount_t get_selection_index(void* values, u32 stride, buf_ucount_t count, buf_comparer_t compare, void* user_data)
{
	buf_ucount_t index = 0;
	void* value = values;
	for(u32 i = 1; i < count; i++)
	{
		void* cursor = values + stride * i;
		if(compare(cursor, value, user_data))
		{
			value = cursor;
			index = i;
		}
	}
	return index;
}

static void swap(void* ptr1, void* ptr2, u32 size, void* swap_buffer)
{
	if(swap_buffer == NULL)
		swap_buffer = alloca(size);
	memcpy(swap_buffer, ptr1, size);
	memcpy(ptr1, ptr2, size);
	memcpy(ptr2, swap_buffer, size);
}

BUF_API void buf_sort(BUFFER* buffer, buf_comparer_t compare, void* user_data)
{
	check_pre_condition(buffer);
	__WRITE_OPERATION__;
	buf_ucount_t count = buf_get_element_count(buffer);
	uint32_t stride = buf_get_element_size(buffer);
	void* ptr = buf_get_ptr(buffer);
	uint8_t swap_buffer[stride];
	for(buf_ucount_t i = 0; i < count; i++)
	{
		void* v1 = ptr + stride * i;
		swap(v1, v1 + stride * get_selection_index(v1, stride, count - i, compare, user_data), stride, swap_buffer);
	}
}

BUF_API void buf_push_sort(BUFFER* buffer, void* value, buf_comparer_t compare, void* user_data)
{
	__WRITE_OPERATION__;
	uint32_t stride = buf_get_element_size(buffer);
	void* ptr = buf_get_ptr(buffer);
	buf_ucount_t i = buf_get_element_count(buffer);
	while(i > 0)
	{
		void* cursor = ptr + (i - 1) * stride;
		if(compare(cursor, value, user_data))
			break;
		--i;
	}
	buf_insert_at(buffer, i, value);
}

BUF_API void* buf_create_element(BUFFER* buffer)
{
	__WRITE_OPERATION__;
	buf_push_pseudo(buffer, 1);
	return buf_peek_ptr(buffer);
}

bool buf_string_comparer(void* v1, void* v2)
{
	return strcmp(*(char**)v1, *(char**)v2) == 0;
}

bool buf_ptr_comparer(void* v1, void* v2)
{
	return (*(char**)v1) == (*(char**)v2);
}

bool buf_s8_comparer(void* v1, void* v2)
{
	return (*(int8_t*)v1) == (*(int8_t*)v2);
}

bool buf_s16_comparer(void* v1, void* v2)
{
	return (*(int16_t*)v1) == (*(int16_t*)v2);
}

bool buf_s32_comparer(void* v1, void* v2)
{
	return (*(int32_t*)v1) == (*(int32_t*)v2);
}

bool buf_s64_comparer(void* v1, void* v2)
{
	return (*(int64_t*)v1) == (*(int64_t*)v2);
}

bool buf_u8_comparer(void* v1, void* v2)
{
	return (*(uint8_t*)v1) == (*(uint8_t*)v2);
}

bool buf_u16_comparer(void* v1, void* v2)
{
	return (*(uint16_t*)v1) == (*(uint16_t*)v2);
}

bool buf_u32_comparer(void* v1, void* v2)
{
	return (*(uint32_t*)v1) == (*(uint32_t*)v2);
}

bool buf_u64_comparer(void* v1, void* v2)
{
	return (*(uint64_t*)v1) == (*(uint64_t*)v2);
}

bool buf_float_comparer(void* v1, void* v2)
{
	return (*(float*)v1) == (*(float*)v2);
}

bool buf_double_comparer(void* v1, void* v2)
{
	return (*(double*)v1) == (*(double*)v2);
}


BUF_API bool buf_string_greater_than(void* v1, void* v2, void* user_data)
{
	return strlen(*((const char* const*)v1)) > strlen(*((const char* const*)v2));
}

BUF_API bool buf_ptr_greater_than(void* v1, void* v2, void* user_data)
{
	return *(char* const*)v1 > *(char* const*)v2;
}

BUF_API bool buf_s8_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const int8_t*)v1 > *(const int8_t*)v2;
}

BUF_API bool buf_s16_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const int16_t*)v1 > *(const int16_t*)v2;
}

BUF_API bool buf_s32_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const int32_t*)v1 > *(const int32_t*)v2;
}

BUF_API bool buf_s64_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const int64_t*)v1 > *(const int64_t*)v2;
}

BUF_API bool buf_u8_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const uint8_t*)v1 > *(const uint8_t*)v2;
}

BUF_API bool buf_u16_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const uint16_t*)v1 > *(const uint16_t*)v2;
}

BUF_API bool buf_u32_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const uint32_t*)v1 > *(const uint32_t*)v2;
}

BUF_API bool buf_u64_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const uint64_t*)v1 > *(const uint64_t*)v2;
}

BUF_API bool buf_float_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const float*)v1 > *(const float*)v2;
}

BUF_API bool buf_double_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const double*)v1 > *(const double*)v2;
}


BUF_API bool buf_string_less_than(void* v1, void* v2, void* user_data)
{
	return strlen(*((const char* const*)v1)) < strlen(*((const char* const*)v2));
}

BUF_API bool buf_ptr_less_than(void* v1, void* v2, void* user_data)
{
	return *(char* const*)v1 < *(char* const*)v2;
}

BUF_API bool buf_s8_less_than(void* v1, void* v2, void* user_data)
{
	return *(const int8_t*)v1 < *(const int8_t*)v2;
}

BUF_API bool buf_s16_less_than(void* v1, void* v2, void* user_data)
{
	return *(const int16_t*)v1 < *(const int16_t*)v2;
}

BUF_API bool buf_s32_less_than(void* v1, void* v2, void* user_data)
{
	return *(const int32_t*)v1 < *(const int32_t*)v2;
}

BUF_API bool buf_s64_less_than(void* v1, void* v2, void* user_data)
{
	return *(const int64_t*)v1 < *(const int64_t*)v2;
}

BUF_API bool buf_u8_less_than(void* v1, void* v2, void* user_data)
{
	return *(const uint8_t*)v1 < *(const uint8_t*)v2;
}

BUF_API bool buf_u16_less_than(void* v1, void* v2, void* user_data)
{
	return *(const uint16_t*)v1 < *(const uint16_t*)v2;
}

BUF_API bool buf_u32_less_than(void* v1, void* v2, void* user_data)
{
	return *(const uint32_t*)v1 < *(const uint32_t*)v2;
}

BUF_API bool buf_u64_less_than(void* v1, void* v2, void* user_data)
{
	return *(const uint64_t*)v1 < *(const uint64_t*)v2;
}

BUF_API bool buf_float_less_than(void* v1, void* v2, void* user_data)
{
	return *(const float*)v1 < *(const float*)v2;
}

BUF_API bool buf_double_less_than(void* v1, void* v2, void* user_data)
{
	return *(const double*)v1 < *(const double*)v2;
}


BUF_API void buf_string_print(void* value, void* user_data)
{
	printf("%s ", *((const char* const*)value));
}

BUF_API void buf_ptr_print(void* value, void* user_data)
{
	printf("%p ", *(char* const*)value);
}

BUF_API void buf_s8_print(void* value, void* user_data)
{
	printf("%" PRIi8, *(const int8_t*)value);
}

BUF_API void buf_s16_print(void* value, void* user_data)
{
	printf("%" PRIi16, *(const int16_t*)value);
}

BUF_API void buf_s32_print(void* value, void* user_data)
{
	printf("%" PRIi32, *(const int32_t*)value);
}

BUF_API void buf_s64_print(void* value, void* user_data)
{
	printf("%" PRIi64, *(const int64_t*)value);
}

BUF_API void buf_u8_print(void* value, void* user_data)
{
	printf("%" PRIu8, *(const uint8_t*)value);
}

BUF_API void buf_u16_print(void* value, void* user_data)
{
	printf("%" PRIu16, *(const uint16_t*)value);
}

BUF_API void buf_u32_print(void* value, void* user_data)
{
	printf("%" PRIu32, *(const uint32_t*)value);
}

BUF_API void buf_u64_print(void* value, void* user_data)
{
	printf("%" PRIu64, *(const uint64_t*)value);
}

BUF_API void buf_float_print(void* value, void* user_data)
{
	printf("%f ", *(const float*)value);
}

BUF_API void buf_double_print(void* value, void* user_data)
{
	printf("%f ", *(const double*)value);
}


#ifdef BUF_DEBUG
static void check_pre_condition(BUFFER* buffer)
{
	GOOD_ASSERT(buffer != NULL, "buffer is NULL");
}
#endif /*BUF_DEBUG*/
