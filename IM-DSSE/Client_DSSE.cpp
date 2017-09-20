#include "Client_DSSE.h"
#include "config.h"
#include "DSSE_KeyGen.h"
#include "string.h"
#include <sstream>	
#include "Miscellaneous.h"
#include <tomcrypt.h>
#include "DSSE.h"
#include "math.h"

#include "zmq.hpp"
using namespace zmq;



#include "struct_thread_precompute_aeskey.h"
#include "struct_thread_getData.h"



Client_DSSE::Client_DSSE()
{
    
    int err;
    if ((err = register_prng(&fortuna_desc)) != CRYPT_OK) 
    {
		printf("Error registering Fortuna PRNG : %s\n", error_to_string(err));
	}

	if ((err = find_prng("fortuna")) != CRYPT_OK) 
    {
		printf("Invalid PRNG : %s\n", error_to_string(err));
	}

	/* start it */
	if ((err = fortuna_start(&prng)) != CRYPT_OK) 
    {
		printf("Start error: %s\n", error_to_string(err));
	}

	if ((err = fortuna_add_entropy((unsigned char*)seed.c_str(), seed.size(), &prng)) != CRYPT_OK) 
    {
		printf("Add_entropy error: %s\n", error_to_string(err));
	}
    if ((err = fortuna_ready(&prng)) != CRYPT_OK) 
    {
		printf("Ready error: %s\n", error_to_string(err));
	}
    
    /* Allocate memory for I' & block state array */
        I_prime = new MatrixType[MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE];
        memset(I_prime,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
        
#if !defined(DECRYPT_AT_CLIENT_SIDE)
        block_state_arr = new bool[MATRIX_ROW_SIZE];
        memset(block_state_arr,0,MATRIX_ROW_SIZE);
#else
        decrypt_key = new unsigned char[MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE];
        reencrypt_key = new unsigned char[MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE];
        memset(decrypt_key,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
        memset(reencrypt_key,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);

#endif
}

Client_DSSE::~Client_DSSE()
{
    
}


/**
 * Function Name: genMaster_key
 *
 * Description:
 * Generate symmetric keys used to encrypt DSSE data structure, files, and hash tables
 *
 * @return	0 if successful
 */
int Client_DSSE::genMaster_key()
{
    DSSE_KeyGen *dsse_key = new DSSE_KeyGen();
    this->masterKey = new MasterKey();
    dsse_key->genMaster_key(this->masterKey,&prng);
    
    delete dsse_key;
    return 0;
}
/**
 * Function Name: saveState
 *
 * Description:
 * Save current state from memorty to disk
 *
 * @return	0 if successful
 */
 
 int Client_DSSE::saveState()
 {
     
    //write keys to file
    printf("   Writing key...");
    
    string key_loc = gcsDataStructureFilepath + "key1";
    Miscellaneous::write_file_cpp(key_loc,this->masterKey->key1,BLOCK_CIPHER_SIZE);
    
    key_loc = gcsDataStructureFilepath + "key2";
    Miscellaneous::write_file_cpp(key_loc,this->masterKey->key2,BLOCK_CIPHER_SIZE);
    
    key_loc = gcsDataStructureFilepath + "key3";
    Miscellaneous::write_file_cpp(key_loc,this->masterKey->key3,BLOCK_CIPHER_SIZE);
    
    printf("OK!\n");
    
    printf("   Writing hash tables...");
    
    Miscellaneous::writeHash_table(T_W,gcsKwHashTable,gcsDataStructureFilepath);
    Miscellaneous::writeHash_table(T_F,gcsFileHashTable,gcsDataStructureFilepath);
        
    Miscellaneous::write_list_to_file(gcsListFreeFileIdx,gcsDataStructureFilepath,lstFree_column_idx);
    Miscellaneous::write_list_to_file(gcsListFreeKwIdx,gcsDataStructureFilepath,lstFree_row_idx);
        
    printf("OK!\n");
        
    printf("   Writing column counter...");
    Miscellaneous::write_array_to_file(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
    printf("OK!\n");
        
    
    printf("   Writing row counter...");
    Miscellaneous::write_array_to_file(FILENAME_KEYWORD_COUNTER_ARRAY,gcsDataStructureFilepath,this->keyword_counter_arr,MATRIX_ROW_SIZE);
    printf("OK!\n");
        
        
    printf("   Writing total keywords and files...");
    TYPE_COUNTER total_keywords_files[2] = {this->T_W.load_factor()*T_W.bucket_count(), this->T_F.load_factor()*T_F.bucket_count()} ;
    Miscellaneous::write_array_to_file(FILENAME_TOTAL_KEYWORDS_FILES,gcsDataStructureFilepath,total_keywords_files,2);
    printf("OK!\n");
    
}
 
/**
 * Function Name: loadState
 *
 * Description:
 * Loat previous state into memory
 *
 * @return	0 if successful
 */
int Client_DSSE::loadState()
{
    // Load keys
    printf("   Loading master key...");
    this->masterKey = new MasterKey();

    string key_loc = gcsDataStructureFilepath + "key1";
    Miscellaneous::read_file_cpp(this->masterKey->key1,BLOCK_CIPHER_SIZE,key_loc);
    
    key_loc = gcsDataStructureFilepath + "key2";
    Miscellaneous::read_file_cpp(this->masterKey->key2,BLOCK_CIPHER_SIZE,key_loc);
    
    key_loc = gcsDataStructureFilepath + "key3" ;
    Miscellaneous::read_file_cpp(this->masterKey->key3,BLOCK_CIPHER_SIZE,key_loc);
    
    //Load client state
    unsigned char empty_label[6] = "EMPTY";
    unsigned char delete_label[7] = "DELETE";
      hashmap_key_class empty_key = hashmap_key_class(empty_label,6);
    hashmap_key_class delete_key = hashmap_key_class(delete_label,7);
    printf("OK!\n");
    printf("    Loading hash tables...");
    T_W = TYPE_GOOGLE_DENSE_HASH_MAP(MAX_NUM_KEYWORDS*KEYWORD_LOADING_FACTOR);
    T_W.max_load_factor(KEYWORD_LOADING_FACTOR);
    T_W.min_load_factor(0.0);
    T_W.set_empty_key(empty_key);
    T_W.set_deleted_key(delete_key);

    T_F = TYPE_GOOGLE_DENSE_HASH_MAP(MAX_NUM_OF_FILES*KEYWORD_LOADING_FACTOR);
    T_F.max_load_factor(FILE_LOADING_FACTOR);
    T_F.min_load_factor(0.0);
    T_F.set_empty_key(empty_key);
    T_F.set_deleted_key(delete_key);
        
        
        
    T_W.clear();
    T_F.clear();


    TYPE_COUNTER total_keywords_files[2];
    Miscellaneous::read_array_from_file(FILENAME_TOTAL_KEYWORDS_FILES,gcsDataStructureFilepath,total_keywords_files,2);
        
    Miscellaneous::readHash_table(T_W,gcsKwHashTable,gcsDataStructureFilepath,total_keywords_files[0]);
    Miscellaneous::readHash_table(T_F,gcsFileHashTable,gcsDataStructureFilepath,total_keywords_files[1]);
    
    lstFree_column_idx.reserve(MAX_NUM_OF_FILES);
    lstFree_column_idx.clear();
    lstFree_row_idx.reserve(MAX_NUM_KEYWORDS);
    lstFree_row_idx.clear();

    Miscellaneous::read_list_from_file(gcsListFreeFileIdx,gcsDataStructureFilepath,lstFree_column_idx);
    Miscellaneous::read_list_from_file(gcsListFreeKwIdx,gcsDataStructureFilepath,lstFree_row_idx);
    
    printf("OK!\n");
    
    
    
    printf("   Loading column and row counters...");
    Miscellaneous::read_array_from_file(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
    
    Miscellaneous::read_array_from_file(FILENAME_KEYWORD_COUNTER_ARRAY,gcsDataStructureFilepath,this->keyword_counter_arr,MATRIX_ROW_SIZE);
    printf("OK!\n");
            
        
    printf("\nFinished!\n");
    printf("Size of keyword hash table: \t\t\t %zu \n",T_W.bucket_count());
    printf("Load factor of keyword hash table: \t\t %3.10f \n",T_W.load_factor());
    printf("# kw in hash table: \t\t\t\t %5.0f \n",T_W.load_factor()*T_W.bucket_count());
    
    printf("Size of file hash table: \t\t\t %zu \n",T_F.bucket_count());
    printf("Load factor of file hash table: \t\t %3.10f \n",T_F.load_factor());
    printf("# files in hash table: \t\t\t\t %5.0f \n",T_F.load_factor()*T_F.bucket_count());
        
    cout<<"Size of list free column idx: "<<lstFree_column_idx.size()<<endl;
    cout<<"Size of list free row idx: "<<lstFree_row_idx.size()<<endl;
    
#if defined (DECRYPT_AT_CLIENT_SIDE)
    delete this->row_keys;
    this->row_keys = new unsigned char[BLOCK_CIPHER_SIZE*MATRIX_ROW_SIZE];
    memset(this->row_keys,0,BLOCK_CIPHER_SIZE*MATRIX_ROW_SIZE);
    DSSE_KeyGen dsse_keygen;
    dsse_keygen.pregenerateRow_keys(this->keyword_counter_arr,row_keys,this->masterKey);
    
#endif
    ready = true;
}


/**
 * Function Name: sendFile
 *
 * Description:
 * send a (physical) file to the server *
 *
 * @param filename: (input) name of sending file
 * @param number: (input) location of sending file
 * @param SENDING_TYPE: (input) type of files (e.g., data structure, file collection, etc.)
 * @return	0 if successful
 */
int Client_DSSE::sendFile(string filename, string path, int SENDING_TYPE)
{

    int n; 
    
    FILE* finput = NULL;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE];
	unsigned char buffer_out[SOCKET_BUFFER_SIZE];
	
    off_t filesize, offset;
    off_t size_sent = 0;
    string filename_with_path = path + filename;
    
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    
    try
    {
        printf("   Opening file...");
        if( ( finput = fopen(filename_with_path.c_str(), "rb" ) ) == NULL )
        {
            printf( "Error! File not found \n" );
            exit(1);
        }
        if( ( filesize = lseek( fileno( finput ), 0, SEEK_END ) ) < 0 )
        {
            perror( "lseek" );
            exit(1);
        }
        if( fseek( finput, 0, SEEK_SET ) < 0 )
        {
            printf("fseek(0,SEEK_SET) failed\n" );
            exit(1);
        }
        printf("OK!\n");
        
        printf("   Connecting to server...");
        socket.connect (PEER_ADDRESS);
        printf("OK!\n");
        
        printf("   Sending file sending command...");
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&SENDING_TYPE,sizeof(SENDING_TYPE));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK!\n");
        
        printf("   Sending file name...");
        socket.send((unsigned char*) filename.c_str(),strlen(filename.c_str()));
        printf("OK!\n");
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        
        printf("   Sending file data...");
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&filesize,sizeof(size_t));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        
        // 6.2 Read file block by block and write to the destination
        size_sent = 0;
        for( offset = 0; offset < filesize; offset += SOCKET_BUFFER_SIZE )
        {
            n = ( filesize - offset > SOCKET_BUFFER_SIZE ) ? SOCKET_BUFFER_SIZE : (int)
                ( filesize - offset );
            if( fread( buffer_in, 1, n, finput ) != (size_t) n )
            {
                printf( "read input file error at block %d",n);
                break;
            }
            if(offset + n ==filesize)
                socket.send(buffer_in,n,0);
            else
                socket.send(buffer_in,n,ZMQ_SNDMORE);
            size_sent += n;
            if(size_sent % 10485760 == 0)
                printf("%jd / %jd sent \n",size_sent,filesize);
        }
        fclose(finput);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK!\t\t\t %jd bytes sent\n",size_sent);
    }
    catch (exception &ex)
    {
        printf("Error!\n");
        exit(1);
    }

    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
	memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    socket.disconnect(PEER_ADDRESS);
    socket.close();
    
    return 0;
    
}


/**
 * Function Name: createEncrypted_data_structure
 *
 * Description:
 * Create the DSSE encrypted data structure
 *
 * @return	0 if successful
 */
int Client_DSSE::createEncrypted_data_structure()
{
    DSSE* dsse = new DSSE();
    vector<string> files_input;


    Miscellaneous misc;

#if defined (DECRYPT_AT_CLIENT_SIDE)
    DSSE_KeyGen* dsse_keygen  = new DSSE_KeyGen();
#endif
    try
    {
        
        
        printf("1. Generate master key...");
        this->genMaster_key();
        printf("OK!\n");
        
        files_input.reserve(MAX_NUM_OF_FILES);
        printf("2. Setting up data structure......\n");
        if(dsse->setupData_structure( this->T_W,this->T_F,
                                    this->keyword_counter_arr, this->block_counter_arr, this->block_state_mat,
                                    this->lstFree_column_idx,this->lstFree_row_idx,
                                    files_input,gcsFilepath,
                                    this->masterKey)!=0)
        {
            printf("Error!\n");
            exit(1);
        }
        printf("\nFinished!\n");
        printf("Size of keyword hash table: \t\t\t %zu \n",this->T_W.bucket_count());
        printf("Load factor of keyword hash table: \t\t %3.10f \n",this->T_W.load_factor());
        printf("# keywords extracted: \t\t\t\t %5.0f \n",this->T_W.load_factor()*T_W.bucket_count());
        printf("Size of file hash table: \t\t\t %zu \n",this->T_F.bucket_count());
        printf("Load factor of file hash table: \t\t %3.10f \n",this->T_F.load_factor());
        printf("# files extracted: \t\t\t %5.0f \n",this->T_F.load_factor()*T_F.bucket_count());
        
        cout<<"Size of list free column idx: "<<lstFree_column_idx.size()<<endl;
        cout<<"Size of list free row idx: "<<lstFree_row_idx.size()<<endl;
    
#if defined (DECRYPT_AT_CLIENT_SIDE)
    this->row_keys = new unsigned char[BLOCK_CIPHER_SIZE*MATRIX_ROW_SIZE];
    memset(this->row_keys,0,BLOCK_CIPHER_SIZE*MATRIX_ROW_SIZE);
    dsse_keygen->pregenerateRow_keys(this->keyword_counter_arr,row_keys,this->masterKey);
    
#endif
        
        printf("\n\n3. Saving state to disk...\n");
        saveState();
        
        printf("\n---ENCRYPTED INDEX CONSTRUCTION COMPLETED!---\n");
        this->sendEncryptedIndex();
        
    }
    catch (exception &ex)
    {
        printf("Error!!\n");
        exit(1);
    }
    ready = true;
    
#if defined (DECRYPT_AT_CLIENT_SIDE)
    delete dsse_keygen;
#endif
    files_input.clear();
    delete dsse;
    return 0;
}

/**
 * Function Name: sendCommandOnly
 *
 * Description: send a command-only to let the server load/save data into memory
 *
 * @return	0 if successful
 */
int Client_DSSE::sendCommandOnly(int cmd)
{
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    socket.connect(PEER_ADDRESS);
    socket.send(&cmd,sizeof(cmd),0);
    unsigned char buffer_in[SOCKET_BUFFER_SIZE];
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE,0);
    socket.disconnect(PEER_ADDRESS);
}

