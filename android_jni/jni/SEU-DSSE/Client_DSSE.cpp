#include "Client_DSSE.h"
#include "DSSE_Param.h"
#include "DSSE_KeyGen.h"
#include "string.h"
#include <sstream>	
#include "Miscellaneous.h"
#include <tomcrypt.h>
#include "DSSE.h"
#include "math.h"
#if defined (CLIENT_SERVER_MODE)
#include "zmq.hpp"
using namespace zmq;
#endif

#if defined(MULTI_THREAD)
#include "struct_thread_precompute_aeskey.h"
#include "struct_thread_getData.h"
#endif

vector<unsigned long long int> Client_DSSE::stats[16];

Client_DSSE::Client_DSSE()
{
    
    
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
    
    for(int i = 0 ; i <BLOCK_CIPHER_SIZE;i++)
    {
        this->extractor_salt[i] = 0;
        this->pseudo_random_key[i] = 0;
    }
    data_structure_constructed = false;
    DSSE_KeyGen *dsse_key = new DSSE_KeyGen();
    this->masterKey = new MasterKey();
    int ret;
    if((ret = dsse_key->genMaster_key(this->masterKey,this->pseudo_random_key,BLOCK_CIPHER_SIZE,this->extractor_salt,BLOCK_CIPHER_SIZE,this->pseudo_random_key,BLOCK_CIPHER_SIZE))!=0)
    {
        printf("Key generation error!");
        ret = KEY_GENERATION_ERR;
        delete this->masterKey;
        goto exit;
    }
    ret = 0;
    
exit:
    data_structure_constructed = false;
    delete dsse_key;
    return ret;
}
#if defined(CLIENT_SERVER_MODE)

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
    int ret;
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
            ret = FILE_OPEN_ERR;
            goto exit;
        }
        if( ( filesize = lseek( fileno( finput ), 0, SEEK_END ) ) < 0 )
        {
            perror( "lseek" );
            ret = FILE_OPEN_ERR;
            goto exit;
        }
        if( fseek( finput, 0, SEEK_SET ) < 0 )
        {
            printf("fseek(0,SEEK_SET) failed\n" );
            ret = FILE_OPEN_ERR;
            goto exit;
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
        ret = CLIENT_SEND_FILE_ERR;
        goto exit;
    }
    ret = 0;
exit:

    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
	memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    socket.disconnect(PEER_ADDRESS);
    socket.close();
    
    return ret;
    
}
#endif

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
    int ret;
    vector<string> files_input;
#if defined(CLIENT_SERVER_MODE)

#if defined(ENCRYPT_PHYSICAL_FILE)
    vector<string> sending_files;
    TYPE_INDEX i;
#endif
    Miscellaneous misc;
#endif

#if defined (DECRYPT_AT_CLIENT_SIDE)
    DSSE_KeyGen* dsse_keygen  = new DSSE_KeyGen();
