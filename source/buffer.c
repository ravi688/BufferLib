#include <buffer.h>				//This must be included at the top for preprocessing reasons


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifdef BUF_DEBUG
#	define GOOD_ASSERT(bool_value, string) do { if(!(bool_value)) {  log_fetal_err("Assertion Failed: %s\n", string); } } while(false)
#else
#	define GOOD_ASSERT(...)
#endif

#define STACK_ALLOCATED_OBJECT  0x1
#define HEAP_ALLOCATED_OBJECT 	 0x2

static BUFFER* binded_buffer;

function_signature(static bool, buf_is_stack_allocated, BUFFER* buffer);
function_signature(static bool, buf_is_heap_allocated, BUFFER* buffer);
function_signature_void(static bool, BUFis_stack_allocated);
function_signature_void(static bool, BUFis_heap_allocated);
#define buf_is_stack_allocated(...) define_alias_function_macro(buf_is_stack_allocated, __VA_ARGS__)
#define buf_is_heap_allocated(...) define_alias_function_macro(buf_is_heap_allocated, __VA_ARGS__)
#define BUFis_stack_allocated() define_alias_function_void_macro(BUFis_stack_allocated)
#define BUFis_heap_allocated() define_alias_function_void_macro(BUFis_heap_allocated)

function_signature_void(uint64_t, BUFget_buffer_object_size) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(sizeof(BUFFER)); } 

function_signature(void, BUFset_on_post_resize, void (*on_post_resize)(void)) { CALLTRACE_BEGIN(); buf_set_on_post_resize(binded_buffer, on_post_resize); CALLTRACE_END(); }
function_signature(void, buf_set_on_post_resize, BUFFER* buffer, void (*on_post_resize)(void))
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	buffer->on_post_resize = on_post_resize;
	CALLTRACE_END();
}

function_signature(void, BUFset_on_pre_resize, void (*on_pre_resize)(void)) { CALLTRACE_BEGIN(); buf_set_on_pre_resize(binded_buffer, on_pre_resize); CALLTRACE_END(); }
function_signature(void, buf_set_on_pre_resize, BUFFER* buffer, void (*on_pre_resize)(void))
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	buffer->on_pre_resize = on_pre_resize;
	CALLTRACE_END();
}

function_signature(void, BUFpush_pseudo, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_push_pseudo(binded_buffer, count); CALLTRACE_END(); }
function_signature(void, buf_push_pseudo, BUFFER* buffer, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	buf_ucount_t previous_element_count = buffer->element_count;
	buffer->element_count += count; 
	if(buffer->capacity <= 0)
		buffer->capacity = 1;
	buf_ucount_t previous_capacity = buffer->capacity;

	while(buffer->capacity < buffer->element_count)
		buffer->capacity *= 2;
	if(previous_capacity != buffer->capacity)
	{
		if(buffer->on_pre_resize != NULL)
		{
			buf_ucount_t temp = previous_capacity; 
			previous_capacity = buffer->capacity; 
			buffer->capacity = temp;
		 	(buffer->on_pre_resize)();
		 	temp = previous_capacity;
		 	previous_capacity = buffer->capacity; 
			buffer->capacity = temp;
		}
		buffer->bytes = realloc(buffer->bytes , buffer->capacity * buffer->element_size); 
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
		if(buffer->on_post_resize != NULL) (buffer->on_post_resize)();
	}
	memset(buffer->bytes + previous_element_count * buffer->element_size , 0 , buffer->element_size * count); 
	CALLTRACE_END();
}

function_signature(void, BUFpop_pseudo, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_pop_pseudo(binded_buffer, count); CALLTRACE_END(); }
function_signature(void, buf_pop_pseudo, BUFFER* buffer, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	GOOD_ASSERT(count <= buffer->element_count, "Buffer Underflow Exception");
	buffer->element_count -= count;	
	CALLTRACE_END();
}