/**
 * Function Name: sendEncryptedIndex
 *
 * Description: Send the encrypted index to the server
 *
 * @return	0 if successful
 */
int Client_DSSE::sendEncryptedIndex()
{
    Miscellaneous misc;
    char choice = ' ';
    do
    {
        cout << "       UPLOAD ENCRYPTED INDEX TO SERVER? (y/n)  \n **** ONLY SAY 'y' IF CLIENT AND SERVER ARE DEPLOYED IN TWO DIFFERENT MACHINES!! *** \n";
        cin >> choice;
        choice = tolower(choice);
    }while( !cin.fail() && choice!='y' && choice!='n' );
    if(choice=='y')
    {
            
        printf("1. Sending Data Matrix I...");
        int n = MATRIX_COL_SIZE / MATRIX_PIECE_COL_SIZE;        
        for(int i = 0 ; i < n ; i++)
        {
            for(TYPE_INDEX m = 0 ; m < MATRIX_ROW_SIZE; m+=MATRIX_PIECE_ROW_SIZE)
            {
                string filename = misc.to_string(m) + "_" + misc.to_string(i*MATRIX_PIECE_COL_SIZE);
                this->sendFile(filename,gcsMatrixPiecePath,CMD_SEND_DATA_STRUCTURE);
            }
        }
        printf("OK!\n");
    #if !defined(DECRYPT_AT_CLIENT_SIDE) 
        printf("1. Sending State Matrix I.st...");
        n = NUM_BLOCKS/BYTE_SIZE/BLOCK_STATE_PIECE_COL_SIZE;
        for(int i = 0 ; i < n ; i++)
        {
            for(TYPE_INDEX m = 0 ; m < BLOCK_STATE_ROW_SIZE; m+=BLOCK_STATE_PIECE_ROW_SIZE)
            {
                string filename = "b_" + misc.to_string(m) + "_" + misc.to_string(i*BLOCK_STATE_PIECE_COL_SIZE);
                this->sendFile(filename,gcsMatrixPiecePath,CMD_SEND_DATA_STRUCTURE);
            }
        }
        printf("OK!\n");
        printf("3. Sending block counter...");
        this->sendFile(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,CMD_SEND_DATA_STRUCTURE);
        printf("OK!!\n");
    #endif
        
    }
    else
    {
       this->sendCommandOnly(CMD_LOADSTATE);
    }
    
    return 0;
}
/**
 * Function Name: searchKeyword
 *
 * Description:
 * search a keyword
 *
 * @param keyword: (input) keyword would like to search
 * @param res: (output) number of files that the searching keyword appear in
 * @return	0 if successful
 */
