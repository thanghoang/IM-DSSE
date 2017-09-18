
#ifndef DSSE_PARAM_H
#define DSSE_PARAM_H
#include "DSSE_Hashmap_Key_Class.h"	


#define VARIANT_III
#define C_3
//#define LOAD_FROM_DISK

#define PEER_ADDRESS "tcp://52.205.235.102:4433"
//#define PEER_ADDRESS "tcp://52.38.183.119:4433"



#if defined (C_0)
#define  MAX_NUM_OF_FILES 50176
#define  MAX_NUM_KEYWORDS 240000 
const string noFile = "50000";
#define  NUM_KEYWORDS  238627
#define  NUM_FILES  50000
    
#elif defined(C_1) 
#define  MAX_NUM_OF_FILES 100352
#define  MAX_NUM_KEYWORDS 500000
const string noFile = "100000";
#define  NUM_KEYWORDS  485013
#define  NUM_FILES  100000
    
#elif defined(C_2) 
#define  MAX_NUM_OF_FILES 150528
#define  MAX_NUM_KEYWORDS 660000
const string noFile = "150000";
#define  NUM_KEYWORDS  657323
#define  NUM_FILES  150000    

#elif defined(C_3) 
#define  MAX_NUM_OF_FILES 200704
#define  MAX_NUM_KEYWORDS 820000
const string noFile = "200000";
#define  NUM_KEYWORDS  802797
#define  NUM_FILES  200000    

#elif defined(C_4) 
#define  MAX_NUM_OF_FILES 250880
#define  MAX_NUM_KEYWORDS 940000
const string noFile = "250000";
#define  NUM_KEYWORDS  938868
#define  NUM_FILES  250000    
#endif





//#define INTEL_AES_NI

#include <stdio.h>
#include <string.h>	
#include <stdlib.h>
#include <cerrno>								
#include <algorithm>						
#include <functional>						
#include <iostream>						
#include <fstream>	
#include <sstream>	
#include <bitset>
#include <vector>
#include <iterator>	
#include <dirent.h>	
#include <sys/types.h>			
#include <sys/stat.h>		
#include <unistd.h>
#include <set>												
#include <sparsehash/dense_hash_map>				
#include <boost/algorithm/string/split.hpp>		
#include <boost/algorithm/string.hpp>
#include "climits"
#include <chrono>





//#define PEER_ADDRESS "tcp://52.38.183.119:4433"

#if defined(VARIANT_MAIN)
#define VARIANT 0
    #define ENCRYPT_BLOCK_SIZE 1              // Variant I in bit and should be either 1,2,4 or divisable by 8 and not larger than 128
    static const string gcsDataStructureFilepath = "../example/" + noFile +"F_data_structure_1/client/";
    static const string gcsMatrixPiecePath = "../example/" + noFile + "F_data_structure_1/server/";

#elif defined(VARIANT_I) 
#define VARIANT 1
    #define ENCRYPT_BLOCK_SIZE 128              // Variant I in bit and should be either 1,2,4 or divisable by 8 and not larger than 128
    static const string gcsDataStructureFilepath = "../example/" + noFile +"F_data_structure_128/client/";
    static const string gcsMatrixPiecePath = "../example/" + noFile + "F_data_structure_128/server/";
    
#elif defined(VARIANT_II)
#define VARIANT 2
    #define DECRYPT_AT_CLIENT_SIDE             //VAriant II

    #define ENCRYPT_BLOCK_SIZE 1              // Variant I in bit and should be either 1,2,4 or divisable by 8 and not larger than 128
    static const string gcsDataStructureFilepath = "../example/" + noFile +"F_data_structure_1/client/";
    static const string gcsMatrixPiecePath = "../example/" + noFile + "F_data_structure_1/server/";

#elif defined(VARIANT_III)
#define VARIANT 3
    #define DECRYPT_AT_CLIENT_SIDE             //VAriant II

    #define ENCRYPT_BLOCK_SIZE 128              // Variant I in bit and should be either 1,2,4 or divisable by 8 and not larger than 128
    static const string gcsDataStructureFilepath = "../example/" + noFile +"F_data_structure_128/client/";
    static const string gcsMatrixPiecePath = "../example/" + noFile + "F_data_structure_128/server/";

#endif

//static const string gcsDataStructureFilepath = "../example/data_structure/client/";
//static const string gcsMatrixPiecePath = "../example/data_structure/server/";



static const string gcsFilepath = "../example/input/" + noFile + "F/";			                // path of files directory (Absolute path recommended if enabling ENCRYPT_PHYSICAL_FILE)

static const string gcsEncFilepath = "../example/encrypted_input/";				// path of encrypted files directory (Absolute path recommended if enabling ENCRYPT_PHYSICAL_FILE)
static const string gcsUpdateFilepath = "../example/update/";			        // path of files directory (Absolute path recommended if enabling ENCRYPT_PHYSICAL_FILE)
static const string gcsEncryptedUpdateFilepath = "../example/encrypted_update/";


//Client- Service Define

#define BLOCK_STATE_PIECE_COL_SIZE NUM_BLOCKS//49 //NUM_BLOCKS//20       // in byte