#endif
    try
    {
        

#if !defined(CLIENT_SERVER_MODE)
    printf("0. Allocating memory for data structure......");
#if !defined(LOAD_FROM_DISK)
        I = new MatrixType*[MATRIX_ROW_SIZE];
        for(TYPE_INDEX row = 0 ; row <MATRIX_ROW_SIZE; row++)
        {
            I[row] = new MatrixType[MATRIX_COL_SIZE];
            memset(I[row],0,MATRIX_COL_SIZE);
        }
    
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
        this->block_state_mat = new MatrixType*[MATRIX_ROW_SIZE];
        for (TYPE_INDEX row = 0 ;  row < MATRIX_ROW_SIZE; row ++)
        {
            this->block_state_mat[row] = new MatrixType[NUM_BLOCKS/BYTE_SIZE];
            memset(this->block_state_mat[row],ZERO_VALUE,NUM_BLOCKS/BYTE_SIZE);
        }
    #endif
#else
    this->I_search = new MatrixType*[1];
    this->I_search[0] = new MatrixType[MATRIX_COL_SIZE];
    this->I_update = new MatrixType*[MATRIX_ROW_SIZE];
    TYPE_INDEX c = ceil((double)(ENCRYPT_BLOCK_SIZE)/(BYTE_SIZE));
    for(TYPE_INDEX i = 0 ; i < MATRIX_ROW_SIZE;i++)
    {
        this->I_update[i] = new MatrixType[c];
        memset(this->I_update[i],0,c);
    }
  #if !defined(DECRYPT_AT_CLIENT_SIDE)
    this->block_state_mat_search = new MatrixType*[1];
    this->block_state_mat_search[0] = new MatrixType[NUM_BLOCKS];
    memset(this->block_state_mat_search[0],0,NUM_BLOCKS);
            
    this->block_state_mat_update = new MatrixType*[BLOCK_STATE_ROW_SIZE];
    for(TYPE_INDEX i = 0 ; i < BLOCK_STATE_ROW_SIZE;i++)
    {
        this->block_state_mat_update[i] = new MatrixType[1];
        memset(this->block_state_mat_update[i],0,1);
    }
    
  #endif    
#endif

#endif
        /* Allocate memory for I' & block state array */
        I_prime = new MatrixType[MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE];
        memset(I_prime,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
        printf("OK!\n");
#if !defined(DECRYPT_AT_CLIENT_SIDE)
        block_state_arr = new bool[MATRIX_ROW_SIZE];
        memset(block_state_arr,0,MATRIX_ROW_SIZE);
#endif
        files_input.reserve(MAX_NUM_OF_FILES);
        printf("1. Setting up data structure......\n");
        if((ret = dsse->setupData_structure(  this->I,this->T_W,this->T_F,
                                    this->keyword_counter_arr, this->block_counter_arr, this->block_state_mat,
                                    files_input,gcsFilepath,gcsEncFilepath,
                                    this->masterKey))!=0)
        {
            goto exit;
        }
        printf("\nFinished!\n");
        printf("Size of keyword hash table: \t\t\t %zu \n",this->T_W.bucket_count());
        printf("Load factor of keyword hash table: \t\t %3.10f \n",this->T_W.load_factor());
        printf("# keywords extracted: \t\t\t\t %5.0f \n",this->T_W.load_factor()*T_W.bucket_count());
        printf("Size of file hash table: \t\t\t %zu \n",this->T_F.bucket_count());
        printf("Load factor of file hash table: \t\t %3.10f \n",this->T_F.load_factor());
        printf("# files extracted: \t\t\t\t %5.0f \n",this->T_F.load_factor()*T_F.bucket_count());
#if defined (DECRYPT_AT_CLIENT_SIDE)
    this->row_keys = new unsigned char[BLOCK_CIPHER_SIZE*MATRIX_ROW_SIZE];
    memset(this->row_keys,0,BLOCK_CIPHER_SIZE*MATRIX_ROW_SIZE);
    dsse_keygen->pregenerateRow_keys(this->keyword_counter_arr,row_keys,this->masterKey);
    
	decrypt_key = new unsigned char[MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE];
        reencrypt_key = new unsigned char[MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE];
        memset(decrypt_key,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
        memset(reencrypt_key,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);

#endif
#if defined(CLIENT_SERVER_MODE)

        /* Write block_counter_arr to the file */
        printf("2. Writing data structure to files...");
        misc.write_array_to_file(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
        printf("OK!\n");


        printf("3. Sending data structure to server...\n");

#if !defined(UPLOAD_DATA_STRUCTURE_MANUALLY_MODE)
        printf("   3.1. Matrix I...\n");
        int n = MATRIX_COL_SIZE / MATRIX_PIECE_COL_SIZE;        
        for(int i = 0 ; i < n ; i++)
        {
            for(TYPE_INDEX m = 0 ; m < MATRIX_ROW_SIZE; m+=MATRIX_PIECE_ROW_SIZE)
            {
                string filename = misc.to_string(m) + "_" + misc.to_string(i*MATRIX_PIECE_COL_SIZE);
                this->sendFile(filename,gcsMatrixPiecePath,CMD_SEND_DATA_STRUCTURE);
            }
        }    
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
    {
        n = NUM_BLOCKS/BYTE_SIZE/BLOCK_STATE_PIECE_COL_SIZE;
    
        for(int i = 0 ; i < n ; i++)
        {
            for(TYPE_INDEX m = 0 ; m < BLOCK_STATE_ROW_SIZE; m+=BLOCK_STATE_PIECE_ROW_SIZE)
            {
                string filename = "b_" + misc.to_string(m) + "_" + misc.to_string(i*BLOCK_STATE_PIECE_COL_SIZE);
                this->sendFile(filename,gcsMatrixPiecePath,CMD_SEND_DATA_STRUCTURE);
            }
        }
    }
    #endif
#else
        printf("Please copy generated data structure to server before continue...");
        cin.get();
#endif

        printf("   3.3. Block counter...\n");
        if((ret = this->sendFile(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,CMD_SEND_DATA_STRUCTURE))!=0)
        {
            goto exit;
        }
#if defined(ENCRYPT_PHYSICAL_FILE)
        printf("4. Sending encrypted files...\n");
        sending_files.clear();
        misc.extract_file_names(sending_files, gcsEncFilepath);	
        for( i = 0;  i < sending_files.size();i++ )
        {
            printf("%lu / %zu \n",i,sending_files.size());
            if((ret = this->sendFile(sending_files[i],gcsEncFilepath,CMD_ADD_FILE_PHYSICAL))!=0)
            {
                goto exit;
            }
        }
#endif

#endif
    }
    catch (exception &ex)
    {
        ret = CLIENT_CREATE_DATA_STRUCTURE_ERR;
        goto exit;
    }
    data_structure_constructed = true;
    ret =0;
exit:
#if defined(CLIENT_SERVER_MODE)
    #if defined (ENCRYPT_PHYSICAL_FILE)
        sending_files.clear();
    #endif
#endif    
#if defined (DECRYPT_AT_CLIENT_SIDE)
    delete dsse_keygen;
#endif
    files_input.clear();
    delete dsse;
    return ret;
}

/**
 * Function Name: searchKeyword
 *
 * Description:
 * search a keyword
 *
 * @param keyword: (input) keyword would like to search
 * @param number: (output) number of files that the searching keyword appear in
 * @return	0 if successful
 */
int Client_DSSE::searchKeyword(string keyword, TYPE_COUNTER &number)
{
    DSSE* dsse = new DSSE();
    int ret;
    SearchToken tau;
        auto start = time_now;
    auto end = time_now;
#if defined(CLIENT_SERVER_MODE)
    int len;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
	unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    zmq::context_t context(1);
    zmq::socket_t socket (context,ZMQ_REQ);
    int cmd;
    
    #if defined(SEND_SEARCH_FILE_INDEX)
    FILE* foutput;
    string filename_search_result = gcsDataStructureFilepath + FILENAME_SEARCH_RESULT;
    size_t file_in_size;
    size_t size_received;
    int64_t more;
    size_t more_size = sizeof(more);
    vector<TYPE_INDEX> lstFile_id;
    Miscellaneous misc;

    #endif
#else
    vector<TYPE_INDEX> lstFile_id;
#endif
#if defined (DECRYPT_AT_CLIENT_SIDE)
    vector<TYPE_INDEX> lstFile_id;
    
    DSSE_KeyGen* dsse_keygen = new DSSE_KeyGen();
#endif
    try
    {
        if(data_structure_constructed == false)
        {
//            printf("Data structure is not constructed yet, please build it first!\n");
            ret = DATA_STRUCTURE_NOT_BUILT_ERR;
            goto exit;
        }
//        printf("\n\n Searching \"%s\" ....\n\n", keyword.c_str());
        start = time_now;
        /*Generate the token for this keyword using SrchToken procedure*/        
//        printf("1. Generating keyword token...");
        if( (ret = dsse->searchToken(  tau,
                            keyword,
                            this->T_W,
                            this->keyword_counter_arr,
                            this->masterKey)) != 0 )
        {
//            printf("Error! SearchToken generation failed\n");
            goto exit;
        }
//        printf("OK!\n");
        end = time_now;
//        cout<<"Generate token: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//          cout<<"0: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
        stats[0].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
        if(tau.row_index == KEYWORD_NOT_EXIST)
        {
//            printf("Keyword not exist!\n");
            goto exit;
        }
        /* 2. Peform the search...*/
#if defined(CLIENT_SERVER_MODE)
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
        start = time_now;
//        printf("2. Sending keyword token to sever...\n");
        
        //network connection
//        printf("   2.1. Connecting to the server...");
        socket.connect(PEER_ADDRESS);
//        printf("connected!\n");
        
//        printf("   2.2. Sending search request...");
        cmd = CMD_SEARCH_OPERATION;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
//        printf("OK!\n");
        
//        printf("   2.3. Sending keyword token with row index: %lu...",tau.row_index);
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(&buffer_out,&tau,sizeof(SearchToken));
        socket.send(buffer_out,sizeof(SearchToken));
//        printf("OK! \n");
        
//        printf("   2.4. Receiving result...");;        
        
      #if defined (SEND_SEARCH_FILE_INDEX)
        
        // open the file to receive first
//        printf("\n     2.4.1 Opening temp file...");
        if((foutput = fopen(filename_search_result.c_str(),"wb+"))==NULL)
        {
//            printf("failed!\n");
            ret = FILE_OPEN_ERR;
            goto exit;
        }
//        printf("OK!\n");
//        printf("       2.4.2 Receiving specific file indices...");
        memset(buffer_in,0,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        memcpy(&file_in_size,buffer_in,sizeof(size_t),ZMQ_RCVMORE);
        // Send ACK
        //socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        
        // Receive the file content
        size_received = 0;
        memset(buffer_in,0,SOCKET_BUFFER_SIZE);
        while(size_received < file_in_size)
        {
            len = socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
            if(len < 0 )
            {
                if(len == REQUEST_TIMEOUT)
                    continue;
                else if (len == 0)
                    break;
                else
                    continue;
            }
            size_received +=len;
            if(size_received >= file_in_size)
            {
                fwrite(buffer_in,1,len-(size_received-file_in_size),foutput);
                break;
            }
            else
            {
                fwrite(buffer_in,1,len,foutput);
            }
            socket.getsockopt(ZMQ_RCVMORE,&more,&more_size);
            if(!more)
                break;
        }
        fclose(foutput);
//        printf("OK!\n");
         
        
//        printf("   2.4.  Loading results...");
        lstFile_id.clear();
        misc.read_list_from_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
        if(lstFile_id.size() == 1 && lstFile_id[0]==ULONG_MAX)
        {
            lstFile_id.clear();
            number = 0;
        }
        else
            number = lstFile_id.size();
//        printf("OK!\n");
        
      #else
            memset(buffer_in,0,SOCKET_BUFFER_SIZE);
            socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
            memcpy(&number,buffer_in,sizeof(number));
//            printf("OK!\n");
      #endif
        end = time_now;
//        cout<<"Search time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"12: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
        stats[1].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
         
//        printf("\n---------------------------\n");
       
   #else 

        MatrixType *search_data = new MatrixType[MATRIX_COL_SIZE];
        memset(search_data,0,MATRIX_COL_SIZE);
        unsigned char* aes_keys = new unsigned char[MATRIX_COL_SIZE];
        memset(aes_keys,0,MATRIX_COL_SIZE);
      #if !defined(MULTI_THREAD)
        start = time_now;
        //get Search data from server
        this->requestSearch_data(tau.row_index, search_data);
        end = time_now;
//        cout<<"Get data from server time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"13: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
         stats[2].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
       
        //precompute aes key while waiting (multithreadable)
        start =time_now;
        
        dsse_keygen->precomputeAES_CTR_keys(aes_keys,tau.row_index,ROW,this->block_counter_arr,this->row_keys,this->masterKey);
        end = time_now;
//        cout<<"Precompute aes key time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"14: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
          stats[3].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
      
      #else
        register_cipher(&aes_desc);
   
        THREAD_PRECOMPUTE_AESKEY aes_key_decrypt_param(aes_keys,tau.row_index,ROW,this->block_counter_arr,this->row_keys,this->masterKey);
        pthread_create(&thread_precomputeAesKey_decrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_decrypt_param);
        
        THREAD_GETDATA get_data_param(tau.row_index,search_data);
        pthread_create(&thread_getData,NULL,&Client_DSSE::thread_getSearchData_func,(void*)&get_data_param);
        
        pthread_join(thread_precomputeAesKey_decrypt, NULL);
        pthread_join(thread_getData, NULL);
        unregister_cipher(&aes_desc);
   
      #endif
      
        MatrixType *search_res = new MatrixType[MATRIX_COL_SIZE];
        memset(search_res,0,MATRIX_COL_SIZE);
        //decrypt to obtain result
        start = time_now;
        dsse_keygen->enc_dec_preAESKey(search_res,search_data,aes_keys,MATRIX_COL_SIZE);
        end = time_now;
//        cout<<"Decrypt time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"18: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
          stats[7].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());

        start = time_now;
        for(TYPE_INDEX ii=0; ii<MATRIX_COL_SIZE; ii++)
        {
                for(int bit_number = 0 ; bit_number<BYTE_SIZE; bit_number++)
                    if(BIT_CHECK(&search_res[ii].byte_data,bit_number))
                        lstFile_id.push_back(ii*BYTE_SIZE+bit_number);
        }
        number = lstFile_id.size();
        end = time_now;
//        cout<<"Get search result time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"19: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
         stats[8].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
       
        Miscellaneous::write_list_to_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
//        printf("\n---------------------------\n");
        
    #endif
    
#else
   
    #if defined (LOAD_FROM_DISK)
        loadData_from_file(ROW,tau.row_index);
    #endif
    #if defined(DECRYPT_AT_CLIENT_SIDE)
    
        MatrixType *search_data = new MatrixType[MATRIX_COL_SIZE];
        memset(search_data,0,MATRIX_COL_SIZE);
      #if !defined(LOAD_FROM_DISK)
        dsse->getBlock(tau.row_index,ROW,this->I,search_data);
      #else
        dsse->getBlock(0,ROW,this->I_search,search_data);
      #endif
        //precompute aes key
        unsigned char* aes_keys = new unsigned char[MATRIX_COL_SIZE];
        memset(aes_keys,0,MATRIX_COL_SIZE);
        dsse_keygen->precomputeAES_CTR_keys(aes_keys,tau.row_index,ROW,this->block_counter_arr,row_keys,this->masterKey);
       
        MatrixType *search_res = new MatrixType[MATRIX_COL_SIZE];
        memset(search_res,0,MATRIX_COL_SIZE);
       
        //decrypt to obtain result
        dsse_keygen->enc_dec_preAESKey(search_res,search_data,aes_keys,MATRIX_COL_SIZE);
        for(TYPE_INDEX ii=0; ii<MATRIX_COL_SIZE; ii++)
        {
                for(int bit_number = 0 ; bit_number<BYTE_SIZE; bit_number++)
                    if(BIT_CHECK(&search_res[ii].byte_data,bit_number))
                        lstFile_id.push_back(ii*BYTE_SIZE+bit_number);
        }

    #else
        
        printf("2. Performing local search...");
        lstFile_id.clear();
      #if defined(LOAD_FROM_DISK)
        TYPE_INDEX tmp = tau.row_index;
        tau.row_index = 0;
        if((ret=dsse->search(lstFile_id,tau,I_search,
                            this->block_counter_arr,
                            this->block_state_mat_search))!=0)
        {
            printf("Error during performing local search...\n");
            goto exit;
        }
      
      #else
        if((ret=dsse->search(lstFile_id,tau,this->I,
                            this->block_counter_arr,
                            this->block_state_mat))!=0)
        {
            printf("Error during performing local search...\n");
            goto exit;
        }
      #endif
        printf("OK!\n");
      #if defined(LOAD_FROM_DISK)
        printf("3. Saving new data to files...");
        saveData_to_file(ROW,tmp);
      #endif
        printf("OK!\n");
    
    #endif

        number = lstFile_id.size();
        Miscellaneous::write_list_to_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
//        printf("\n---------------------------\n");

#endif
    }
    catch (exception &ex)
    {
        ret = CLIENT_SEARCH_ERR;
        goto exit;
    }
    ret = 0 ;
    
exit:
    memset(&tau,0,sizeof(SearchToken));
    delete dsse;
#if defined(CLIENT_SERVER_MODE) && !defined(DECRYPT_AT_CLIENT_SIDE)
    socket.disconnect(PEER_ADDRESS);
    socket.close();
#if defined(SEND_SEARCH_FILE_INDEX)
    filename_search_result.clear();
#endif
#endif

#if defined (DECRYPT_AT_CLIENT_SIDE)
    delete dsse_keygen;
#endif

    return ret;
 }
#if defined(CLIENT_SERVER_MODE)

int Client_DSSE::requestBlock_data(TYPE_INDEX block_index, MatrixType* I_prime, bool* block_state_arr ) 
{
    int cmd;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    int len, ret;
    off_t offset;
    
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    int64_t more;
    size_t more_size = sizeof(more);
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
        
//        printf("   2.1. Connecting to the server... ");
        socket.connect(PEER_ADDRESS);
//        printf("OK!\n");
        
//        printf("   2.2. Sending request command...");
        cmd = CMD_REQUEST_BLOCK_DATA;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
//        printf("OK\n");
        
//        printf("   2.3. Sending RequestIndex token..."); 
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&block_index,sizeof(block_index));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
//        printf("OK\n");
       
//        printf("   2.4. Receiving data..."); 
        offset = 0;
        
        socket.recv(serialized_buffer,serialized_buffer_len,0);
        //socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        
        /* deserialize */
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
        ret = CLIENT_REQUEST_BLOCK_DATA_ERR;
        goto exit;
    }
    ret = 0;

exit:
    delete serialized_buffer;
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    socket.disconnect(PEER_ADDRESS);
    socket.close();
    return ret;
}
#endif