function_signature(void, BUFinsert_pseudo, buf_ucount_t index, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_insert_pseudo(binded_buffer, index, count); CALLTRACE_END(); }
function_signature(void, buf_insert_pseudo, BUFFER* buffer, buf_ucount_t index, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Binded Buffer Is NULL Exception");
	GOOD_ASSERT(index < buffer->capacity,"Buffer Overflow Exception");
	buf_ucount_t num_shift_elements = buffer->element_count - index; 
	buffer->element_count += count; 
	if(buffer->element_count > buffer->capacity)
	{	
		if(buffer->on_pre_resize != NULL) (buffer->on_pre_resize)();
		buffer->bytes = realloc(buffer->bytes , buffer->element_size * buffer->element_count);
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
		buffer->capacity = buffer->element_count;
		if(buffer->on_post_resize != NULL) (buffer->on_post_resize)();
	}
	void* dst_ptr = buffer->bytes + (buffer->element_count - 1) * buffer->element_size;
	uint8_t offset = buffer->element_size * count;
	while(num_shift_elements)
	{
		memcpy(dst_ptr , dst_ptr - offset, buffer->element_size) ; 
		dst_ptr -= buffer->element_size;
		--num_shift_elements;
	}
	memset(buffer->bytes + index * buffer->element_size, 0, count * buffer->element_size);
	CALLTRACE_END();
}

function_signature(void, BUFremove_pseudo, buf_ucount_t index, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_remove_pseudo(binded_buffer, index, count);  CALLTRACE_END(); }
function_signature(void, buf_remove_pseudo, BUFFER* buffer, buf_ucount_t index, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	GOOD_ASSERT(index < buffer->element_count, "Index Out Of Range Exception");
	GOOD_ASSERT(count <= buffer->element_count,"Buffer Underflow Exception");
	buffer->element_count -= count; 
	buf_ucount_t num_shift_elements = buffer->element_count;
	void* dst_ptr = buffer->bytes + index * buffer->element_size;
	while(num_shift_elements)
	{
		memcpy(dst_ptr , dst_ptr + buffer->element_size, buffer->element_size); 
		dst_ptr += buffer->element_size;
		--num_shift_elements;
	}
	memset(buffer->bytes + buffer->element_count * buffer->element_size , 0 , buffer->element_size * count);
	CALLTRACE_END();
}

function_signature(void, BUFset_auto_managed, bool value) { CALLTRACE_BEGIN(); buf_set_auto_managed(binded_buffer, value); CALLTRACE_END(); }
function_signature(void, buf_set_auto_managed, BUFFER* buffer, bool value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception"); 
	if(value && (buffer->auto_managed_empty_blocks == BUF_INVALID))
			buffer->auto_managed_empty_blocks = BUFcreate(BUF_INVALID, sizeof(void*), 0, 0); 
	else if(!value && (buffer->auto_managed_empty_blocks != BUF_INVALID))
				buf_free(buffer->auto_managed_empty_blocks); 
 	buffer->is_auto_managed = value; 
 	CALLTRACE_END();
}

function_signature_void(buf_ucount_t, BUFget_offset) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_offset(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_offset, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception"); 
	CALLTRACE_RETURN(buffer->offset); 
}

function_signature_void(buf_ucount_t, BUFget_capacity) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_capacity(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_capacity, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception"); 
	CALLTRACE_RETURN(buffer->capacity); 
}

function_signature_void(buf_ucount_t, BUFget_element_count) { CALLTRACE_BEGIN(); CALLTRACE_RETURN( buf_get_element_count(binded_buffer)); } 
function_signature(buf_ucount_t, buf_get_element_count, BUFFER* buffer) 
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception"); 
	CALLTRACE_RETURN(buffer->element_count); 
} 

function_signature_void(buf_ucount_t, BUFget_element_size) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_element_size(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_element_size, BUFFER* buffer)
{ 
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception"); 
	CALLTRACE_RETURN(buffer->element_size); 
} 

function_signature_void(void*, BUFget_ptr) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_ptr(binded_buffer)); }
function_signature(void*, buf_get_ptr, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception"); 
	CALLTRACE_RETURN(buffer->bytes); 
} 

function_signature(void, BUFset_offset, buf_ucount_t offset) { CALLTRACE_BEGIN(); buf_set_offset(binded_buffer, offset); CALLTRACE_END(); }
function_signature(void, buf_set_offset, BUFFER* buffer, buf_ucount_t offset)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	buffer->offset = offset;
	CALLTRACE_END();
}

function_signature(void, BUFset_capacity, buf_ucount_t capacity) { CALLTRACE_BEGIN(); buf_set_capacity(binded_buffer, capacity); CALLTRACE_END(); }
function_signature(void, buf_set_capacity, BUFFER* buffer, buf_ucount_t capacity)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
 	buffer->capacity = capacity;
 	CALLTRACE_END();
}