int Client_DSSE::searchKeyword(string keyword, TYPE_COUNTER &res)
{
    DSSE* dsse = new DSSE();
    SearchToken tau;
    auto start = time_now;
    auto end = time_now;

    int len;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
	unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    zmq::context_t context(1);
    zmq::socket_t socket (context,ZMQ_REQ);
    int cmd;
    vector<TYPE_INDEX> lstFile_id;
#if defined(SEND_SEARCH_FILE_INDEX)
    string filename_search_result = gcsDataStructureFilepath + FILENAME_SEARCH_RESULT;
    int64_t more;
    FILE* foutput = NULL;
    size_t file_in_size;
    Miscellaneous misc;
    
#endif
#if defined (DECRYPT_AT_CLIENT_SIDE)
    
    DSSE_KeyGen* dsse_keygen = new DSSE_KeyGen();
#endif
    try
    {
        if(ready == false)
        {
            printf("Encrypted index is not constructed/load yet, please build it first!\n");
            res = KEYWORD_NOT_EXIST;
            return 0;
        }
        printf("\n\n Searching \"%s\" ....\n\n", keyword.c_str());
        start = time_now;
        /*Generate the token for this keyword using SrchToken procedure*/        
        printf("1. Generating keyword token...");
        dsse->searchToken(  tau,
                            keyword,
                            this->T_W,
                            this->keyword_counter_arr,
                            this->masterKey);
        end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
        if(tau.row_index == KEYWORD_NOT_EXIST)
        {
            res = 0;
            return 0;
        }
        
#if !defined(DECRYPT_AT_CLIENT_SIDE)

        //increase counter to 1
        this->keyword_counter_arr[tau.row_index]+=1;
        
        
        start = time_now;
        printf("2. Sending keyword token and waiting for result...");
        
        socket.connect(PEER_ADDRESS);
        
        // Send search command
        cmd = CMD_SEARCH_OPERATION;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        
        // Send keyword token
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(&buffer_out,&tau,sizeof(SearchToken));
        socket.send(buffer_out,sizeof(SearchToken));
        
        // Receive the result 
      #if defined (SEND_SEARCH_FILE_INDEX)
        // open the file to receive first
        if((foutput = fopen(filename_search_result.c_str(),"wb+"))==NULL)
        {
            printf("Error!!\n");
            exit(1);
        }
        // Receive file size
        memset(buffer_in,0,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE,ZMQ_RCVMORE);
        memcpy(&file_in_size,buffer_in,sizeof(size_t));
        
        // receive file content
        unsigned char* file_in = new unsigned char[file_in_size];
        socket.recv(file_in,file_in_size);
        fwrite(file_in,1,file_in_size,foutput);
        fclose(foutput);
        // Load result from file;
        lstFile_id.clear();
        misc.read_list_from_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
        if(lstFile_id.size() == 1 && lstFile_id[0]==ULONG_MAX)
        {
            lstFile_id.clear();
            res = 0;
        }
        else
            res = lstFile_id.size();
      #else
        memset(buffer_in,0,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        memcpy(&res,buffer_in,sizeof(res));
      #endif
        end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
       
#else 

        MatrixType *search_data = new MatrixType[MATRIX_COL_SIZE];
        memset(search_data,0,MATRIX_COL_SIZE);
        unsigned char* aes_keys = new unsigned char[MATRIX_COL_SIZE];
        memset(aes_keys,0,MATRIX_COL_SIZE);

    
        THREAD_PRECOMPUTE_AESKEY aes_key_decrypt_param(aes_keys,tau.row_index,ROW,false,this->block_counter_arr,this->row_keys,this->masterKey);
        pthread_create(&thread_precomputeAesKey_decrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_decrypt_param);
        
        THREAD_GETDATA get_data_param(tau.row_index,search_data);
        pthread_create(&thread_getData,NULL,&Client_DSSE::thread_getSearchData_func,(void*)&get_data_param);
        
        pthread_join(thread_precomputeAesKey_decrypt, NULL);
        pthread_join(thread_getData, NULL);
      
        MatrixType *search_res = new MatrixType[MATRIX_COL_SIZE];
        memset(search_res,0,MATRIX_COL_SIZE);
        
        printf("2. Decrypting...");
        start = time_now;
        dsse_keygen->enc_dec_preAESKey(search_res,search_data,aes_keys,MATRIX_COL_SIZE);
        end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
        
        printf("3. Getting search result...");
        start = time_now;
        for(TYPE_INDEX ii=0; ii<MATRIX_COL_SIZE; ii++)
        {
                for(int bit_number = 0 ; bit_number<BYTE_SIZE; bit_number++)
                    if(BIT_CHECK(&search_res[ii].byte_data,bit_number))
                        lstFile_id.push_back(ii*BYTE_SIZE+bit_number);
        }
        res = lstFile_id.size();
        end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
       
        Miscellaneous::write_list_to_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
#endif
    
    }
    catch (exception &ex)
    {
        printf("Error!!\n");
        exit(1);
    }
    memset(&tau,0,sizeof(SearchToken));
    delete dsse;
#if !defined(DECRYPT_AT_CLIENT_SIDE)
    socket.disconnect(PEER_ADDRESS);
    socket.close();
#if defined(SEND_SEARCH_FILE_INDEX)
    filename_search_result.clear();
#endif
#endif

#if defined (DECRYPT_AT_CLIENT_SIDE)
    delete dsse_keygen;
#endif

    return 0;
 }

int Client_DSSE::requestBlock_data(TYPE_INDEX block_index, MatrixType* I_prime, bool* block_state_arr ) 
{
    int cmd;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    
#if !defined(DECRYPT_AT_CLIENT_SIDE)
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE+(MATRIX_ROW_SIZE/BYTE_SIZE);
    TYPE_INDEX row,ii,state_col,state_bit_position;
#else
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE;
#endif    
    
    MatrixType* serialized_buffer = new MatrixType[serialized_buffer_len]; //consist of data and block state
    

    try
    {   
        memset(serialized_buffer,0,serialized_buffer_len);
        
        // Connect to the server
        socket.connect(PEER_ADDRESS);
        
        // Send update data request command
        cmd = CMD_REQUEST_BLOCK_DATA;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        
        // Send RequestIndex token 
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&block_index,sizeof(block_index));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        
        // Receive data 
        socket.recv(serialized_buffer,serialized_buffer_len,0);
        
        // Deserialize 
        memcpy(I_prime,serialized_buffer,(MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE);
#if !defined (DECRYPT_AT_CLIENT_SIDE)
        for(row = 0,ii=(MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE) ; row < MATRIX_ROW_SIZE; ii++,row++)
        {
            state_col = ii / BYTE_SIZE;
            state_bit_position = ii % BYTE_SIZE;
            if(BIT_CHECK(&serialized_buffer[state_col].byte_data,state_bit_position))
                block_state_arr[row] = ONE_VALUE;
            else
                block_state_arr[row] = ZERO_VALUE;
        }
#endif
    }
    catch(exception &ex)
    {
        printf("Error!!\n");
        exit(1);
    }
    
    delete[] serialized_buffer;
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    socket.disconnect(PEER_ADDRESS);
    socket.close();
    return 0;
}



int Client_DSSE::sendBlock_data(TYPE_INDEX block_index, MatrixType *I_prime)
 {
    int cmd;
    Miscellaneous misc;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    string filename_temp_with_path;
    int n; 
    off_t offset;

    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE;
    try
    {
        socket.connect(PEER_ADDRESS);
        
        // Send update block command...");
        cmd = CMD_UPDATE_BLOCK_DATA;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        
        // Send Block index
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&block_index,sizeof(block_index));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        
        // Send block data 
        socket.send((unsigned char*)I_prime,serialized_buffer_len);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);       
    }
    catch (exception &ex)
    {
        printf("Error!!\n");
        exit(1);
    }


    socket.disconnect(PEER_ADDRESS);
    socket.close();
    memset(buffer_in,0,sizeof(buffer_in));
    memset(buffer_out,0,sizeof(buffer_out));
    filename_temp_with_path.clear();
    return 0;
}