#if defined(CLIENT_SERVER_MODE) 
int Client_DSSE::sendBlock_data(TYPE_INDEX block_index, MatrixType *I_prime)
 {
    int cmd;
    Miscellaneous misc;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    int ret;
    string filename_temp_with_path;
    int n; 
    off_t offset;

    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE;
    try
    {
//        printf("   2.1. Connecting to the server... ");
        socket.connect(PEER_ADDRESS);
//        printf("OK!\n");
        
//        printf("   2.2. Sending update block command...");
        cmd = CMD_UPDATE_BLOCK_DATA;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
//        printf("OK\n");
        
//        printf("   2.3. Sending block index..."); 
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&block_index,sizeof(block_index));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
//        printf("OK\n");
        
        
//        printf("   2.4. Sending block data..."); 
        /* send serialized data */
        socket.send((unsigned char*)I_prime,serialized_buffer_len);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
//        printf("OK\n");
        
    }
    catch (exception &ex)
    {
        ret = CLIENT_UPDATE_BLOCK_DATA_ERR;
        goto exit;
    }
    ret = 0;
exit:
    socket.disconnect(PEER_ADDRESS);
    socket.close();
    memset(buffer_in,0,sizeof(buffer_in));
    memset(buffer_out,0,sizeof(buffer_out));
    filename_temp_with_path.clear();
    return ret;
}
#endif

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
    int ret;
    TYPE_KEYWORD_DICTIONARY extracted_keywords;
    string adding_filename_with_path = path + filename;
    TYPE_INDEX file_index;  
    
    stringstream new_filename_with_path;
    string s;
    