function_signature(void, BUFset_element_count, buf_ucount_t element_count) { CALLTRACE_BEGIN(); buf_set_element_count(binded_buffer, element_count); CALLTRACE_END(); }
function_signature(void, buf_set_element_count, BUFFER* buffer, buf_ucount_t element_count)
{ 
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	buffer->element_count = element_count; 
	CALLTRACE_END();
}

function_signature(void, BUFset_element_size, buf_ucount_t element_size) { CALLTRACE_BEGIN(); buf_set_element_size(binded_buffer, element_size); CALLTRACE_END(); }
function_signature(void, buf_set_element_size, BUFFER* buffer, buf_ucount_t element_size)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	buffer->element_size = element_size; 
	CALLTRACE_END();
} 

function_signature(void, BUFset_ptr, void* ptr) { CALLTRACE_BEGIN(); buf_set_ptr(binded_buffer, ptr); CALLTRACE_END(); }
function_signature(void, buf_set_ptr, BUFFER* buffer, void* ptr)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	buffer->bytes = ptr; 
	CALLTRACE_END();
}

function_signature_void(BUFFER*, BUFget_binded_buffer)
{ 
	CALLTRACE_BEGIN();
	CALLTRACE_RETURN(binded_buffer);
} 

function_signature_void(bool, BUFis_auto_managed) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_is_auto_managed(binded_buffer)); }
function_signature(bool, buf_is_auto_managed, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	CALLTRACE_RETURN(buffer->is_auto_managed); 
}

function_signature(void, BUFbind, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
  binded_buffer = buffer;
  CALLTRACE_END(); 
} 

function_signature_void(void, BUFunbind)
{ 
	CALLTRACE_BEGIN();
	binded_buffer = NULL; 
	CALLTRACE_END();
} 

function_signature_void(void, BUFlog) { CALLTRACE_BEGIN(); buf_log(binded_buffer); CALLTRACE_END(); }
function_signature(void, buf_log, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	log_msg(
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
	CALLTRACE_END();
}

function_signature(void, BUFtraverse_elements, buf_ucount_t start, buf_ucount_t end, void (*func)(void* /*element ptr*/, void* /*args ptr*/), void* args) { CALLTRACE_BEGIN(); buf_traverse_elements(binded_buffer, start, end, func, args); CALLTRACE_END(); }
function_signature(void, buf_traverse_elements, BUFFER* buffer, buf_ucount_t start, buf_ucount_t end, void (*func)(void* /*element ptr*/, void* /*args ptr*/), void* args)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception"); 
	GOOD_ASSERT((start <= end) && (start < buffer->element_count) && (end < buffer->element_count), "(start <= end) && (start < buffer->element_count) && (end < buffer->element_count) evaulates to false");
	for(buf_ucount_t i = start; i <= end; i++)
	 		func(buf_getptr_at(buffer, i), args);
	CALLTRACE_END();
}

function_signature_void(static bool, BUFis_stack_allocated) { CALLTRACE_BEGIN();  CALLTRACE_RETURN(buf_is_stack_allocated(binded_buffer)); }
function_signature(static bool, buf_is_stack_allocated, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	CALLTRACE_RETURN((buffer->info & STACK_ALLOCATED_OBJECT) == STACK_ALLOCATED_OBJECT); 
}

function_signature_void(static bool, BUFis_heap_allocated) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_is_heap_allocated(binded_buffer)); }
function_signature(static bool, buf_is_heap_allocated, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	CALLTRACE_RETURN((buffer->info & HEAP_ALLOCATED_OBJECT) == HEAP_ALLOCATED_OBJECT); 
}

function_signature_void(void, BUFfree) { CALLTRACE_BEGIN(); buf_free(binded_buffer); CALLTRACE_END(); }
function_signature(void, buf_free, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Buffer Is NULL Exception");
	if((buffer->free != NULL) && (buffer->element_count > 0))
		buf_traverse_elements(buffer, 0, buf_get_element_count(buffer)- 1, (void (*)(void*, void*))(buffer->free), NULL);
	if(buffer->bytes != NULL)
	{
		free(buffer->bytes);  
		buffer->bytes == NULL;
	}

  if(buffer->info & HEAP_ALLOCATED_OBJECT)
  { buffer->info = 0x00; free(buffer) ; } 
	buffer = NULL;
	CALLTRACE_END();	
}

