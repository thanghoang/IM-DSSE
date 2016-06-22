#ifndef CLIENT_DSSE_H
#define CLIENT_DSSE_H

#include <MasterKey.h>
#include <DSSE_Param.h>
#include <struct_MatrixType.h>

class Client_DSSE
{
private:
    MasterKey* masterKey;
    
    // Global & static file counter
    bool data_structure_constructed;
	static TYPE_COUNTER gc;

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
    bool** block_state_mat;
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
#endif

};

#endif // CLIENT_DSSE_H