#if defined(CLIENT_SERVER_MODE)
    #if defined(ENCRYPT_PHYSICAL_FILE)
    vector<string> file_names;
    #endif
    
#else
    TYPE_INDEX row;
    TYPE_COUNTER* null_ptr = NULL;
#endif
#if defined(DECRYPT_AT_CLIENT_SIDE)
    DSSE_KeyGen* dsse_keygen = new DSSE_KeyGen();
	
#endif

auto start = time_now;
auto end = time_now;

    try
    {
        if(data_structure_constructed == false)
        {
//            printf("Data structure is not constructed yet, please build it first!\n");
            ret = DATA_STRUCTURE_NOT_BUILT_ERR;
        }
//        printf("\n\n ADDING NEW FILE......\n\n");
//        printf("1. Determining block index...");
        start = time_now;
        if((ret = dsse->requestBlock_index(adding_filename_with_path,block_index,this->T_F,this->masterKey))!=0)
        {
//            printf("Failed!\n");;
        }
//        printf("OK!\n");
//        cout<<"BLOCK INDEX: "<<block_index<<endl;
        end = time_now;
//        cout<<"Get Block index time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"20: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
         stats[9].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
       
#if defined(LOAD_FROM_DISK)
      TYPE_INDEX idx = 0;
        if(ENCRYPT_BLOCK_SIZE < BYTE_SIZE)
            idx = block_index % (BYTE_SIZE / ENCRYPT_BLOCK_SIZE);
  
#endif

#if defined (DECRYPT_AT_CLIENT_SIDE)
        memset(decrypt_key,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
        memset(reencrypt_key,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
    
    #if !defined(MULTI_THREAD)
        start = time_now;
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            dsse_keygen->precomputeAES_CTR_keys(decrypt_key,block_index,COL,this->block_counter_arr,this->row_keys,this->masterKey);
        }
        this->block_counter_arr[block_index]+=1;
        dsse_keygen->precomputeAES_CTR_keys(reencrypt_key,block_index,COL,this->block_counter_arr,this->row_keys,this->masterKey);
        end = time_now;
//        cout<<"Precompute aes key time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"21: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
        stats[10].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());

    #else
        register_cipher(&aes_desc);
   
        THREAD_PRECOMPUTE_AESKEY aes_key_decrypt_param(decrypt_key,block_index,COL,this->block_counter_arr,this->row_keys,this->masterKey);
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            pthread_create(&thread_precomputeAesKey_decrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_decrypt_param);
        }        
        
        //MISS INCREASING BLOCK COUNTER ARRAY TO 1
        THREAD_PRECOMPUTE_AESKEY aes_key_reencrypt_param(reencrypt_key,block_index,COL,this->block_counter_arr,this->row_keys,this->masterKey);
        pthread_create(&thread_precomputeAesKey_reencrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_reencrypt_param);
        
    #endif
    