function_signature_void(BUFFER*, BUFget_clone)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(binded_buffer != NULL, "binded buffer is NULL Exception");
	CALLTRACE_RETURN(BUFcopy_construct(binded_buffer));
}
function_signature(BUFFER, buf_get_clone, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
	CALLTRACE_RETURN(buf_copy_construct(buffer));
}

function_signature(void, BUFmove_to, BUFFER* destination) { CALLTRACE_BEGIN(); buf_move_to(binded_buffer, destination); CALLTRACE_END(); }
function_signature(void, buf_move_to, BUFFER* buffer, BUFFER* destination)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
	GOOD_ASSERT(destination != NULL, "destination buffer is NULL Exception");
	buf_copy_to(buffer, destination);
	buf_free(buffer);
	CALLTRACE_END();
}

function_signature(void, BUFcopy_to, BUFFER* destination) { CALLTRACE_BEGIN(); buf_copy_to(binded_buffer, destination); CALLTRACE_END(); }
function_signature(void, buf_copy_to, BUFFER* buffer, BUFFER* destination)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
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
	CALLTRACE_END(); 
}

function_signature(BUFFER*, BUFcopy_construct, BUFFER* source)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(source != NULL, "source buffer Is NULL Exception"); 
	BUFFER* buffer = BUFcreate(NULL, source->element_size, source->capacity, source->offset); 
	buf_copy_to(source, buffer);
	CALLTRACE_RETURN(buffer);
}

function_signature(BUFFER, buf_copy_construct, BUFFER* source)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(source != NULL, "source buffer is NULL Exception");			
	BUFFER buffer = buf_create(source->element_size, source->capacity, source->offset); 
	buf_copy_to(source, &buffer);
	CALLTRACE_RETURN(buffer);
}

function_signature(void, BUFset_on_free, void (*free)(void*)) { CALLTRACE_BEGIN(); buf_set_on_free(binded_buffer, free); CALLTRACE_END(); }
function_signature(void, buf_set_on_free, BUFFER* buffer, void (*free)(void*))
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");			
	buffer->free = free;
	CALLTRACE_END();
}

function_signature(BUFFER, buf_create, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset)
{
	CALLTRACE_BEGIN();
	BUFFER buffer;
	BUFcreate_object(&buffer);
	BUFcreate(&buffer, element_size, capacity, offset);
	CALLTRACE_RETURN(buffer);
}

//TODO: Replace the name with BUFcreate_object_from_bytes
function_signature(BUFFER*, BUFcreate_object, void* bytes)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(bytes != NULL, "bytes equals to NULL");
	BUFFER* buffer = bytes;
	buffer->bytes = NULL;
	buffer->info = 0x00;
	buffer->info |= STACK_ALLOCATED_OBJECT; 
	buffer->element_size = 0;
	buffer->element_count = 0;
	buffer->capacity = 0;
	buffer->auto_managed_empty_blocks = NULL; 
	buffer->is_auto_managed = false; 
	buffer->on_pre_resize = NULL; 
	buffer->on_post_resize = NULL;
	buffer->offset = 0;
	buffer->free = NULL;
	CALLTRACE_RETURN(buffer);
}

function_signature(BUFFER*, BUFcreate, BUFFER* buffer, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(((int64_t)element_size) > 0, "element_size cannot be negative or zero"); 
	GOOD_ASSERT(((int64_t)capacity) >= 0, "capacity cannot be negative");
	GOOD_ASSERT(((int64_t)offset) >= 0, "offset cannot be negative");
	if(buffer == NULL)
	{ 
		buffer = malloc(sizeof(BUFFER));
		GOOD_ASSERT(buffer != NULL, "Memory Allocation Failure Exception");
		buffer->info = 0x00; 
		buffer->info |= HEAP_ALLOCATED_OBJECT;
		buffer->auto_managed_empty_blocks = NULL; 
		buffer->is_auto_managed = false;
		buffer->on_pre_resize = NULL; 
		buffer->on_post_resize = NULL;
		buffer->free = NULL;
	}
	buffer->bytes = (void*)malloc(element_size * capacity + offset); 
	GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	buffer->element_size = element_size; 
	buffer->capacity = capacity;
	buffer->element_count = 0;
	buffer->offset = offset;
	CALLTRACE_RETURN(buffer); 
}