/**
 * Function Name: addFile
 *
 * Description:
 * add a new file to the database *
 * Client request the server to send the l'th block index of this adding file first
 * - i.   Client creates the l'th block index of the file according to its trapdoor
 * - ii.  Clients send the l index to the server to receive the corresponding l'th block data (stored in I_prime)
 * - iii. Client performs "addToken" proc to generate the new block data
 * - iv.  Clients sends this new block data to the server to update
 * - v.   Server call "add" proc to add new block data
 *
 * @param filename: (input) name of adding file
 * @param number: (input) location of adding file
 * @return	0 if successful
 */
int Client_DSSE::addFile(string filename, string path)
{
    Miscellaneous misc;
    DSSE* dsse = new DSSE();
    TYPE_INDEX block_index;

    TYPE_KEYWORD_DICTIONARY extracted_keywords;
    string adding_filename_with_path = path + filename;
    TYPE_INDEX file_index;  
    
    stringstream new_filename_with_path;
    string s;
    
    
    
#if defined(DECRYPT_AT_CLIENT_SIDE)
    DSSE_KeyGen* dsse_keygen = new DSSE_KeyGen();
   
#endif

auto start = time_now;
auto end = time_now;

    try
    {
        if(ready == false)
        {
            printf("Encrypted index is not constructe yet, please build it first!\n");
            return 0;
        }
        printf("1. Determining block index...");
        start = time_now;
        
        dsse->requestBlock_index(adding_filename_with_path,block_index,this->T_F,this->lstFree_column_idx,this->masterKey);
        end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;


#if defined (DECRYPT_AT_CLIENT_SIDE)
     
        THREAD_PRECOMPUTE_AESKEY aes_key_decrypt_param(decrypt_key,block_index,COL,false, this->block_counter_arr,this->row_keys,this->masterKey);
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            pthread_create(&thread_precomputeAesKey_decrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_decrypt_param);
        }        
        
        THREAD_PRECOMPUTE_AESKEY aes_key_reencrypt_param(reencrypt_key,block_index,COL,true, this->block_counter_arr,this->row_keys,this->masterKey);
        pthread_create(&thread_precomputeAesKey_reencrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_reencrypt_param);
     
#endif
    
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            printf("2. Getting block (& state) data from server...");
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
            start = time_now;
            this->requestBlock_data(block_index, this->I_prime,this->block_state_arr);
            
            end = time_now;
            cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

    #else
        THREAD_GETDATA get_data_param(block_index,this->I_prime);
        pthread_create(&thread_getData,NULL,&Client_DSSE::thread_getUpdateData_func,(void*)&get_data_param);
        
    #endif
        }
    #if defined(DECRYPT_AT_CLIENT_SIDE) 
        
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            pthread_join(thread_precomputeAesKey_decrypt, NULL);
            pthread_join(thread_getData, NULL);
        }
        pthread_join(thread_precomputeAesKey_reencrypt, NULL);
        
   
    #endif

        printf("3. Peforming AddToken...");
