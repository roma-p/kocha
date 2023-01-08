#include "containers/darray.h"
#include "core/kmemory.h"
#include "core/logger.h"

void* _darray_create(u64 length, u64 stride) {
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);
    u64 array_size  = length * stride;
    u64* new_array   = kallocate(header_size + array_size, MEMORY_TAG_DARRAY);
    kset_memory(new_array, 0, header_size + array_size); // TODO: USELESS, DONE IN KALLOCATE OR I MISSED
                             // SOMETHING...    
    new_array[DARRAY_CAPACITY] = length;
    new_array[DARRAY_LENGTH]   = 0;
    new_array[DARRAY_STRIDE]   = stride;

    // pointer logic: return a pointer to new_array but slided of the header length! 
    return (void*)(new_array + DARRAY_FIELD_LENGTH);  
}

void _darray_destroy(void* array){
    u64* total_array = (u64*) array - DARRAY_FIELD_LENGTH; // getting a pointer to the header with
                               // pointer logic.
    u64 header_size = DARRAY_FIELD_LENGTH * sizeof(u64);
    u64 total_size  = header_size + total_array[DARRAY_CAPACITY] * total_array[DARRAY_STRIDE];
    kfree(total_array, total_size, MEMORY_TAG_DARRAY);
}

u64 _darray_field_get(void *array, u64 field){
    u64* total_array = (u64*) array - DARRAY_FIELD_LENGTH;
    return total_array[field];
}

void _darray_field_set(void *array, u64 field, u64 value){
    u64* total_array = (u64*) array - DARRAY_FIELD_LENGTH;
    total_array[field] = value;
}

void* _darray_resize(void* array){
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    void* tmp  = _darray_create((DARRAY_RESIZE_FACTOR * darray_capacity(array)), stride);
    kcopy_memory(tmp, array, length * stride); // FIXME: why not use realloc? 
    _darray_field_set(tmp, DARRAY_LENGTH, length);
    _darray_destroy(array);
    return tmp;
}

void* _darray_push(void* array, const void* value_ptr){
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if(length >= darray_capacity(array)){
    array = _darray_resize(array);
    }
    u64 addr = (u64)array;
    addr += (length * stride);
    kcopy_memory((void*)addr, value_ptr, stride);
    _darray_field_set(array, DARRAY_LENGTH, length + 1);
    return array;
}

void _darray_pop(void* array, void* dest){
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    u64 addr   = (u64)array;
    addr += ((length - 1)* stride);
    kcopy_memory(dest, (void*)addr, stride);
    _darray_field_set(array, DARRAY_LENGTH, length - 1);
}

void* _darray_pop_at(void* array, u64 index, void* dest) {
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if(index >= length) {
    LOG_ERROR("Index outside the bounds of this array! Length: %i, index: %i", length, index);
    return array;
    }

    u64 addr = (u64)array;
    kcopy_memory(dest, (void*)(addr + (index * stride)), stride);

    if(index != length - 1) {
    kcopy_memory(
        (void*)(addr + (index * stride)),
        (void*)(addr + ((index + 1) * stride)),
        stride * (length - index)
    );
    }
    _darray_field_set(array, DARRAY_LENGTH, length - 1);
    return array;
}

void* _darray_insert_at(void* array, u64 index, void* value_ptr){
    u64 length = darray_length(array);
    u64 stride = darray_stride(array);
    if(index >= length) {
    LOG_ERROR("Index outside the bounds of this array! Length: %i, index: %i", length, index);
    return array;
    }
    if (length >= darray_capacity(array)){
    array = _darray_resize(array);
    }

    u64 addr = (u64)array;

    if (index != length - 1) {
    kcopy_memory(   
        (void*)(addr + ((index + 1) * stride)),
        (void*)(addr + (index * stride)),
        stride * (length - index)
    );
    }

    kcopy_memory((void*)(addr + (index * stride)), value_ptr, stride);
    _darray_field_set(array, DARRAY_LENGTH, length + 1);
    return array;
}