function_signature(void, BUFget_at, buf_ucount_t index, void* out_value) { CALLTRACE_BEGIN(); buf_get_at(binded_buffer, index, out_value); CALLTRACE_END(); }
function_signature(void, buf_get_at, BUFFER* buffer, buf_ucount_t index, void* out_value)
{	
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");			
	GOOD_ASSERT(index < buffer->element_count,"index >= buffer->element_count, Index Out of Range Exception");
	memcpy(out_value , buffer->bytes + index * buffer->element_size, buffer->element_size); 
	CALLTRACE_END();
}

function_signature(void*, BUFgetptr_at, buf_ucount_t index) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_getptr_at(binded_buffer, index)); }
function_signature(void*, buf_getptr_at, BUFFER* buffer, buf_ucount_t index)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");			
	GOOD_ASSERT(index < buffer->element_count,"index >= buffer->element_count, Index Out of Range Exception");
	CALLTRACE_RETURN(buffer->bytes + index * buffer->element_size); 
}

function_signature(void, BUFset_at, buf_ucount_t index , void* in_value) { CALLTRACE_BEGIN(); buf_set_at(binded_buffer, index, in_value); CALLTRACE_END(); }
function_signature(void, buf_set_at, BUFFER* buffer, buf_ucount_t index , void* in_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");		
	GOOD_ASSERT(index < buffer->element_count,"Index Out of Range Exception");
	memcpy(buffer->bytes + index * buffer->element_size, in_value , buffer->element_size); 
	CALLTRACE_END();
}

function_signature_void(void*, BUFget_offset_bytes) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_offset_bytes(binded_buffer)); }
function_signature(void*, buf_get_offset_bytes, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(buffer->offset != 0, "buffer->offset equals to Zero!");
	CALLTRACE_RETURN(buffer->bytes + buffer->capacity * buffer->element_size);
}

function_signature(void, BUFset_offset_bytes, void* offset_bytes) { CALLTRACE_BEGIN(); buf_set_offset_bytes(binded_buffer, offset_bytes); CALLTRACE_END(); }
function_signature(void, buf_set_offset_bytes, BUFFER* buffer, void* offset_bytes)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(offset_bytes != NULL, "offset_bytes is NULL Exception");
	GOOD_ASSERT(buffer->offset != 0, "buffer->offset equals to Zero!");
	memcpy(buf_get_offset_bytes(buffer), offset_bytes, buffer->offset);
	CALLTRACE_END();
}

function_signature_void(buf_ucount_t, BUFget_buffer_size) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_buffer_size(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_buffer_size, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	CALLTRACE_RETURN(buffer->capacity * buffer->element_size  + buffer->offset);
}

function_signature(void, BUFresize, buf_ucount_t new_capacity) { CALLTRACE_BEGIN(); buf_resize(binded_buffer, new_capacity); CALLTRACE_END(); }
function_signature(void, buf_resize, BUFFER* buffer, buf_ucount_t new_capacity)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
	if(new_capacity == buffer->capacity)
		CALLTRACE_RETURN();
	buf_ucount_t new_buffer_size = new_capacity * buffer->element_size + buffer->offset; 
	buf_ucount_t buffer_size = buffer->capacity * buffer->element_size + buffer->offset;
	void* new_buffer = malloc(new_buffer_size); 
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
	else//if (new_buffer_size < buffer_size)
	{
		//copy only the elements, excluding the offset
		memcpy(new_buffer, buffer->bytes, new_buffer_size - buffer->offset);
		if(buffer->offset != 0)
		//copy the offset bytes at the end of the new buffer
		memcpy(new_buffer + new_buffer_size - buffer->offset, buffer->bytes + buffer_size - buffer->offset, buffer->offset);
	}
	if(buffer->bytes != NULL)
		free(buffer->bytes);
	buffer->bytes = new_buffer;
	if((new_buffer_size < buffer_size) && (buffer->element_count >= buffer->capacity))
		buffer->element_count = new_capacity;
	buffer->capacity = new_capacity;
	CALLTRACE_END();
}

function_signature(void, BUFclear_buffer, void* clear_value) { CALLTRACE_BEGIN(); buf_clear_buffer(binded_buffer, clear_value); CALLTRACE_END(); }
function_signature(void, buf_clear_buffer, BUFFER* buffer, void* clear_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");		
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
	CALLTRACE_END();
}

