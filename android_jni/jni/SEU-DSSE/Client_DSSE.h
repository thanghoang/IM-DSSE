#ifndef CLIENT_DSSE_H
#define CLIENT_DSSE_H

#include <MasterKey.h>
#include <DSSE_Param.h>
#include <struct_MatrixType.h>

#if defined(MULTI_THREAD)
#include <pthread.h>
#endif

class Client_DSSE
{
private:
    MasterKey* masterKey;
    
    // file counter
    bool data_structure_constructed;
	
    // Security parameter/Extractor Salt (Deterministic Seed) for generating Krawczyk PRK using RDRAND
	unsigned char extractor_salt[BLOCK_CIPHER_SIZE];
	// Security parameter/Pseudo Random Key (uniform & random intermediate key) for generating Krawczyk KDF (BLOCK_CIPHER_SIZE bytes)
	unsigned char pseudo_random_key[BLOCK_CIPHER_SIZE];

    //Data Structure 
    MatrixType** I;    

    MatrixType* I_prime;

    //counter for keyword
    TYPE_COUNTER keyword_counter_arr[MAX_NUM_KEYWORDS];

    //counter for block
    TYPE_COUNTER block_counter_arr[NUM_BLOCKS];

    bool* block_state_arr;
    MatrixType** block_state_mat;
#if defined(DECRYPT_AT_CLIENT_SIDE)
    unsigned char* row_keys;
	unsigned char* decrypt_key ;
        unsigned char* reencrypt_key ;
#endif

#if defined (LOAD_FROM_DISK)
    MatrixType** I_search;
    MatrixType** I_update;
    
    MatrixType** block_state_mat_search;
    MatrixType** block_state_mat_update;
#endif

#if defined(MULTI_THREAD)    
    pthread_t thread_precomputeAesKey_decrypt;
    pthread_t thread_precomputeAesKey_reencrypt;
    pthread_t thread_getData;
#endif
    
    // Hash map where the trapdoors for the keywords are stored
	TYPE_GOOGLE_DENSE_HASH_MAP T_W;

	// Static hash map where the trapdoors for the files are stored
	TYPE_GOOGLE_DENSE_HASH_MAP T_F;
    
    int requestBlock_data(TYPE_INDEX block_index,
                            MatrixType* I_prime, bool* block_state_arr);
    int sendBlock_data(TYPE_INDEX block_index, MatrixType *I_prime);
    
public:
	
    Client_DSSE();
    ~Client_DSSE();
    
    int genMaster_key();
    int createEncrypted_data_structure();
    
    int searchKeyword(string keyword, TYPE_COUNTER &number);
    
    int addFile(string filename, string path);
    
    int delFile(string filename, string path);
    
#if defined(CLIENT_SERVER_MODE)
    int sendFile(string filename, string path, int SENDING_TYPE);
    
    int requestSearch_data(TYPE_INDEX row_index, MatrixType* I_prime);
#endif

#if defined(LOAD_FROM_DISK)
    int loadData_from_file(int dim, TYPE_INDEX idx);
    int saveData_to_file(int dim, TYPE_INDEX idx);
#endif

#if defined(MULTI_THREAD)

    static void* thread_precomputeAesKey_func(void* param);
    static void* thread_getSearchData_func(void* param);
    static void* thread_getUpdateData_func(void* param);
#endif
static vector<unsigned long long int> stats[16];
};

#endif // CLIENT_DSSE_H