#endif
    
#if defined (CLIENT_SERVER_MODE)
        if(ENCRYPT_BLOCK_SIZE>1)
        {
    #if !defined (DECRYPT_AT_CLIENT_SIDE)
//        printf("2. Getting block data & state from server...\n");
    #else
//        printf("2. Getting block data from server...\n");
    #endif
    
    #if !defined(MULTI_THREAD) || !defined(DECRYPT_AT_CLIENT_SIDE)
    start = time_now;
            if((ret = this->requestBlock_data(block_index, this->I_prime,this->block_state_arr))!=0)
            {
                cout<<"Failed!"<<endl;
                goto exit;
            }
    end = time_now;
//    cout<<"Get Block data from server time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"22: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
          stats[11].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
      
    #else
         
        THREAD_GETDATA get_data_param(block_index,this->I_prime);
        pthread_create(&thread_getData,NULL,&Client_DSSE::thread_getUpdateData_func,(void*)&get_data_param);
        
    #endif
        }
    #if defined(DECRYPT_AT_CLIENT_SIDE)  && defined(MULTI_THREAD)
        
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            pthread_join(thread_precomputeAesKey_decrypt, NULL);
            pthread_join(thread_getData, NULL);
        }
        pthread_join(thread_precomputeAesKey_reencrypt, NULL);
        
        unregister_cipher(&aes_desc);
   
    #endif
#else 
    #if defined(LOAD_FROM_DISK)
        loadData_from_file(COL,block_index);
    #endif
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            printf("2. Getting block data locally...");
        #if !defined(LOAD_FROM_DISK)    
            dsse->getBlock(block_index,COL,this->I,this->I_prime);
        #else
            dsse->getBlock(idx,COL,this->I_update,this->I_prime);
         
        #endif
            printf("OK!\n");
        }
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
        printf("2.1 Getting state data locally...");
        //this->block_state_arr = new bool[MATRIX_ROW_SIZE];
        for(row = 0 ; row < MATRIX_ROW_SIZE; row++)
        {
        #if !defined(LOAD_FROM_DISK)
            TYPE_INDEX col = block_index / BYTE_SIZE;
            int bit = block_index % BYTE_SIZE;
            if(BIT_CHECK(&this->block_state_mat[row][col].byte_data,bit))
                this->block_state_arr[row] = 1;
            else
                this->block_state_arr[row] = 0;
        #else
            //TYPE_INDEX col = (block_index %(*BYTE_SIZE)) / BYTE_SIZE;
            int bit = block_index  % BYTE_SIZE ;
            if(BIT_CHECK(&this->block_state_mat_update[row][0].byte_data,bit))
                this->block_state_arr[row] = 1;
            else
                this->block_state_arr[row] = 0;
        #endif
        }
        printf("OK!\n");
    #endif
#endif
//        printf("3. Peforming AddToken...");
start = time_now;
#if !defined(DECRYPT_AT_CLIENT_SIDE)
        /* Perform AddToken to add the file to the requested block */
        extracted_keywords.clear();
   
        if((ret = dsse->addToken( adding_filename_with_path,           
                        this->I_prime,
                        gcsEncryptedUpdateFilepath,
                        file_index,
                        this->T_F,this->T_W, extracted_keywords,
                        this->keyword_counter_arr,this->block_counter_arr,this->block_state_arr,
                        this->masterKey)) !=0)
        {
//            printf("Failed!\n");
            goto exit;
        }        

#else
        dsse->addToken(adding_filename_with_path,this->I_prime,gcsEncryptedUpdateFilepath,file_index,this->T_F,this->T_W,extracted_keywords,decrypt_key,reencrypt_key,this->masterKey);        