function_signature(void, BUFclear, void* clear_value) { CALLTRACE_BEGIN(); buf_clear(binded_buffer, clear_value); CALLTRACE_END(); }
function_signature(void, buf_clear, BUFFER* buffer, void* clear_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");		
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
 	CALLTRACE_END();
}

function_signature(void, BUFinsert_at_noalloc, buf_ucount_t index , void* in_value , void* out_value) { CALLTRACE_BEGIN(); buf_insert_at_noalloc(binded_buffer, index, in_value, out_value); CALLTRACE_END(); }
function_signature(void, buf_insert_at_noalloc, BUFFER* buffer, buf_ucount_t index , void* in_value , void* out_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(index < buffer->capacity,"Buffer Overflow Exception");
	GOOD_ASSERT(buffer->element_count > index ,"Index should be less than buffer->element_count");
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size) ; 
	memcpy(buffer->bytes + index * buffer->element_size, in_value , buffer->element_size) ;  
	CALLTRACE_END();
}

function_signature(void, BUFinsert_at, buf_ucount_t index , void* in_value) { CALLTRACE_BEGIN(); buf_insert_at(binded_buffer, index, in_value); CALLTRACE_END(); }
function_signature(void, buf_insert_at, BUFFER* buffer, buf_ucount_t index , void* in_value)
{	
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(index < buffer->capacity,"Buffer Overflow Exception");
	GOOD_ASSERT(buffer->element_count > index ,"Index should be less than buffer->element_count");
	buf_ucount_t num_shift_elements = buffer->element_count - index; 
	++(buffer->element_count); 
	if((buffer->element_count) > buffer->capacity)
	{
		if(buffer->on_pre_resize != NULL)(buffer->on_pre_resize)();
		buffer->bytes = realloc(buffer->bytes , buffer->element_size * buffer->element_count) ; 
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
		buffer->capacity = buffer->element_count;
		if(buffer->on_post_resize != NULL) (buffer->on_post_resize)();
	}

	void* dst_ptr = buffer->bytes + (buffer->element_count - 1) * buffer->element_size;
	while(num_shift_elements)
	{
		memcpy(dst_ptr , (dst_ptr - buffer->element_size) , buffer->element_size) ; 
	    dst_ptr -= buffer->element_size;
		--num_shift_elements;
	}
	memcpy(dst_ptr , in_value , buffer->element_size); 
	CALLTRACE_END();
}

static bool ptr_comparer(void* ptr1, void* ptr2) { return *((uint8_t*)ptr1) == *((uint8_t*)ptr2); }

function_signature(bool, BUFremove_at_noshift, buf_ucount_t index , void* out_value) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_remove_at_noshift(binded_buffer, index, out_value)); }
function_signature(bool, buf_remove_at_noshift, BUFFER* buffer, buf_ucount_t index , void* out_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	GOOD_ASSERT(index < binded_buffer->element_count,"index >= binded_buffer->element_count, Index Out of Range Exception");
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size) ;
	if(buf_is_auto_managed(buffer))
	{		
		BUFFER* previous_buffer = buffer; 
		void* ptr = buffer->bytes + index * buffer->element_size;
		if(buf_find_index_of(buffer->auto_managed_empty_blocks, &ptr, ptr_comparer) == BUF_INVALID_INDEX) /*if ptr is not found in the auto_managed_empty_blocks BUFFER*/
			buf_push(buffer->auto_managed_empty_blocks, &ptr);
	}
	memset(buffer->bytes + index * buffer->element_size , 0 , buffer->element_size); 
	CALLTRACE_RETURN(true);
}

function_signature(bool, BUFremove_at, buf_ucount_t index , void* out_value) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_remove_at(binded_buffer, index, out_value)); }
function_signature(bool, buf_remove_at, BUFFER* buffer, buf_ucount_t index , void* out_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	GOOD_ASSERT(index < buffer->element_count,"Index Out of Range Exception");
	--(buffer->element_count); 
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size); 

	void* dst_ptr = buffer->bytes + index * buffer->element_size;
	if(index <= buffer->element_count)
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
	CALLTRACE_RETURN(true);
}

function_signature(bool, BUFremove_noshift, void* object, bool (*comparer)(void*, void*)) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_remove_noshift(binded_buffer, object, comparer)); }
function_signature(bool, buf_remove_noshift, BUFFER* buffer, void* object, bool (*comparer)(void*, void*))
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
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
			CALLTRACE_RETURN(true); 
		}
	}
	CALLTRACE_RETURN(false); 
}

