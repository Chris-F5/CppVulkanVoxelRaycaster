#pragma once

#include <stddef.h>
#include <iostream>
#include <stdlib.h>

template<typename T>
class MemPool{
private:
    size_t max;
    size_t allocated;
    T *data;
public:
    MemPool(size_t max);
    size_t allocateBlock();
    void freeBlock(size_t index);
    T* getBlock(size_t index);
    void cleanup();
};

template<typename T>
MemPool<T>::MemPool(size_t max){
    this->max = max;
    this->allocated = 0;
    this->data = (T*)malloc(max * sizeof(T));
}

template<typename T>
void MemPool<T>::cleanup(){
    free(this->data);
}

template<typename T>
size_t MemPool<T>::allocateBlock(){
    if(this->allocated >= this->max)
        throw std::runtime_error("cant allocate more in full pool");
    return this->allocated++;
}

template<typename T>
void MemPool<T>::freeBlock(size_t index){
    throw std:: runtime_error("pool freeing not yet implimented");   
}

template<typename T>
T* MemPool<T>::getBlock(size_t index){
    return &this->data[index];
}
