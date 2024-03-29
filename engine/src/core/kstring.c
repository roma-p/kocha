#include "core/kstring.h"
#include "core/kmemory.h"
#include <string.h>


char * string_duplicate(const char* str){
    u64 length = string_length(str);
    char * copy = kallocate(length+1, MEMORY_TAG_STRING);
    kcopy_memory(copy, str, length + 1);
    return copy;
    // +1 for null terminator
}

u64 string_length(const char* str) {
    return strlen(str); // TODO: to be replaced by custom optimised implementation. 
}

b8 strings_equal(const char* str_1, const char* str_2){
    return strcmp(str_1, str_2) == 0;
}
