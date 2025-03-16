
#include <bufferlib/buffer_test.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef BUF_DEBUG
#	define GOOD_ASSERT(bool_value, string) do { if(!(bool_value)) {  printf("Assertion Failed: %s\n", string); exit(EXIT_FAILURE); } } while(false)
#else
#	define GOOD_ASSERT(...)
#endif

bool equal_ptr(void* ptr1, void* ptr2)
{
	return (*(char**)ptr1) == (*(char**)ptr2);
}

void print_ptr(void* ptr)
{
	printf("%d ", *(int*)ptr); 
}

BUFFER* read_file(const char* file_name)
{
    BUFpush_binded();
    FILE* file = fopen(file_name, "r");
    GOOD_ASSERT(file != NULL, "file opening failed!");
    fseek(file, 0, SEEK_END);
    uint64_t size = ftell(file); 
    fseek(file, 0, SEEK_SET);
    BUFFER* read_buffer = BUFcreate(NULL, sizeof(char), size, 1); 
    BUFbind(read_buffer);
    BUFset_element_count(size);
    char null_value = 0;
    BUFset_offset_bytes(&null_value); 
    fread(BUFget_ptr(), sizeof(char), BUFget_element_count(), file);
    fclose(file);
    BUFpop_binded();
    return read_buffer;
}

bool cmp_float(void* f1, void* f2) { return (*(float*)f1) == (*(float*)f2); }

void test_pushv()
{
	BUFFER buffer = buf_create(sizeof(double), 0, 0);
	double values[4] = { 100.0, 200.0, 300.0, 400.0 };
	buf_pushv(&buffer, values, 4);

	for(buf_ucount_t i = 0; i < buf_get_element_count(&buffer); i++)
		printf("%f ", *(double*)buf_getptr_at(&buffer, i));

	buf_free(&buffer);
}

#define _assert(assertion, desc) __assert(assertion, desc, __LINE__, __FUNCTION__, __FILE__)
static void __assert(bool assertion, const char* desc, u32 line, const char* fn, const char* fl)
{
	if(assertion)
		return;
	printf("[Failed] %s, at %u, %s, %s", desc, line, fn, fl);
	exit(-1);
}

static void test_buf_resize()
{
	buffer_t b = buf_create(sizeof(int), 0, 0);
	_assert(buf_get_element_count(&b) == 0, "either buf_create or buf_get_element_count is not working");
	_assert(buf_get_ptr(&b) == NULL, "either buf_create or buf_get_ptr is not working");
	_assert(buf_get_capacity(&b) == 0, "either buf_create or buf_get_capacity is not working");
	_assert(buf_get_element_size(&b) == 4, "either buf_create or buf_get_element_count is not working");
	int value = 143;
	buf_push(&b, &value);
	_assert(buf_get_element_count(&b) == 1, "either buf_push or buf_get_element_count is not working");
	_assert(buf_get_ptr(&b) != NULL, "either buf_push or buf_get_ptr is not working");
	_assert(buf_get_capacity(&b) == 1, "either buf_push or buf_get_capacity is not working");
	_assert(buf_get_element_size(&b) == 4, "either buf_push or buf_get_element_count is not working");
	for(int i = 0; i < 100; i++)
		buf_push(&b, &i);
	_assert(buf_get_at_typeof(&b, int, 0) == 143, "either buf_push or buf_get_at_typeof is not working");
	for(int i = 1; i < 101; i++)
		_assert(buf_get_at_typeof(&b, int, i) == (i - 1), "either buf_push or buf_get_at_typeof is not working");
	_assert(buf_get_element_count(&b) == 101, "either buf_create or buf_get_element_count is not working");
	_assert(buf_get_ptr(&b) != NULL, "either buf_create or buf_get_ptr is not working");
	_assert(buf_get_capacity(&b) == 128, "either buf_create or buf_get_capacity is not working");
	_assert(buf_get_element_size(&b) == 4, "either buf_create or buf_get_element_count is not working");
	buf_ensure_capacity(&b, 967);
	_assert(buf_get_capacity(&b) == 1024, "either buf_ensure_capacity or buf_get_capacity is not working");
	buf_resize(&b, 101);
	for(int i = 1; i < 101; i++)
		_assert(buf_get_at_typeof(&b, int, i) == (i - 1), "either buf_resize or buf_get_at_typeof is not working");
	_assert(buf_get_element_count(&b) == 101, "either buf_resize or buf_get_element_count is not working");
	_assert(buf_get_ptr(&b) != NULL, "either buf_resize or buf_get_ptr is not working");
	_assert(buf_get_capacity(&b) == 101, "either buf_resize or buf_get_capacity is not working");
	_assert(buf_get_element_size(&b) == 4, "either buf_resize or buf_get_element_count is not working");
	buf_resize(&b, 0);
	_assert(buf_get_element_count(&b) == 0, "either buf_resize or buf_get_element_count is not working");
	_assert(buf_get_ptr(&b) == NULL, "either buf_resize or buf_get_ptr is not working");
	_assert(buf_get_capacity(&b) == 0, "either buf_resize or buf_get_capacity is not working");
	_assert(buf_get_element_size(&b) == 4, "either buf_resize or buf_get_element_count is not working");
	for(int i = 0; i < 100; i++)
		buf_insert_at(&b, 0, &i);
	_assert(buf_get_element_count(&b) == 100, "either buf_insert_at or buf_get_element_count is not working");
	_assert(buf_get_ptr(&b) != NULL, "either buf_insert_at or buf_get_ptr is not working");
	_assert(buf_get_capacity(&b) == 128, "either buf_insert_at or buf_get_capacity is not working");
	_assert(buf_get_element_size(&b) == 4, "either buf_insert_at or buf_get_element_count is not working");
	for(int i = 99; i >= 0; i--)
		_assert(buf_get_at_typeof(&b, int, 99 - i) == i, "either buf_insert_at or buf_get_at_typeof is not working");
	buf_insert_pseudo(&b, 43, 65);
	_assert(buf_get_element_count(&b) == 165, "either buf_insert_at or buf_get_element_count is not working");
	_assert(buf_get_ptr(&b) != NULL, "either buf_insert_at or buf_get_ptr is not working");
	_assert(buf_get_capacity(&b) == 256, "either buf_insert_at or buf_get_capacity is not working");
	_assert(buf_get_element_size(&b) == 4, "either buf_insert_at or buf_get_element_count is not working");
	for(int i = 0; i < 43; i++)
		_assert(buf_get_at_typeof(&b, int, i) == (99 - i), "either buf_insert_pseudo or buf_get_at_typeof is not working");
	for(int i = 43; i < 100; i++)
		_assert(buf_get_at_typeof(&b, int, i + 65) == (99 - i), "either buf_insert_pseudo or buf_get_at_typeof is not working");
	buf_fit(&b);
	_assert(buf_get_element_count(&b) == 165, "either buf_fit or buf_get_element_count is not working");
	_assert(buf_get_ptr(&b) != NULL, "either buf_fit or buf_get_ptr is not working");
	_assert(buf_get_capacity(&b) == 165, "either buf_fit or buf_get_capacity is not working");
	_assert(buf_get_element_size(&b) == 4, "either buf_fit or buf_get_element_count is not working");
	buf_free(&b);
	puts("Following functions seem to be working:\n"
			"\tbuf_create\n"
			"\tbuf_get_element_count\n"
			"\tbuf_get_ptr\n"
			"\tbuf_get_capacity\n"
			"\tbuf_get_element_size\n"
			"\tbuf_push\n"
			"\tbuf_get_at_typeof\n"
			"\tbuf_ensure_capacity\n"
			"\tbuf_resize\n"
			"\tbuf_insert_at\n"
			"\tbuf_insert_pseudo\n"
			"\tbuf_fit\n"
			"\tbuf_free\n");
}

