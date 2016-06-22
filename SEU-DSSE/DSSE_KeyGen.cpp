#include "DSSE_KeyGen.h"
#include "Krawczyk128_KDF.h"        //for key generation
#include "DSSE_Crypto.h"        // for traditional crypto primitives
#include "DSSE.h"
#include "Miscellaneous.h"
DSSE_KeyGen::DSSE_KeyGen()
{

}

DSSE_KeyGen::~DSSE_KeyGen()
{
    
}


/**
 * Function Name: genRow_key
 *
 * Description:
 * Generate a row keys to encrypt row in the DSSE encrypted data structure 
 *
 * @param pOutData: (output) row key
 * @param out_len: (output) length of row key 
 * @param pInData: (input) IV (formed by row counter and row index)
 * @param in_len: (input) length of IV
 * @param pKey: (input) symmetric key
 * @return	0 if successful
 */
int DSSE_KeyGen::genRow_key(unsigned char *pOutData,
                     int out_len,
                     unsigned char *pInData,
                     int in_len,
                     MasterKey *pKey) 
{
	memset(pOutData,0,out_len);
    
    if(out_len>0 && in_len>0)
    {
		// Generate the row key using OMAC AES CTR 128 function
		omac_aes128_intel(pOutData, out_len, pInData, in_len, pKey->key3);
	}
    else
    {
		cout << "Either length of input or output to generate_row_key is <= 0" << endl;
    }
	return 0;
}

/**
 * Function Name: genMaster_key
 *
 * Description:
 * Generate all symmetric keys used for DSSE encrypted data structure, file collection, and client hash tables
 *
 * @param pKey: (output) symmetric keys being generated
 * @param pPRK: (input) pseudo random key 
 * @param PRK_len: (input) length of pseudo random key
 * @param pXTS: (input) extractor salt
 * @param XTS_len: (input) length of extractor salt
 * @param pSKM: (input) Source key material
 * @param SKM_len: (input) length of source key material
 * @return	0 if successful
 */
int DSSE_KeyGen::genMaster_key(MasterKey *pKey,
        unsigned char *pPRK,
        int PRK_len,
        unsigned char *pXTS,
        int XTS_len,
        unsigned char *pSKM,
        int SKM_len)
{
    
    string key_loc;
    Krawczyk128_KDF* Kraw = new Krawczyk128_KDF();
    int ret;
#if !defined(LOAD_PREBUILT_DATA_MODE)

	cout << "Entering dynamicsse_keygen function" << endl << endl;

	int error = 0;
	double elapsed = 0, start_time = 0, end_time = 0;
	if(pKey->isNULL() || pPRK == NULL || pXTS == NULL || pSKM == NULL )
    {
        ret = KEY_NULL_ERR;
        goto exit;
    }
	
	// Generate Extractor salt (Deterministic Seed) using RDRAND
	if ((error = Kraw->generate_XTS(pXTS, XTS_len)) != CRYPT_OK) 
    {
		printf("Error calling generate_XTS: %d\n", error);
		ret = KEY_GENERATION_ERR;
        goto exit;
	}

	// Generate Source Key Material using RDRAND
	if((error = Kraw->generate_128_SKM(pSKM, SKM_len)) != CRYPT_OK) 
    {
		printf("Error calling generate_128_SKM: %d\n", error);
		ret = KEY_GENERATION_ERR;
        goto exit;
	}

	// Generate Pseudo Random Key using OMAC-128
	if((error = Kraw->generate_128_PRK(pPRK, PRK_len, pXTS, XTS_len, pSKM, SKM_len)) != CRYPT_OK) 
    {
		printf("Error calling generate_128_PRK function: %d\n", error);
		ret = KEY_GENERATION_ERR;
        goto exit;
	}

	// Generate key1 using Krawczyk Key Derivation Function
	if((error = Kraw->generate_krawczyk_128_KDF(pKey->key1, BLOCK_CIPHER_SIZE, (unsigned char *)"key1", 4, pPRK, PRK_len)) != CRYPT_OK) 
    {
		printf("Error calling krawczyk_128_kdf function: %d\n", error);
		ret = KEY_GENERATION_ERR;
        goto exit;
	}

	// Generate key2 using Krawczyk Key Derivation Function
	if((error = Kraw->generate_krawczyk_128_KDF(pKey->key2, BLOCK_CIPHER_SIZE, (unsigned char *)"key2", 4, pPRK, PRK_len)) != CRYPT_OK) 
    {
		printf("Error calling krawczyk_128_kdf function: %d\n", error);
		ret = KEY_GENERATION_ERR;
        goto exit;
	}

	// Generate key3 using Krawczyk Key Derivation Function
	if((error = Kraw->generate_krawczyk_128_KDF(pKey->key3, BLOCK_CIPHER_SIZE, (unsigned char *)"key3", 4, pPRK, PRK_len)) != CRYPT_OK) 
    {
		printf("Error calling krawczyk_128_kdf function: %d\n", error/*error_to_string(error)*/);
		ret = KEY_GENERATION_ERR;
        goto exit;
	}
    
    //write keys to file
    key_loc = gcsDataStructureFilepath + "key1";
    Miscellaneous::write_file_cpp(key_loc,pKey->key1,BLOCK_CIPHER_SIZE);
    
    key_loc = gcsDataStructureFilepath + "key2";
    Miscellaneous::write_file_cpp(key_loc,pKey->key2,BLOCK_CIPHER_SIZE);
    
    key_loc = gcsDataStructureFilepath + "key3";
    Miscellaneous::write_file_cpp(key_loc,pKey->key3,BLOCK_CIPHER_SIZE);
    
#else //load key from file
    
    key_loc = gcsDataStructureFilepath + "key1";
    Miscellaneous::read_file_cpp(pKey->key1,BLOCK_CIPHER_SIZE,key_loc);
    
    key_loc = gcsDataStructureFilepath + "key2";
    Miscellaneous::read_file_cpp(pKey->key2,BLOCK_CIPHER_SIZE,key_loc);
    
    key_loc = gcsDataStructureFilepath + "key3" ;
    Miscellaneous::read_file_cpp(pKey->key3,BLOCK_CIPHER_SIZE,key_loc);
    
#endif
    ret = 0;
exit:
    delete Kraw;
	return ret;
}

