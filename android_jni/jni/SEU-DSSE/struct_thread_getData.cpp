#include "struct_thread_getData.h"

THREAD_GETDATA::THREAD_GETDATA()
{
}

THREAD_GETDATA::~THREAD_GETDATA()
{
}

THREAD_GETDATA::THREAD_GETDATA(TYPE_INDEX idx, MatrixType* data)
{
    this->idx = idx;
    this->data = data;
}