int main()
{
	test_buf_resize();

	BUFFER string_buffer = buf_create(sizeof(char), 0, 0);
	buf_push_string(&string_buffer, "Hello World");
	buf_push_newline(&string_buffer);
	char stage_buffer[512];
	buf_printf(&string_buffer, stage_buffer, "My name is %s\n", "Ravi Prakash Singh");
	buf_printf(&string_buffer, stage_buffer, "My name is %s\n", "Ravi Prakash Singh");
	buf_printf(&string_buffer, stage_buffer, "My name is %s\n", "Ravi Prakash Singh");
	buf_printf(&string_buffer, stage_buffer, "My name is %s\n", "Ravi Prakash Singh");
	buf_printf(&string_buffer, stage_buffer, "My name is %s\n", "Ravi Prakash Singh");
	buf_printf(&string_buffer, stage_buffer, "My name is %s\n", "Ravi Prakash Singh");
	buf_printf(&string_buffer, stage_buffer, "My name is %s\n", "Ravi Prakash Singh");
	buf_push_null(&string_buffer);
	puts(buf_get_ptr(&string_buffer));
	buf_free(&string_buffer);

	test_pushv();
	int __a = 100;
	bool result = BUFstart_default_testing();
	if(!result)
		puts("One or More Tests are Failed");
	else 
		puts("Passed");
	puts("\n");

	BUFFER* buffer = BUFcreate(NULL, sizeof(float), 0, 0); 
	BUFbind(buffer);
	float value = 100;
	BUFpush(&value); value += 1;
	BUFpush(&value); value += 1;
	BUFpush(&value); value += 1;
	BUFpush(&value); value += 1;
	BUFpush(&value); value += 1;
	BUFpush(&value);

	value = 101;
	if(!BUFremove(&value, cmp_float))
		printf("Not found: %f\n", value);
	// value = 102;
	// if(!BUFremove(&value, cmp_float))
	// 	printf("Not found: %f\n", value);

	BUFlog();
	__attribute__((unused)) float* array = BUFget_ptr();

	for(int i = 0; i < BUFget_element_count(); i++)
		printf("%f ", BUFget_value_at_typeof(float, i));
	puts("");

	BUFFER* clone = BUFget_clone();
	BUFbind(clone); 

	printf("Cloned: "); 

	for(int i = 0; i < BUFget_element_count(); i++)
		printf("%f ", BUFget_value_at_typeof(float, i));
	puts("");

	BUFfree();
	BUFbind(buffer); 
	BUFfree();

	BUFFER* text = read_file("Hello.txt");
	BUFbind(text);
	puts(BUFget_ptr());
	BUFlog();
	BUFfree();
	if(__a != 100)
		printf("[Error]: Buffer Overflow");
	return 0;
}