start = time_now;
#if !defined(DECRYPT_AT_CLIENT_SIDE)
        extracted_keywords.clear();
        dsse->addToken( adding_filename_with_path,           
                        this->I_prime,
                        file_index,
                        this->T_F,this->T_W, extracted_keywords,
                        this->keyword_counter_arr,this->block_counter_arr,this->block_state_arr,
                        this->lstFree_column_idx,this->lstFree_row_idx,
                        this->masterKey);
#else
        dsse->addToken(adding_filename_with_path,this->I_prime,
                        file_index,this->T_F,this->T_W,extracted_keywords,decrypt_key,reencrypt_key,
                        this->lstFree_column_idx,this->lstFree_row_idx,this->masterKey);        
#endif
        //increase the counter
        this->block_counter_arr[block_index] += 1;
        
       
end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

        keywords_dictionary.insert(extracted_keywords.begin(),extracted_keywords.end());
        

        printf("5. Send updated column/block to server...");
        start = time_now;
        this->sendBlock_data(block_index,this->I_prime);
        end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
      
  
    }
    catch(exception &ex)
    {
        printf("Error!!\n");
        exit(1);
    }    
    
    extracted_keywords.clear();

    delete dsse;


#if defined (DECRYPT_AT_CLIENT_SIDE)
    delete dsse_keygen;
