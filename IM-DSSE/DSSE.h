#ifndef DSSE_H
#define DSSE_H

#include "struct_MatrixType.h"
#include "struct_SearchToken.h"
#include "config.h"



class DSSE{
private:
    prng_state prng;
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

    int pickRandom_element(TYPE_INDEX &randomIdx,vector<TYPE_INDEX> &setIdx,prng_state* prng);
    


public:
    DSSE();
    ~DSSE();
    
    int loadEncrypted_matrix_from_files(MatrixType** I);
    int saveEncrypted_matrix_to_files(MatrixType** );

    int loadBlock_state_matrix_from_file(MatrixType** I);
    int saveBlock_state_matrix_to_file(MatrixType** I);
    int createBlock_state_matrix_files();
    
    int setupData_structure(TYPE_GOOGLE_DENSE_HASH_MAP &rT_W, 
                            TYPE_GOOGLE_DENSE_HASH_MAP &rT_F, 
                            TYPE_COUNTER *pKeywordCounterArray,
                            TYPE_COUNTER *pBlockCounterArray,
                            MatrixType **pBlockStateMatrix,
                            vector<TYPE_INDEX> &lstFree_column_idx,
                            vector<TYPE_INDEX> &lstFree_row_idx,
                            vector<string> &rFileNames, 
                            string path,  
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
                            vector<TYPE_INDEX> &lstFree_column_idx,
                            MasterKey *pKey);
                        
    int addToken(string new_adding_file_with_path,             
                 MatrixType* I_prime,                   
                 TYPE_INDEX &file_index,              
                 TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                 TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
                 TYPE_KEYWORD_DICTIONARY &extracted_keywords,
                 TYPE_COUNTER *pKeywordCounterArray,
                 TYPE_COUNTER *pBlockCounterArray,
                 bool *pBlockStateArray,
                 vector<TYPE_INDEX> &lstFree_column_idx,
                 vector<TYPE_INDEX> &lstFree_row_idx,
                 MasterKey* pKey);   
    int addToken(        string new_adding_file_with_path,
                        MatrixType* I_prime,
                        TYPE_INDEX &file_index, 
                        TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                        TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
                        TYPE_KEYWORD_DICTIONARY &extracted_keywords,
                        unsigned char* decrypt_key_arr,
                        unsigned char* reencrypt_key_arr,
                        vector<TYPE_INDEX> &lstFree_column_idx,
                        vector<TYPE_INDEX> &lstFree_row_idx,
                        MasterKey* pKey);


    int delToken(string new_deleting_file_with_path,              
                 MatrixType* I_prime,                  
                 TYPE_INDEX &file_index, 
                 TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                 TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
                 TYPE_COUNTER *pKeywordCounterArray,
                 TYPE_COUNTER *pBlockCounterArray,
                 bool *pBlockStateArray,
                 vector<TYPE_INDEX> &lstFree_column_idx,
                vector<TYPE_INDEX> &lstFree_row_idx,
                 MasterKey* pKey);   
    
                        
    int delToken(string del_file_with_path,
                MatrixType* I_prime,
                TYPE_INDEX &file_index,
                TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                TYPE_GOOGLE_DENSE_HASH_MAP &rT_W,
                unsigned char* decrypt_key_arr,
                unsigned char* reencrypt_key_arr,
                vector<TYPE_INDEX> &lstFree_column_idx,
                vector<TYPE_INDEX> &lstFree_row_idx,
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
                        

    int saveBlock_state_matrix_to_file(MatrixType** I, int dim, TYPE_INDEX idx);
    int saveEncrypted_matrix_to_files(MatrixType** I, int dim, TYPE_INDEX idx);
    
    int loadEncrypted_matrix_from_files(MatrixType** I, int dim, TYPE_INDEX idx);
    int loadBlock_state_matrix_from_file(MatrixType** I, int dim, TYPE_INDEX idx);
    

};

#endif