#endif
end = time_now;
//        cout<<"AddToken time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"23: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
        stats[12].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());

//        printf("OK!\n");        
        keywords_dictionary.insert(extracted_keywords.begin(),extracted_keywords.end());
        
#if defined(CLIENT_SERVER_MODE)     
        /* Send the newly updated block data to the server */
//        printf("5. Sending I\' to server...\n");
        start = time_now;
        if((ret = this->sendBlock_data(block_index,this->I_prime))!=0)
        {
            printf("Failed!\n");
            goto exit;
        }
        end = time_now;
//        cout<<"Upload new block data time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"24: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
          stats[13].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
      
    #if defined(ENCRYPT_PHYSICAL_FILE)
        /* Send the newly added encrypted file to the server */
//        printf("6. Sending newly added encrypted file....\n");
        misc.extract_file_names(file_names,gcsEncryptedUpdateFilepath);
        if((ret = this->sendFile(file_names[0],gcsEncryptedUpdateFilepath,CMD_ADD_FILE_PHYSICAL))!=0)
        {
//            printf("Failed!\n");
            goto exit;
        }
    #endif

//        printf("\n---------------------------\n");
#else  
        printf("4. Peforming Local Add...");
    #if defined(LOAD_FROM_DISK)
        if((ret = dsse->update(I_prime,
                   idx, this->I_update,
                    NULL,NULL)) !=0)
        {
            printf("Failed!\n");
            goto exit;
        }
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
        int bit = block_index % BYTE_SIZE ;
        for(row = 0 ; row < MATRIX_ROW_SIZE; row++)
        {
            BIT_SET(&this->block_state_mat_update[row][0].byte_data,bit);
        }
     #endif
    #else
        if((ret = dsse->update(I_prime,
                   block_index,
                    this->I,
                    NULL,this->block_state_mat,*null_ptr)) !=0)
        {
            printf("Failed!\n");
            goto exit;
        }
    #endif
        
        printf("OK!\n");
    #if defined(LOAD_FROM_DISK)
        printf("5. Saving new data to files...");
        saveData_to_file(COL,block_index);
        printf("OK\n");
    #endif
        
//        cout<<endl<<"---------------------------"<<endl;
#endif
    
    //move the file to the stored folder 
    /*
    new_filename_with_path << gcsFilepath << filename;
    s = new_filename_with_path.str();
    std::rename(adding_filename_with_path.c_str(),new_filename_with_path.str().c_str());
        printf("\nFinished!\n");
        printf("Load factor of keyword hash table: \t\t %1.10f \n",this->T_W.load_factor());
        printf("# new keywords: \t\t\t\t %zu \n",extracted_keywords.size());
        printf("# total distinct keywords: \t\t\t %zu \n\n",keywords_dictionary.size());
        printf("Load factor of file hash table: \t\t %1.10f \n ",this->T_F.load_factor());
      */  
    }
    catch(exception &ex)
    {
        ret = CLIENT_ADD_FILE_ERR;
        goto exit;
    }    
    ret = 0;

exit:
    extracted_keywords.clear();

    delete dsse;
#if defined(CLIENT_SERVER_MODE)

#if defined(ENCRYPT_PHYSICAL_FILE)
    file_names.clear();
#endif
#endif

#if defined (DECRYPT_AT_CLIENT_SIDE)
    delete dsse_keygen;
	
#endif
    return ret;
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
 * @param number: (input) location of deleting file
 * @return	0 if successful
 */
int Client_DSSE::delFile(string filename, string path)
{
    Miscellaneous misc;
    DSSE* dsse = new DSSE();
    int ret;
    TYPE_INDEX block_index;
    TYPE_INDEX file_index;        
    
    string deleting_filename_with_path = path + filename;
    stringstream new_filename_with_path;
    string s;
#if defined (CLIENT_SERVER_MODE)
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] ;
    unsigned char buffer_out[SOCKET_BUFFER_SIZE];
    string encrypted_filename = "";
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
#else
    TYPE_COUNTER* null_ptr = NULL;
    TYPE_INDEX row;
#endif
#if defined (ENCRYPT_PHYSICAL_FILE)
    int cmd;
#endif

#if defined(DECRYPT_AT_CLIENT_SIDE)
    DSSE_KeyGen* dsse_keygen = new DSSE_KeyGen();