#endif
    return 0;
}

/**
 * Function Name: delFile
 *
 * Description:
 * add a new file to the database *
 * Client request the server to send the l'th block index of this deleting file first
 * - i.   Client creates the l'th block index of the file according to its trapdoor
 * - ii.  Clients send the l index to the server to receive the corresponding l'th block data (stored in I_prime)
 * - iii. Client performs "delToken" proc to generate the new block data
 * - iv.  Clients sends this new block data to the server to update
 * - v.   Server call "del" proc to add new block data
 * @param filename: (input) name of deleting file
 * @param path: (input) location of deleting file
 * @return	0 if successful
 */
int Client_DSSE::delFile(string filename, string path)
{
    Miscellaneous misc;
    DSSE* dsse = new DSSE();

    TYPE_INDEX block_index;
    TYPE_INDEX file_index;        
    
    string deleting_filename_with_path = path + filename;
    stringstream new_filename_with_path;
    string s;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] ;
    unsigned char buffer_out[SOCKET_BUFFER_SIZE];
    string encrypted_filename = "";
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);

#if defined(DECRYPT_AT_CLIENT_SIDE)
    DSSE_KeyGen* dsse_keygen = new DSSE_KeyGen();
#endif
auto start = time_now;
auto end = time_now;
    try
    {
         if(ready == false)
        {
            printf("Encrypted index is not constructe yet, please build it first!\n");
            return 0;
        }
        
        printf("1. Determining the block index...");
        start = time_now;
        dsse->requestBlock_index(deleting_filename_with_path,block_index,this->T_F,this->lstFree_column_idx,this->masterKey);
        
        end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    
#if defined (DECRYPT_AT_CLIENT_SIDE)
        //precompute AES keys
        THREAD_PRECOMPUTE_AESKEY aes_key_decrypt_param(decrypt_key,block_index,COL,false, this->block_counter_arr,this->row_keys,this->masterKey);
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            pthread_create(&thread_precomputeAesKey_decrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_decrypt_param);
        }

        THREAD_PRECOMPUTE_AESKEY aes_key_reencrypt_param(reencrypt_key,block_index,COL, true, this->block_counter_arr,this->row_keys,this->masterKey);
        pthread_create(&thread_precomputeAesKey_reencrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_reencrypt_param);
     
    