#define MATRIX_PIECE_COL_SIZE  128 //2560         //in byte
#define MATRIX_PIECE_ROW_SIZE 20000//MATRIX_ROW_SIZE //20000         // in bit


#define CLIENT_SERVER_MODE                  // require two machines for client and server
//#define ENCRYPT_PHYSICAL_FILE             // perform AES encryption to encrypt file collection
//#define SEND_SEARCH_FILE_INDEX              // Retrieve detailed indices of matching files of a search query


#if defined(CLIENT_SERVER_MODE) && defined(DECRYPT_AT_CLIENT_SIDE)
#define MULTI_THREAD
#endif


#define LOAD_PREBUILT_DATA_MODE                   // enable it to load the previously created data structures
#define UPLOAD_DATA_STRUCTURE_MANUALLY_MODE       // enable it to manually copy previously created data structure to the server.



#define BLOCK_STATE_PIECE_ROW_SIZE MATRIX_PIECE_ROW_SIZE    
#define BLOCK_STATE_ROW_SIZE MATRIX_ROW_SIZE
#define COL 1
#define ROW 2
using namespace std;	
using google::dense_hash_map;
//using tr1::hash;
using namespace boost::algorithm;

#define ZERO_VALUE 0
#define ONE_VALUE 1	

#define BYTE_SIZE 8	
#define RDRAND_RETRY_NUM 10
#define TRAPDOOR_SIZE 16   
#define NONCE_SIZE 12
#define BLOCK_CIPHER_SIZE 16
															

#define MATRIX_ROW_SIZE MAX_NUM_KEYWORDS

//Loading factors in hash table before resizing
#define FILE_LOADING_FACTOR 0.5
#define KEYWORD_LOADING_FACTOR 0.5



#define MATRIX_COL_SIZE ((MAX_NUM_OF_FILES/BYTE_SIZE))	
	
#define MAC_NAME "MAC"

#define NUM_BLOCKS (MAX_NUM_OF_FILES/ENCRYPT_BLOCK_SIZE)	


//Commands for Client - Server interaction
#define CMD_SEND_DATA_STRUCTURE         0x000010
#define CMD_ADD_FILE_PHYSICAL           0x00000F
#define CMD_DELETE_FILE_PHYSICAL        0x000040
#define CMD_SEARCH_OPERATION            0x000020

#define CMD_REQUEST_BLOCK_DATA          0x000050
#define CMD_REQUEST_SEARCH_DATA         0x000051


#define CMD_UPDATE_BLOCK_DATA           0x000060
#define CMD_SUCCESS                     "CMD_OK"

#define REQUEST_TIMEOUT                 -76

//define the default filename of some data structures in DSSE scheme
#define FILENAME_MATRIX                 "data_structure"
#define FILENAME_GLOBAL_COUNTER         "global_counter"
#define FILENAME_BLOCK_STATE_MATRIX     "block_state_mat"
#define FILENAME_BLOCK_STATE_ARRAY     "block_state_arr"
#define FILENAME_BLOCK_COUNTER_ARRAY     "block_counter_arr"
#define FILENAME_I_PRIME                "i_prime"
#define FILENAME_SEARCH_RESULT          "search_result"

#define KEYWORD_NOT_EXIST MAX_NUM_KEYWORDS+1
#define FILE_NOT_EXIST MAX_NUM_OF_FILES+1

//buffer size of each packet for sending / receiving 
#define SOCKET_BUFFER_SIZE              256

//MACROS
#define BIT_READ(character, position, the_bit)	((*the_bit = *character & (1 << position)))	
#define BIT_SET(character, position) ((*character |= 1 << position))	
#define BIT_CLEAR(character, position) ((*character &= ~(1 << position)))
#define BIT_TOGGLE(character, position)	((*character ^= 1 << position))
#define BIT_CHECK(var,pos) !!((*var) & (1<<(pos)))


static const string gcsKwHashTable = "kw_hashtable";
static const string gcsFileHashTable = "file_hashtable";
static const string gcsListFreeFileIdx = "lstFreeFileIdx";
static const string gcsListFreeKwIdx = "lstFreeKwIdx";

// Delimiter separating unique keywords from files 		
const char* const delimiter = "`-=[]\\;\',./~!@#$%^&*()+{}|:\"<>? \n\t\v\b\r\f\a";	
												
typedef unsigned long long int TYPE_COUNTER;
typedef unsigned long long int TYPE_INDEX;
typedef dense_hash_map<hashmap_key_class,TYPE_INDEX,hashmap_key_class,hashmap_key_class> TYPE_GOOGLE_DENSE_HASH_MAP;
typedef set<string> TYPE_KEYWORD_DICTIONARY;

//Static variables need to be always maintained in the memory for fast operation
static vector<TYPE_INDEX> lstFree_column_idx;
static vector<TYPE_INDEX> lstFree_row_idx;
static TYPE_KEYWORD_DICTIONARY keywords_dictionary;

#define time_now std::chrono::high_resolution_clock::now()

static set<TYPE_INDEX> setSelected_idx;


#endif