#endif
auto start = time_now;
auto end = time_now;
    try
    {
        if(data_structure_constructed == false)
        {
//            printf("Data structure is not constructed yet, please build it first! \n");
            ret = DATA_STRUCTURE_NOT_BUILT_ERR;
            goto exit;
        }
        
//        printf("\n\n DELETING FILE...\n\n");
//        printf("1. Determining the block index...");
    start = time_now;
        if((ret = dsse->requestBlock_index(deleting_filename_with_path,block_index,this->T_F,this->masterKey))!=0)
        {
//            printf("Failed!\n");
            goto exit;
        }
//        printf("OK!\n");
    end = time_now;
//    cout<<"Get Block index time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"20: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
            stats[9].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
    
#if defined(LOAD_FROM_DISK)
        TYPE_INDEX idx = 0;
        if(ENCRYPT_BLOCK_SIZE < BYTE_SIZE)
            idx = block_index % (BYTE_SIZE / ENCRYPT_BLOCK_SIZE);
//        cout<<"idx: "<<idx<<endl;
      
#endif

#if defined (DECRYPT_AT_CLIENT_SIDE)
        //precompute AES keys
        memset(decrypt_key,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
        memset(reencrypt_key,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
   

   
    #if !defined(MULTI_THREAD)
    start = time_now;
        dsse_keygen->precomputeAES_CTR_keys(decrypt_key,block_index,COL,this->block_counter_arr,this->row_keys,this->masterKey);
        this->block_counter_arr[block_index]+=1;
        dsse_keygen->precomputeAES_CTR_keys(reencrypt_key,block_index,COL,this->block_counter_arr,this->row_keys,this->masterKey);
    end = time_now;
//    cout<<"21: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
        stats[10].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());

    #else
        register_cipher(&aes_desc);
        THREAD_PRECOMPUTE_AESKEY aes_key_decrypt_param(decrypt_key,block_index,COL,this->block_counter_arr,this->row_keys,this->masterKey);
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            pthread_create(&thread_precomputeAesKey_decrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_decrypt_param);
        }
        THREAD_PRECOMPUTE_AESKEY aes_key_reencrypt_param(reencrypt_key,block_index,COL,this->block_counter_arr,this->row_keys,this->masterKey);
        pthread_create(&thread_precomputeAesKey_reencrypt,NULL,&Client_DSSE::thread_precomputeAesKey_func,(void*)&aes_key_reencrypt_param);
     
    #endif
    
#endif

#if defined(CLIENT_SERVER_MODE)
        if(ENCRYPT_BLOCK_SIZE>1)
        {
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
//            printf("2. Getting block data & state from server...\n");
    #else
//            printf("2. Getting block data from server...\n");
    #endif
    
    #if !defined(MULTI_THREAD) || !defined(DECRYPT_AT_CLIENT_SIDE)
    start = time_now;
            if((ret = this->requestBlock_data(block_index, this->I_prime, this->block_state_arr)) != 0 )
            {
                goto exit;
            }
    end = time_now;
//    cout<<"Get Block data from the server time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"22: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
          stats[11].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
     
    #else
        THREAD_GETDATA get_data_param(block_index,this->I_prime);
        pthread_create(&thread_getData,NULL,&Client_DSSE::thread_getUpdateData_func,(void*)&get_data_param);
    #endif
        }
    #if defined(DECRYPT_AT_CLIENT_SIDE) && defined(MULTI_THREAD)
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            pthread_join(thread_precomputeAesKey_decrypt, NULL);
            pthread_join( thread_getData, NULL);
            
        }
        pthread_join(thread_precomputeAesKey_reencrypt, NULL);
        
        unregister_cipher(&aes_desc);
    #endif
    
#else
    #if defined(LOAD_FROM_DISK)
        loadData_from_file(COL,block_index);
    #endif
        if(ENCRYPT_BLOCK_SIZE>1)
        {
            printf("2. Getting block data locally...");
    #if !defined(LOAD_FROM_DISK)  
            dsse->getBlock(block_index,COL,this->I,this->I_prime);
    #else
            dsse->getBlock(idx,COL,this->I_update,this->I_prime);
    #endif
        }
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
        printf("2.1 Getting state data locally...");
        //this->block_state_arr = new bool[MATRIX_ROW_SIZE];
        for(row = 0 ; row < MATRIX_ROW_SIZE; row++)
        {
        #if !defined(LOAD_FROM_DISK)
            TYPE_INDEX col = block_index / BYTE_SIZE;
            int bit = block_index % BYTE_SIZE;
            if(BIT_CHECK(&this->block_state_mat[row][col].byte_data,bit))
                this->block_state_arr[row] = 1;
            else
                this->block_state_arr[row] = 0;
        #else
            int bit = block_index  % BYTE_SIZE ;
            if(BIT_CHECK(&this->block_state_mat_update[row][0].byte_data,bit))
                this->block_state_arr[row] = 1;
            else
                this->block_state_arr[row] = 0;
        #endif
        }        
        printf("OK!\n");
    #endif
#endif

   
//printf("3. Peforming DelToken...");
start = time_now;
#if !defined(DECRYPT_AT_CLIENT_SIDE)
        dsse->delToken( deleting_filename_with_path,                   
                        this->I_prime,                    
                        file_index,
                        this->T_F,this->T_W,
                        this->keyword_counter_arr,this->block_counter_arr,this->block_state_arr,
                        this->masterKey);
#else
        dsse->delToken(deleting_filename_with_path,this->I_prime,file_index,this->T_F,this->T_W,decrypt_key,reencrypt_key,this->masterKey);
#endif
end = time_now;
//cout<<"DelToken time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"23: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
          stats[12].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
  
//        printf("OK!\n");
#if defined(CLIENT_SERVER_MODE)
    start = time_now;
//        printf("5. Sending updated data to server\n");
        this->sendBlock_data(block_index,this->I_prime);
     end = time_now;
//        cout<<"Upload new block data time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"24: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
          stats[13].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
         
    #if defined(ENCRYPT_PHYSICAL_FILE)
        
        printf("6. Sending file index needs to be deleted...\n");
        printf("   6.1. Connecting to server...");
        socket.connect(PEER_ADDRESS);
        printf("OK!\n");
        
        printf("   6.2. Sending File Delete Command...");
        cmd = CMD_DELETE_FILE_PHYSICAL;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK!\n");
        
        printf("   6.3. Sending file index...");
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&file_index,sizeof(file_index));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK!\n");
    #endif

//        printf("\n---------------------------\n");
#else
        printf("3. Peforming Local DelToken...");
    #if defined(LOAD_FROM_DISK)
        if((ret = dsse->update(this->I_prime,
                   idx,
                    this->I_update,
                    NULL,NULL))!=0)
        {
            printf("Failed!\n");
            goto exit;
        }
     #if !defined(DECRYPT_AT_CLIENT_SIDE)
        int bit = block_index % BYTE_SIZE ;
        for(row = 0 ; row < MATRIX_ROW_SIZE; row++)
        {
            BIT_SET(&this->block_state_mat_update[row][0].byte_data,bit);
        }
     #endif
    #else
        if((ret = dsse->update(this->I_prime,
                   block_index,
                    this->I,
                    NULL,this->block_state_mat,*null_ptr))!=0)
        {
            printf("Failed!\n");
            goto exit;
        }
    #endif
        
        printf("OK!\n");
    #if defined(LOAD_FROM_DISK)
        printf("5. Saving new data to files...");
        saveData_to_file(COL,block_index);
        printf("OK!\n");
    #endif
//        printf("\n---------------------------\n");
#endif
        //remove pysical files
        /*
        new_filename_with_path << gcsFilepath << filename;
        s = new_filename_with_path.str();
        remove(s.c_str());
        */
        
