#ifndef CLIENT_DSSE_H
#define CLIENT_DSSE_H

#include <MasterKey.h>
#include <config.h>
#include <struct_MatrixType.h>

#include <pthread.h>

class Client_DSSE
{
private:
    MasterKey* masterKey;
    
    prng_state prng;
    bool ready = false;
    
    MatrixType* I_prime;
    MatrixType** block_state_mat;


    //block state
    bool* block_state_arr;    
    //counter for keyword
    TYPE_COUNTER keyword_counter_arr[MAX_NUM_KEYWORDS];
    //counter for block
    TYPE_COUNTER block_counter_arr[NUM_BLOCKS];


    unsigned char* row_keys;
    unsigned char* decrypt_key;
    unsigned char* reencrypt_key;

    
    vector<TYPE_INDEX> lstFree_column_idx;
    vector<TYPE_INDEX> lstFree_row_idx;

    // Hash map where the trapdoors for the keywords are stored
	TYPE_GOOGLE_DENSE_HASH_MAP T_W;

	// Static hash map where the trapdoors for the files are stored
	TYPE_GOOGLE_DENSE_HASH_MAP T_F;

    pthread_t thread_precomputeAesKey_decrypt;
    pthread_t thread_precomputeAesKey_reencrypt;
    pthread_t thread_getData;
    

    
public:
	
    Client_DSSE();
    ~Client_DSSE();
    
    
    int genMaster_key();
    int createEncrypted_data_structure();
    
    int sendEncryptedIndex();
    int sendCommandOnly(int cmd);
    int loadState();
    int saveState();
    
    
    int searchKeyword(string keyword, TYPE_COUNTER &number);
    int addFile(string filename, string path);
    int delFile(string filename, string path);
    


    int sendFile(string filename, string path, int SENDING_TYPE);    
    int requestSearch_data(TYPE_INDEX row_index, MatrixType* I_prime);
    int requestBlock_data(TYPE_INDEX block_index, MatrixType* I_prime, bool* block_state_arr);
    int sendBlock_data(TYPE_INDEX block_index, MatrixType *I_prime);

    
    static void* thread_precomputeAesKey_func(void* param);
    static void* thread_getSearchData_func(void* param);
    static void* thread_getUpdateData_func(void* param);
};

#endif // CLIENT_DSSE_H