#endif

        if(ENCRYPT_BLOCK_SIZE>1)
        {
            printf("2. Getting block (& state) data from server...");
     
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
            start = time_now;
            this->requestBlock_data(block_index, this->I_prime, this->block_state_arr);
            end = time_now;
            cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    #else
            THREAD_GETDATA get_data_param(block_index,this->I_prime);
            pthread_create(&thread_getData,NULL,&Client_DSSE::thread_getUpdateData_func,(void*)&get_data_param);
    #endif
        }
    #if defined(DECRYPT_AT_CLIENT_SIDE) 
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            pthread_join(thread_precomputeAesKey_decrypt, NULL);
            pthread_join( thread_getData, NULL);
            
        }
        pthread_join(thread_precomputeAesKey_reencrypt, NULL);
        
    #endif
    
   
        printf("3. Peforming DelToken...");
        start = time_now;
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
        dsse->delToken( deleting_filename_with_path,                   
                        this->I_prime,                    
                        file_index,
                        this->T_F,this->T_W,
                        this->keyword_counter_arr,this->block_counter_arr,this->block_state_arr,
                        this->lstFree_column_idx,this->lstFree_row_idx,
                        this->masterKey);
    #else
        dsse->delToken(deleting_filename_with_path,
                        this->I_prime,file_index,this->T_F,this->T_W,
                        decrypt_key,reencrypt_key,
                        this->lstFree_column_idx, this->lstFree_row_idx,
                        this->masterKey);
    #endif
         //increase the counter
         cout<<"block index: "<<endl;
        this->block_counter_arr[block_index] += 1;
   