//        printf("\nFinished!\n");
//        printf("Load factor of file hash table: \t\t %1.10f \n",this->T_F.load_factor());
//        printf("Load factor of keyword hash table: \t\t %1.10f\n",this->T_W.load_factor());
    }
    catch (exception &ex)
    {
        ret = CLIENT_DELETE_FILE_ERR;
        goto exit;
    }
    ret = 0;
exit:

#if defined (CLIENT_SERVER_MODE)
    memset(buffer_in,0,SOCKET_BUFFER_SIZE) ;
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    encrypted_filename.clear();
#endif

#if defined(DECRYPT_AT_CLIENT_SIDE)
    delete dsse_keygen;

        
#endif
    delete dsse;
    return ret;
}
#if defined(CLIENT_SERVER_MODE)
int Client_DSSE::requestSearch_data(TYPE_INDEX row_index, MatrixType* I_prime) 
{
    int cmd;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    int len, ret;
    off_t offset;
    
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    int64_t more;
    size_t more_size = sizeof(more);
    
    TYPE_INDEX serialized_buffer_len = MATRIX_COL_SIZE;
    //MatrixType* serialized_buffer = new MatrixType[serialized_buffer_len]; //consist of data and block state
    
    TYPE_INDEX row,ii,state_col,state_bit_position;
    try
    {   
        //memset(serialized_buffer,0,serialized_buffer_len);
        
//        printf("   2.1. Connecting to the server... ");
        socket.connect(PEER_ADDRESS);
//        printf("OK!\n");
        
//        printf("   2.2. Sending request command...");
        cmd = CMD_REQUEST_SEARCH_DATA;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
        //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
//        printf("OK\n");
        
//        printf("   2.3. Sending RequestIndex token..."); 
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&row_index,sizeof(row_index));

        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
//        printf("OK\n");
        
//        printf("   2.4. Receiving data..."); 
        offset = 0;
        socket.recv((unsigned char*)I_prime,serialized_buffer_len);
        //socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));

    }
    catch(exception &ex)
    {
        ret = CLIENT_REQUEST_BLOCK_DATA_ERR;
        goto exit;
    }
    ret = 0;

exit:
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    socket.disconnect(PEER_ADDRESS);
    socket.close();
    return ret;
}
#endif


/**
 * Function Name: loadData_from_file
 *
 * Description:
 * Load a piece of DSSE data structure from file, given an index and the dimension
 * 
 * @param dim: (input) dimension ( COL or ROW)
 * @param idx: (input) index
 * @return	0 if successful
 */
#if defined(LOAD_FROM_DISK)
int Client_DSSE::loadData_from_file(int dim, TYPE_INDEX idx)
{
    DSSE* dsse = new DSSE();
    if(dim == ROW)
    {
        dsse->loadEncrypted_matrix_from_files(this->I_search,dim,idx);
     #if !defined(DECRYPT_AT_CLIENT_SIDE)
        dsse->loadBlock_state_matrix_from_file(this->block_state_mat_search,dim,idx);
     #endif
    }
    else
    {
        dsse->loadEncrypted_matrix_from_files(this->I_update,dim,(idx));
      #if !defined(DECRYPT_AT_CLIENT_SIDE)
        dsse->loadBlock_state_matrix_from_file(this->block_state_mat_update,dim,(idx));
      #endif
    }
    delete dsse;
}

/**
 * Function Name: saveData_to_file
 *
 * Description:
 * Save a piece of DSSE data structure to file, given an index and the dimension
 * 
 * @param dim: (input) dimension ( COL or ROW)
 * @param idx: (input) index
 * @return	0 if successful
 */
int Client_DSSE::saveData_to_file(int dim, TYPE_INDEX idx)
{
    DSSE* dsse = new DSSE();
    if(dim == ROW)
    {
        dsse->saveEncrypted_matrix_to_files(this->I_search,dim,idx);
        
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
        dsse->saveBlock_state_matrix_to_file(this->block_state_mat_search,dim,idx);
    #endif
    }
    else
    {
        dsse->saveEncrypted_matrix_to_files(this->I_update,COL,idx);
      #if !defined(DECRYPT_AT_CLIENT_SIDE)
        dsse->saveBlock_state_matrix_to_file(this->block_state_mat_update,COL,idx);
      #endif
    }
    delete dsse;
}
#endif

#if defined (MULTI_THREAD)
void *Client_DSSE::thread_precomputeAesKey_func(void* param)
{
    auto start = time_now;
    
    THREAD_PRECOMPUTE_AESKEY* opt = (THREAD_PRECOMPUTE_AESKEY*) param;
    DSSE_KeyGen dsse_keygen;
    dsse_keygen.precomputeAES_CTR_keys(opt->aes_keys,opt->idx,opt->dim,opt->block_counter_arr,opt->row_keys,opt->masterKey);
    auto end = time_now;
//    cout<<"precompute AES key in thread time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    if(opt->dim == ROW)
    {
        //cout<<"15: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
        stats[4].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());

    }
    else
    {
    //   cout<<"25: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
            stats[14].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());

    }
    pthread_exit((void*)opt);
}
void *Client_DSSE::thread_getSearchData_func(void* param)
{
    auto start = time_now;
    THREAD_GETDATA* opt = (THREAD_GETDATA*) param;
    Client_DSSE* call = new Client_DSSE();
    call->requestSearch_data(opt->idx, opt->data);
    auto end = time_now;
//    cout<<"Get Search Data in thread time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"16: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    stats[5].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
         
    delete call;
    pthread_exit((void*)opt);
}
void *Client_DSSE::thread_getUpdateData_func(void* param)
{
    auto start = time_now;
    THREAD_GETDATA* opt = (THREAD_GETDATA*) param;
    Client_DSSE* call = new Client_DSSE();
    call->requestBlock_data(opt->idx,opt->data,NULL);
    auto end = time_now;
//    cout<<"Get Update data in thread time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"17: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    stats[6].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
       
    delete call;
    pthread_exit((void*)opt);
}
#endif