function_signature(bool, BUFremove, void* object, bool (*comparer)(void*, void*)) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_remove(binded_buffer, object, comparer)); }
function_signature(bool, buf_remove, BUFFER* buffer, void* object, bool (*comparer)(void*, void*))
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");	
	void* cursor = buffer->bytes; 
	for(buf_ucount_t i = 0; i < buffer->element_count; i++, cursor += buffer->element_size)
	{
		if(comparer(object, cursor))
		{
			memcpy(cursor, cursor + buffer->element_size, (buffer->element_count - i - 1) * buffer->element_size); 
			memset(buf_peek_ptr(buffer), 0, buffer->element_size); 
			--(buffer->element_count);
			CALLTRACE_RETURN(true); 
		}
	}
	CALLTRACE_RETURN(false); 
}

function_signature_void(void, BUFfit) { CALLTRACE_BEGIN(); buf_fit(binded_buffer); CALLTRACE_END(); }
function_signature(void, buf_fit, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	//TODO: Replace this with BUFresize(binded_buffer->element_count)
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!"); 
	buffer->bytes =  realloc(buffer->bytes , buffer->element_count * buffer->element_size); 
	GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	buffer->capacity = buffer->element_count;
	CALLTRACE_END();
}

function_signature_void(void*, BUFpeek_ptr) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_peek_ptr(binded_buffer)); }
function_signature(void*, buf_peek_ptr, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!"); 
	CALLTRACE_RETURN(buffer->bytes + (buffer->element_count - 1) * buffer->element_size);
}

function_signature(void, BUFpeek, void* out_value) { CALLTRACE_BEGIN(); buf_peek(binded_buffer, out_value); CALLTRACE_END(); }
function_signature(void, buf_peek, BUFFER* buffer, void* out_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	memcpy(out_value, buffer->bytes + (buffer->element_count - 1) * buffer->element_size , buffer->element_size); 
	CALLTRACE_END();  
}

function_signature(void, BUFpop, void* out_value) { CALLTRACE_BEGIN(); buf_pop(binded_buffer, out_value); CALLTRACE_END(); }
function_signature(void, buf_pop, BUFFER* buffer, void* out_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");	
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	--(buffer->element_count);
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + buffer->element_count * buffer->element_size , buffer->element_size) ; 
	CALLTRACE_END();
}

function_signature(buf_ucount_t, BUFfind_index_of, void* value, bool (*comparer)(void*, void*)) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_find_index_of(binded_buffer, value, comparer)); }
function_signature(buf_ucount_t, buf_find_index_of, BUFFER* buffer, void* value, bool (*comparer)(void*, void*))
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
	void* cursor = buffer->bytes; 
	for(buf_ucount_t i = 0; i < buffer->element_count; i++, cursor += buffer->element_size)
		if(comparer(value, cursor))
			CALLTRACE_RETURN(i); 
	CALLTRACE_RETURN(BUF_INVALID_INDEX);
}

function_signature(void, BUFpush, void* in_value) { CALLTRACE_BEGIN(); buf_push(binded_buffer, in_value); CALLTRACE_END(); }
function_signature(void, buf_push, BUFFER* buffer, void* in_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
	buf_ucount_t new_capacity = (buffer->capacity == 0) ? 1 : buffer->capacity; 
	++(buffer->element_count);
	while(new_capacity < buffer->element_count)
		new_capacity *= 2;
	buf_resize(buffer, new_capacity);
	buf_set_at(buffer, buffer->element_count - 1, in_value);
	CALLTRACE_END();
}

function_signature(void, BUFpushv, void* in_value, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_pushv(binded_buffer, in_value, count); CALLTRACE_END(); }
function_signature(void, buf_pushv, BUFFER* buffer, void* in_value, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "buffer is NULL Exception");
	buf_ucount_t new_capacity = (buffer->capacity == 0) ? 1 : buffer->capacity; 
	buffer->element_count += count;
	while(new_capacity < buffer->element_count)
		new_capacity *= 2;
	buf_resize(buffer, new_capacity);
	for(buf_ucount_t i = 0; i < count; i++, in_value += buffer->element_size)
		buf_set_at(buffer, buffer->element_count - count + i, in_value);
	CALLTRACE_END(); 
}