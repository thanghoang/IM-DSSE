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
		omac_aes128(pOutData, out_len, pInData, in_len, pKey->key3);
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




/**
 * Function Name: precgenerateRow_keys
 *
 * Description:
 * Precompute AES-CTR row keys
 *
 * @param row_counter_arr: (input) row counters
 * @param output: (output) precomputed AES-CTR row keys
 * @param pKey: (input) symmetric key generated by genMasterKey which is used to encrypt data structures
 * @return	0 if successful
 */
int DSSE_KeyGen::pregenerateRow_keys(   TYPE_COUNTER* row_counter_arr,
                                unsigned char output[MATRIX_ROW_SIZE*BLOCK_CIPHER_SIZE],
                                MasterKey *pKey)
{
    TYPE_INDEX row;
    unsigned char row_key [BLOCK_CIPHER_SIZE];
    unsigned char row_key_input [BLOCK_CIPHER_SIZE];
    int ret;
    DSSE_KeyGen* dsse_keygen = new DSSE_KeyGen();
    memset(output,0,BLOCK_CIPHER_SIZE*MATRIX_ROW_SIZE);
    for(row = 0 ; row < MATRIX_ROW_SIZE ; row++)
    {
        memset(row_key,0,sizeof(row_key));
        memset(row_key_input,0,sizeof(row_key_input));
        memcpy(row_key_input,&row,sizeof(row));
        memcpy(&row_key_input[BLOCK_CIPHER_SIZE/2],&row_counter_arr[row],sizeof(row_counter_arr[row]));
        if((ret = dsse_keygen->genRow_key(row_key, BLOCK_CIPHER_SIZE, row_key_input, BLOCK_CIPHER_SIZE, pKey))!=0)
        {
            goto exit;
        }
        memcpy(&output[row*BLOCK_CIPHER_SIZE],row_key,BLOCK_CIPHER_SIZE);
    }
exit:
    memset(row_key,0,BLOCK_CIPHER_SIZE);
    memset(row_key_input,0,BLOCK_CIPHER_SIZE);
    delete dsse_keygen;
    return ret;
}


/**
 * Function Name: enc_dec_preAESKey
 *
 * Description:
 * Encrypt the input data using precomputed AES keys
 *
 * @param output: (output) output data
 * @param input: (input) input data
 * @param preKey: (input) AES_CTR keys which are precomputed using precomputeAES_CTR_keys function
 * @param len: (input) length of the input data (same with length of key)
 * @return	0 if successful
 */
int DSSE_KeyGen::enc_dec_preAESKey(MatrixType* output, 
                            MatrixType* input, 
                            unsigned char preKey[], 
                            TYPE_INDEX len)
{
    int ret = 0;
    TYPE_INDEX idx;
    idx = 0;
    for(idx = 0 ; idx<len;idx++)
    {
        output[idx].byte_data = input[idx].byte_data ^ preKey[idx];
    }
    return ret;
}



/**
 * Function Name: precomputeAES_CTR_keys for DSSE data structure
 *
 * Description:
 * Precompute AES-CTR key
 *
 * @param key: (output) output data
 * @param idx: (input) index
 * @param dim: (input) Dimension that we would like to generate keys for (column or row)
 * @param pregenRow_keys: (input) row keys which are pregenerated by pregenerateRow_keys function
 * @param pKey: (input) symmetric keys generated by genMaster_key() function
 * @return	0 if successful
 */

int DSSE_KeyGen::precomputeAES_CTR_keys(unsigned char* key,
                                                TYPE_INDEX idx, int dim,
                                                TYPE_COUNTER* col_counter_arr, 
                                                unsigned char* pregenRow_keys,
                                                MasterKey* pKey)
{
    unsigned char U[BLOCK_CIPHER_SIZE];
    unsigned char V[BLOCK_CIPHER_SIZE];
    unsigned char uchar_counter[BLOCK_CIPHER_SIZE];
    int ret;
    TYPE_INDEX row,col;
    int bit_position;
    
    TYPE_INDEX i;
    memset(U,0,BLOCK_CIPHER_SIZE);
    if(dim == COL)
    {
        for(row  = 0  ; row < MATRIX_ROW_SIZE;row++)
        {
            memcpy(&uchar_counter[BLOCK_CIPHER_SIZE/2],&col_counter_arr[idx],sizeof(TYPE_COUNTER));
            memcpy(&uchar_counter,&idx,sizeof(TYPE_INDEX));
            
            // AES CTR 128 function
            aes128_ctr_encdec(U, V, &pregenRow_keys[row*BLOCK_CIPHER_SIZE], uchar_counter, ONE_VALUE);                
            
            i = row * ENCRYPT_BLOCK_SIZE / BYTE_SIZE;
            if(ENCRYPT_BLOCK_SIZE<BYTE_SIZE)
            {
                bit_position = row * ENCRYPT_BLOCK_SIZE % BYTE_SIZE;
                for(int bit_number =0 ; bit_number<ENCRYPT_BLOCK_SIZE;bit_number++)
                {
                    if(BIT_CHECK(&V[0],bit_number))
                        BIT_SET(&key[i],(bit_position+bit_number));
                    else
                        BIT_CLEAR(&key[i],(bit_position+bit_number));
                }
            }
            else
            {
                memcpy(&key[i],V,ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
            }
        }
    }
    else //key for search
    {
        row = idx;
        for(TYPE_INDEX block_index = 0 ; block_index < NUM_BLOCKS; block_index++)
        {
            memcpy(&uchar_counter[BLOCK_CIPHER_SIZE/2],&col_counter_arr[block_index],sizeof(col_counter_arr[block_index]));
            memcpy(&uchar_counter,&block_index,sizeof(TYPE_INDEX));
            
            // AES CTR 128 function
            aes128_ctr_encdec(U, V, &pregenRow_keys[row*BLOCK_CIPHER_SIZE], uchar_counter, ONE_VALUE);
            
            i = block_index * ENCRYPT_BLOCK_SIZE / BYTE_SIZE;
                
            if(ENCRYPT_BLOCK_SIZE < BYTE_SIZE)
            {
                bit_position = (block_index * ENCRYPT_BLOCK_SIZE) % BYTE_SIZE;
                for(int bit_number = 0 ; bit_number < ENCRYPT_BLOCK_SIZE; bit_number++)
                {
                    if(BIT_CHECK(&V[0],bit_number))
                        BIT_SET(&key[i],(bit_position+bit_number));
                    else
                        BIT_CLEAR(&key[i],(bit_position+bit_number));
                }
            }
            else
            {
                memcpy(&key[i],V,ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
            }
        }
    }
exit:
    return ret;
}
