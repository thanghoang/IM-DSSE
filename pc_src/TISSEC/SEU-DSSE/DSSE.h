#ifndef DSSE_H
#define DSSE_H

#include "struct_MatrixType.h"
#include "struct_SearchToken.h"

#define KEYWORD_TOKEN_GENERATION_ERR            -0x00000001
#define DATA_STRUCTURE_NOT_BUILT_ERR            -0x00000002
#define MAX_KEYWORD_INDEX_EXCEEDED_ERR          -0x00000003
#define MAX_FILE_INDEX_EXCEEDED_ERR             -0x00000004
#define MAX_NUM_BLOCK_EXCEEDED_ERR              -0x00000005
#define FILE_OPEN_ERR                           -0x00000006
#define KEY_NULL_ERR                            -0x00000007


#define KEY_GENERATION_ERR                      -0x00000008
#define SETUP_DATA_STRUCTURE_ERR                -0x00000009
#define ENCRYPT_DATA_STRUCTURE_ERR              -0x0000000A
#define INITIALIZE_MATRIX_ERR                   -0x0000000B
#define SEARCH_ERR                              -0x0000000C
#define ADD_TOKEN_ERR                           -0x0000000D
#define DELETE_TOKEN_ERR                        -0x0000000E
#define ADD_ERR                                 -0x0000000F
#define DELETE_ERR                              -0x00000010
#define REQUEST_BLOCK_IDX_ERR                   -0x00000011
#define FETCH_BLOCK_DATA_ERR                    -0x00000012
#define BIT_FIELD_ACCESS_ERR                    -0x00000013


#define SEARCH_TOKEN_ROW_IDX_ERR                -0x00000101




#define CLIENT_SEND_FILE_ERR                    -0x00000300
#define CLIENT_CREATE_DATA_STRUCTURE_ERR        -0x00000301
#define CLIENT_SEARCH_ERR                       -0x00000302
#define CLIENT_REQUEST_BLOCK_DATA_ERR           -0x00000303
#define CLIENT_UPDATE_BLOCK_DATA_ERR            -0x00000304
#define CLIENT_ADD_FILE_ERR                     -0x00000305
#define CLIENT_DELETE_FILE_ERR                  -0x00000306
         


#define HASH_TABLE_NULL_ERR                     -0x00000100
#define COUNTER_EXCEED_LIMIT                    -0x00000101


class DSSE{
private:
    
    int scanDatabase(
		vector<string> &rFileNames,
		TYPE_KEYWORD_DICTIONARY &rKeywordsDictionary,
        TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
		TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
        string path,
        MasterKey* pKey);
        
    int createKeyword_file_pair(
        vector<vector<TYPE_INDEX>> &kw_file_pair,
		TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
		TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
        vector<TYPE_INDEX> &lstFree_keyword_idx,
        vector<TYPE_INDEX> &lstFree_file_idx,
		string path,
		MasterKey *pKey);
        
    int createEncrypted_matrix_from_kw_file_pair(vector<vector<TYPE_INDEX>> &kw_file_pair,
                                                    TYPE_COUNTER* row_counter_arr,
                                                    TYPE_COUNTER* block_counter_arr,
                                                    MasterKey *pKey);

    int pickRandom_element(TYPE_INDEX &randomIdx,vector<TYPE_INDEX> &setIdx);
    


public:
    DSSE();
    ~DSSE();
    
    
    int loadEncrypted_matrix_from_files(MatrixType** I);
    int loadBlock_state_matrix_from_file(MatrixType** I);
    int createBlock_state_matrix_files();
    
    int setupData_structure(MatrixType **I, 
                            TYPE_GOOGLE_DENSE_HASH_MAP &rT_W, 
                            TYPE_GOOGLE_DENSE_HASH_MAP &rT_F, 
                            TYPE_COUNTER *pKeywordCounterArray,
                            TYPE_COUNTER *pBlockCounterArray,
                            MatrixType **pBlockStateMatrix,
                            vector<string> &rFileNames, 
                            string path, 
                            string encrypted_files_path, 
                            MasterKey *pKey);
                                
    int searchToken(SEARCH_TOKEN &pSearchToken, 
                    string keyword,
                    TYPE_GOOGLE_DENSE_HASH_MAP &rT_W, 
                    TYPE_COUNTER *pKeywordCounterArray,
                    MasterKey *pKey);
    
    int search( vector<TYPE_INDEX> &rFileIDs, 
                SEARCH_TOKEN pSearchToken, 
                MatrixType **I,
                TYPE_COUNTER *pBlockCounterArray,
                MatrixType **pBlockStateMatrix);
                
    int requestBlock_index(string adding_filename_with_pad,
                            TYPE_INDEX &block_index,
                            TYPE_GOOGLE_DENSE_HASH_MAP &r_TF,
                            MasterKey *pKey);
                        
    int addToken(string new_adding_file_with_path,             
                 MatrixType* I_prime,                   
                 string encrypted_filename,      
                 TYPE_INDEX &file_index,              
                 TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                 TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
                 TYPE_KEYWORD_DICTIONARY &extracted_keywords,
                 TYPE_COUNTER *pKeywordCounterArray,
                 TYPE_COUNTER *pBlockCounterArray,
                 bool *pBlockStateArray,
                 MasterKey* pKey);   
    int addToken(        string new_adding_file_with_path,
                        MatrixType* I_prime,
                        string encrypted_file_path,           
                        TYPE_INDEX &file_index, 
                        TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                        TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
                        TYPE_KEYWORD_DICTIONARY &extracted_keywords,
                        unsigned char* decrypt_key_arr,
                        unsigned char* reencrypt_key_arr,
                        MasterKey* pKey);


    int delToken(string new_deleting_file_with_path,              
                 MatrixType* I_prime,                  
                 TYPE_INDEX &file_index, 
                 TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                 TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
                 TYPE_COUNTER *pKeywordCounterArray,
                 TYPE_COUNTER *pBlockCounterArray,
                 bool *pBlockStateArray,
                 MasterKey* pKey);   
    
                        
    int delToken(string del_file_with_path,
                MatrixType* I_prime,
                TYPE_INDEX &file_index,
                TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
                unsigned char* decrypt_key_arr,
                unsigned char* reencrypt_key_arr,
                MasterKey* pKey);
                
                
	int update(MatrixType* I_prime,               
            TYPE_INDEX block_idx,                             
            MatrixType** I,
            TYPE_COUNTER *pBlockCounterArray,  
            MatrixType **pBlockStateMatrix);
   
    
    int updateBlock(    MatrixType* updating_block,
                        MatrixType* input_block,
                        TYPE_INDEX update_idx);
    
    
    int getBlock( TYPE_INDEX index,    
                        int dim,
                        MatrixType** I,
                        MatrixType* I_prime);
                        
#if defined(LOAD_FROM_DISK)

    int saveBlock_state_matrix_to_file(MatrixType** I, int dim, TYPE_INDEX idx);
    int saveEncrypted_matrix_to_files(MatrixType** I, int dim, TYPE_INDEX idx);
    int loadEncrypted_matrix_from_files(MatrixType** I, int dim, TYPE_INDEX idx);
    int loadBlock_state_matrix_from_file(MatrixType** I, int dim, TYPE_INDEX idx);
    
#endif

};

#endif
