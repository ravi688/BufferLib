
#include <bufferlib/buffer_test.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef BUF_DEBUG
#	define GOOD_ASSERT(bool_value, string) do { if(!(bool_value)) {  log_fetal_err("Assertion Failed: %s\n", string); } } while(false)
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

#define read_file(...) define_alias_function_macro(read_file, __VA_ARGS__)
function_signature(BUFFER*, read_file, const char* file_name)
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

int main()
{

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
	float* array = BUFget_ptr();

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