#ifndef DSSE_KEYGEN_H
#define DSSE_KEYGEN_H

#include "MasterKey.h"
class DSSE_KeyGen
{
public:
    DSSE_KeyGen();
    ~DSSE_KeyGen();
    
    int genMaster_key(MasterKey *pKey, unsigned char *pPRK, int PRK_len, unsigned char *pXTS, int XTS_len, unsigned char *pSKM, int SKM_len);
    int genRow_key(unsigned char *pOutData, int out_len, unsigned char *pInData, int in_len, MasterKey *pKey);
     

};

#endif // DSSE_KEYGEN_H
