#ifndef STRUCT_THREAD_GETDATA_H
#define STRUCT_THREAD_GETDATA_H
#include "struct_MatrixType.h"
#include "config.h"
typedef struct THREAD_GETDATA
{
    MatrixType* data;
    TYPE_INDEX idx;
    THREAD_GETDATA();
    ~THREAD_GETDATA();
    THREAD_GETDATA(TYPE_INDEX idx, MatrixType* data);
};

#endif // STRUCT_THREAD_GETDATA_H