end = time_now;
cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    
    
        printf("5. Send updated data to server...");
        start = time_now;
        this->sendBlock_data(block_index,this->I_prime);
        end = time_now;
        cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    }
    catch (exception &ex)
    {
        printf("Error!!\n");
        exit(1);
    }

    memset(buffer_in,0,SOCKET_BUFFER_SIZE) ;
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    encrypted_filename.clear();


#if defined(DECRYPT_AT_CLIENT_SIDE)
    delete dsse_keygen;
#endif
    delete dsse;
    return 0;
}

int Client_DSSE::requestSearch_data(TYPE_INDEX row_index, MatrixType* I_prime) 
{
    int cmd;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    
    TYPE_INDEX serialized_buffer_len = MATRIX_COL_SIZE;
    
    TYPE_INDEX row,ii,state_col,state_bit_position;
    try
    {   
        
        // Connect to server
        socket.connect(PEER_ADDRESS);
        
        // Send search command
        cmd = CMD_REQUEST_SEARCH_DATA;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        
        // Send search request index
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&row_index,sizeof(row_index));

        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        
        // Receive search data
        socket.recv((unsigned char*)I_prime,serialized_buffer_len);
 
    }
    catch(exception &ex)
    {
        printf("Error!\n");
        exit(1);
    }
    
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    socket.disconnect(PEER_ADDRESS);
    socket.close();
    return 0;
}




void *Client_DSSE::thread_precomputeAesKey_func(void* param)
{
    printf("\n   [Thread] Generating AES-CTR decryption key...");
    auto start = time_now;
    THREAD_PRECOMPUTE_AESKEY* opt = (THREAD_PRECOMPUTE_AESKEY*) param;
    DSSE_KeyGen* dsse_keygen = new DSSE_KeyGen();
    dsse_keygen->precomputeAES_CTR_keys(opt->aes_keys,opt->idx,opt->dim, opt->isIncremental, opt->block_counter_arr,opt->row_keys,opt->masterKey);
    delete dsse_keygen;
    auto end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    pthread_exit((void*)opt);
}
void *Client_DSSE::thread_getSearchData_func(void* param)
{
    printf("\n   [Thread] Getting search data from server...");
    auto start = time_now;
    THREAD_GETDATA* opt = (THREAD_GETDATA*) param;
    Client_DSSE* call = new Client_DSSE();
    call->requestSearch_data(opt->idx, opt->data);
    auto end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    
    delete call;
    pthread_exit((void*)opt);
}
void *Client_DSSE::thread_getUpdateData_func(void* param)
{
    printf("\n   [Thread] Getting update data from server...");
    auto start = time_now;
    THREAD_GETDATA* opt = (THREAD_GETDATA*) param;
    Client_DSSE* call = new Client_DSSE();
    call->requestBlock_data(opt->idx,opt->data,NULL);
    auto end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    
    delete call;
    pthread_exit((void*)opt);
}

