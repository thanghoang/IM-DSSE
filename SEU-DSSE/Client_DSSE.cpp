#include "Client_DSSE.h"
#include "DSSE_Param.h"
#include "DSSE_KeyGen.h"
#include "string.h"
#include <sstream>	
#include "Miscellaneous.h"

#include "DSSE.h"

#if defined (CLIENT_SERVER_MODE)
#include "zmq.hpp"
using namespace zmq;
#endif

TYPE_COUNTER Client_DSSE::gc = 1;

Client_DSSE::Client_DSSE()
{
    
    data_structure_constructed = false;
    
    for(int i = 0 ; i <BLOCK_CIPHER_SIZE;i++)
    {
        this->extractor_salt[i] = 0;
        this->pseudo_random_key[i] = 0;
    }
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
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
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

    try
    {
        printf("0. Allocating memory for data structure......");
#if !defined(CLIENT_SERVER_MODE)
        I = new MatrixType*[MATRIX_ROW_SIZE];
        for(TYPE_INDEX row = 0 ; row <MATRIX_ROW_SIZE; row++)
        {
            I[row] = new MatrixType[MATRIX_COL_SIZE];
            memset(I[row],0,MATRIX_COL_SIZE);
        }
        this->block_state_mat = new bool*[MATRIX_ROW_SIZE];
        for (TYPE_INDEX row = 0 ;  row < MATRIX_ROW_SIZE; row ++)
        {
            this->block_state_mat[row] = new bool[NUM_BLOCKS];
            memset(this->block_state_mat[row],ZERO_VALUE,NUM_BLOCKS);
        }
#endif
        /* Allocate memory for I' & block state array */
        I_prime = new MatrixType[MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE];
        memset(I_prime,0,MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE/BYTE_SIZE);
        printf("OK!\n");
      
        block_state_arr = new bool[MATRIX_ROW_SIZE];
        memset(block_state_arr,0,MATRIX_ROW_SIZE);

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
        
#if defined(CLIENT_SERVER_MODE)

        /* Write block_counter_arr, global counter to the file */
        printf("2. Writing data structure to files...");
        misc.write_array_to_file(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
        misc.write_counter_to_file(FILENAME_GLOBAL_COUNTER,gcsDataStructureFilepath,this->gc);
        printf("OK!\n");


        printf("3. Sending data structure to server...\n");

#if !defined(UPLOAD_DATA_STRUCTURE_MANUALLY_MODE)
        printf("   3.1. Matrix I...\n");
        int n = MATRIX_COL_SIZE / MATRIX_PIECE_COL_SIZE;
        for(int i = 0 ; i < n ; i++)
        {
            string filename = std::to_string(i);
            this->sendFile(filename,gcsMatrixPiecePath,CMD_SEND_DATA_STRUCTURE);
        }
#else
        printf("Please copy generated data structure to server before continue...");
        cin.get();
#endif
        printf("   3.2. Block state...\n");
        if((ret = this->sendFile(FILENAME_BLOCK_STATE_MATRIX,gcsDataStructureFilepath,CMD_SEND_DATA_STRUCTURE))!=0)
        {
            goto exit;
        }
        printf("   3.3. Block counter...\n");
        if((ret = this->sendFile(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,CMD_SEND_DATA_STRUCTURE))!=0)
        {
            goto exit;
        }
        printf("   3.4. Global counter...\n");
        if((ret = this->sendFile(FILENAME_GLOBAL_COUNTER,gcsDataStructureFilepath,CMD_SEND_DATA_STRUCTURE))!=0)
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
    
    try
    {
        if(data_structure_constructed == false)
        {
            printf("Data structure is not constructed yet, please build it first!\n");
            ret = DATA_STRUCTURE_NOT_BUILT_ERR;
            goto exit;
        }
        printf("\n\n Searching \"%s\" ....\n\n", keyword.c_str());
        
        /*Generate the token for this keyword using SrchToken procedure*/        
        printf("1. Generating keyword token...");
        if( (ret = dsse->searchToken(  tau,
                            keyword,
                            this->T_W,
                            this->keyword_counter_arr,
                            this->masterKey)) != 0 )
        {
            printf("Error! SearchToken generation failed\n");
            goto exit;
        }
        printf("OK!\n");
        if(tau.row_index == KEYWORD_NOT_EXIST)
        {
            printf("Keyword not exist!\n");
            goto exit;
        }
        /* 2. Peform the search...*/
#if defined(CLIENT_SERVER_MODE)
        printf("2. Sending keyword token to sever...\n");
        
        //network connection
        printf("   2.1. Connecting to the server...");
        socket.connect(PEER_ADDRESS);
        printf("connected!\n");
        
        printf("   2.2. Sending search request...");
        cmd = CMD_SEARCH_OPERATION;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK!\n");
        
        printf("   2.3. Sending keyword token with row index: %lu...",tau.row_index);
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(&buffer_out,&tau,sizeof(SearchToken));
        socket.send(buffer_out,sizeof(SearchToken));
        printf("OK! \n");
        
        printf("   2.4. Receiving result...");;        
#if defined (SEND_SEARCH_FILE_INDEX)
        
        // open the file to receive first
        printf("\n     2.4.1 Opening temp file...");
        if((foutput = fopen(filename_search_result.c_str(),"wb+"))==NULL)
        {
            printf("failed!\n");
            ret = FILE_OPEN_ERR;
            goto exit;
        }
        printf("OK!\n");
        printf("       2.4.2 Receiving specific file indices...");
        memset(buffer_in,0,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        memcpy(&file_in_size,buffer_in,sizeof(size_t));
        // Send ACK
        socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        
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
        printf("OK!\n");
        
        printf("   2.4.  Loading results...");
        lstFile_id.clear();
        misc.read_list_from_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
        if(lstFile_id.size() == 1 && lstFile_id[0]==ULONG_MAX)
        {
            lstFile_id.clear();
            number = 0;
        }
        else
            number = lstFile_id.size();
        printf("OK!\n");
        
#else
        memset(buffer_in,0,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        memcpy(&number,buffer_in,sizeof(number));
        printf("OK!\n");
#endif

        printf("\n---------------------------\n");
        
#else   
        
        printf("2. Performing local search...");
        lstFile_id.clear();
        if((ret=dsse->search(lstFile_id,tau,this->I,
                            this->block_counter_arr,
                            this->block_state_mat))!=0)
        {
            printf("Error during performing local search...\n");
            goto exit;
        }
        printf("OK!\n");
        number = lstFile_id.size();
        Miscellaneous::write_list_to_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
        printf("\n---------------------------\n");
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
#if defined(CLIENT_SERVER_MODE)
    socket.disconnect(PEER_ADDRESS  );
    socket.close();
#if defined(SEND_SEARCH_FILE_INDEX)
    filename_search_result.clear();
#endif
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
    
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE+(MATRIX_ROW_SIZE/BYTE_SIZE);
    MatrixType serialized_buffer[serialized_buffer_len]; //consist of data and block state
    
    TYPE_INDEX row,ii,state_col,state_bit_position;
    try
    {   
        memset(serialized_buffer,0,serialized_buffer_len);
        
        printf("   2.1. Connecting to the server... ");
        socket.connect(PEER_ADDRESS);
        printf("OK!\n");
        
        printf("   2.2. Sending request command...");
        cmd = CMD_REQUEST_BLOCK_DATA;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK\n");
        
        printf("   2.3. Sending RequestIndex token..."); 
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&block_index,sizeof(block_index));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        printf("OK\n");
        
        printf("   2.4. Receiving data..."); 
        offset = 0;
        while(offset<serialized_buffer_len)
        {
            len=socket.recv(&serialized_buffer[offset],SOCKET_BUFFER_SIZE,0);
            offset+=len;
            socket.getsockopt(ZMQ_RCVMORE,&more,&more_size);
            if(!more)
                break;
        }
        socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        
        /* deserialize */
        memcpy(I_prime,serialized_buffer,(MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE);
        for(row = 0,ii=(MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE) ; row < MATRIX_ROW_SIZE; ii++,row++)
        {
            state_col = ii / BYTE_SIZE;
            state_bit_position = ii % BYTE_SIZE;
            if(BIT_CHECK(&serialized_buffer[state_col].byte_data,state_bit_position))
                block_state_arr[row] = ONE_VALUE;
            else
                block_state_arr[row] = ZERO_VALUE;
        }
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
    MatrixType serialized_buffer[serialized_buffer_len]; //consist of data and block state
    try
    {
        printf("   2.1. Connecting to the server... ");
        socket.connect(PEER_ADDRESS);
        printf("OK!\n");
        
        printf("   2.2. Sending update block command...");
        cmd = CMD_UPDATE_BLOCK_DATA;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK\n");
        
        printf("   2.3. Sending block index..."); 
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&block_index,sizeof(block_index));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK\n");
        
        /* serialize I_prime */
        memset(serialized_buffer,0,serialized_buffer_len);
        memcpy(serialized_buffer,I_prime,serialized_buffer_len);
        
        /* send serialized data */
        for(offset = 0 ; offset < serialized_buffer_len; offset +=SOCKET_BUFFER_SIZE)
        {
            n = (serialized_buffer_len - offset > SOCKET_BUFFER_SIZE) ? SOCKET_BUFFER_SIZE : (int) 
                (serialized_buffer_len - offset);
                if(offset + n == serialized_buffer_len)
                    socket.send(&serialized_buffer[offset],n,0);
                else
                    socket.send(&serialized_buffer[offset],n,ZMQ_SNDMORE);
        }
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
    try
    {
        if(data_structure_constructed == false)
        {
            printf("Data structure is not constructed yet, please build it first!\n");
            ret = DATA_STRUCTURE_NOT_BUILT_ERR;
            goto exit;
        }
        printf("\n\n ADDING NEW FILE......\n\n");
        printf("1. Determining block index...");
        if((ret = dsse->requestBlock_index(adding_filename_with_path,block_index,this->T_F,this->masterKey))!=0)
        {
            printf("Failed!\n");;
            goto exit;
        }
        printf("OK!\n");

#if defined (CLIENT_SERVER_MODE)
        printf("2. Getting block data & state from server...\n");
        if((ret = this->requestBlock_data(block_index, this->I_prime,this->block_state_arr))!=0)
        {
            cout<<"Failed!"<<endl;
            goto exit;
        }
#else 
        printf("2. Getting block data & state locally...");
        dsse->fetchBlock_data(block_index,this->I_prime,this->I);
        this->block_state_arr = new bool[MATRIX_ROW_SIZE];
        for(row = 0 ; row < MATRIX_ROW_SIZE; row++)
        {
            this->block_state_arr[row] = this->block_state_mat[row][block_index];
        }
        printf("OK!\n");
        
#endif
        printf("3. Peforming AddToken...");
        /* Perform AddToken to add the file to the requested block */
        extracted_keywords.clear();
        if((ret = dsse->addToken( adding_filename_with_path,                    //input
                        this->I_prime,                        //input first, output later
                        gcsEncryptedUpdateFilepath,      //output
                        file_index,
                        this->T_F,this->T_W, extracted_keywords,
                        this->keyword_counter_arr,this->block_counter_arr,this->block_state_arr,
                        this->gc,
                        this->masterKey)) !=0)
        {
            printf("Failed!\n");
            goto exit;
        }
        keywords_dictionary.insert(extracted_keywords.begin(),extracted_keywords.end());
        printf("OK!\n");
        
#if defined(CLIENT_SERVER_MODE)     
        /* Send the newly updated block data to the server */
        printf("5. Sending I\' to server...\n");
        if((ret = this->sendBlock_data(block_index,this->I_prime))!=0)
        {
            printf("Failed!\n");
            goto exit;
        }
        
#if defined(ENCRYPT_PHYSICAL_FILE)
        /* Send the newly added encrypted file to the server */
        printf("6. Sending newly added encrypted file....\n");
        misc.extract_file_names(file_names,gcsEncryptedUpdateFilepath);
        if((ret = this->sendFile(file_names[0],gcsEncryptedUpdateFilepath,CMD_ADD_FILE_PHYSICAL))!=0)
        {
            printf("Failed!\n");
            goto exit;
        }
#endif

        printf("\n---------------------------\n");
#else  
        printf("4. Peforming Local Add...");
        if((ret = dsse->update(I_prime,
                   block_index,
                    this->I,
                    NULL,this->block_state_mat,*null_ptr)) !=0)
        {
            printf("Failed!\n");
            goto exit;
        }
        printf("OK\n");
        cout<<endl<<"---------------------------"<<endl;
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
    
    string adding_filename_with_path = path + filename;
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
    
    try
    {
        if(data_structure_constructed == false)
        {
            printf("Data structure is not constructed yet, please build it first! \n");
            ret = DATA_STRUCTURE_NOT_BUILT_ERR;
            goto exit;
        }
        
        printf("\n\n DELETING FILE...\n\n");
        printf("1. Determining the block index...");

        if((ret = dsse->requestBlock_index(adding_filename_with_path,block_index,this->T_F,this->masterKey))!=0)
        {
            printf("Failed!\n");
            goto exit;
        }
        printf("OK!\n");
        
#if defined (CLIENT_SERVER_MODE)
        printf("2. Geting the block data from server\n");
        if((ret = this->requestBlock_data(block_index, this->I_prime, this->block_state_arr)) != 0 )
        {
            goto exit;
        }
#else
        printf("2. Getting block data & state locally...");
        dsse->fetchBlock_data(block_index,this->I_prime,this->I);
        this->block_state_arr = new bool[MATRIX_ROW_SIZE];
        for(row = 0 ; row < MATRIX_ROW_SIZE; row++)
        {
            block_state_arr[row] = this->block_state_mat[row][block_index];
        }
        printf("OK!\n");
#endif
        printf("3. Peforming DelToken...");
        dsse->delToken( adding_filename_with_path,                   
                        this->I_prime,                    
                        file_index,
                        this->T_F,this->T_W,
                        this->keyword_counter_arr,this->block_counter_arr,this->block_state_arr,
                        this->gc,
                        this->masterKey);
        printf("OK!\n");
#if defined(CLIENT_SERVER_MODE)
        
        printf("5. Sending updated data to server\n");
        this->sendBlock_data(block_index,this->I_prime);
        
#if defined(ENCRYPT_PHYSICAL_FILE)
        
        printf("6. Sending file index needs to be deleted...\n");
        printf("   6.1. Connecting to server...");
        socket.connect(PEER_ADDRESS);
        printf("OK!\n");
        
        printf("   6.2. Sending File Delete Command...");
        cmd = CMD_DELETE_FILE_PHYSICAL;
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&cmd,sizeof(cmd));
        
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK!\n");
        
        printf("   6.3. Sending file index...");
        memset(buffer_out,0,SOCKET_BUFFER_SIZE);
        memcpy(buffer_out,&file_index,sizeof(file_index));
        socket.send(buffer_out,SOCKET_BUFFER_SIZE);
        socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
        printf("OK!\n");
#endif

        printf("\n---------------------------\n");
#else
       
        printf("3. Peforming Local DelToken...");
        if((ret = dsse->update(this->I_prime,
                   block_index,
                    this->I,
                    NULL,this->block_state_mat,*null_ptr))!=0)
        {
            printf("Failed!\n");
            goto exit;
        }
        printf("OK!\n");
        
        printf("\n---------------------------\n");
#endif
        //remove pysical files
        /*
        new_filename_with_path << gcsFilepath << filename;
        s = new_filename_with_path.str();
        remove(s.c_str());
        */
        
        printf("\nFinished!\n");
        printf("Load factor of file hash table: \t\t %1.10f \n",this->T_F.load_factor());
        printf("Load factor of keyword hash table: \t\t %1.10f\n",this->T_W.load_factor());
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
    delete dsse;
    return ret;
}