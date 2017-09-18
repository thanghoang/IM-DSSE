
#include "Server_DSSE.h"
#include "config.h"
#include "DSSE_KeyGen.h"

#include "Miscellaneous.h"

#include "DSSE.h"
#include "zmq.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
using namespace zmq;


Server_DSSE::Server_DSSE()
{
    TYPE_INDEX i;
    
    /* Allocate memory for data structure (matrix) */
#if !defined(DECRYPT_AT_CLIENT_SIDE)
    block_counter_arr = new TYPE_COUNTER[NUM_BLOCKS];
    for(int i = 0 ; i < NUM_BLOCKS; i++)
        block_counter_arr[i]=1;
#endif

#if !defined(DISK_STORAGE_MODE)
        this->I = new MatrixType *[MATRIX_ROW_SIZE];
        for (i = 0; i < MATRIX_ROW_SIZE;i++ )
        {
            this->I[i] = new MatrixType[MATRIX_COL_SIZE];
        }
    #if !defined(DECRYPT_AT_CLIENT_SIDE)
        /* Allocate memory for state matrix */
        this->block_state_mat = new MatrixType *[MATRIX_ROW_SIZE];
        for(i = 0 ; i < MATRIX_ROW_SIZE; i++)
        {
            this->block_state_mat[i] = new MatrixType [NUM_BLOCKS/BYTE_SIZE];
            memset(this->block_state_mat[i],ZERO_VALUE,NUM_BLOCKS/BYTE_SIZE);
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
}

Server_DSSE::~Server_DSSE()
{
    
}

/**
 * Function Name: start()
 *
 * Description:
 * Start the DSSE program in server side (e.g., open and listen port)
 *
 * @param socket: (output) opening socket
 * @return	0 if successful
 */
int Server_DSSE::start()
{
    unsigned char buffer[SOCKET_BUFFER_SIZE];
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REP);
    socket.bind("tcp://*:"+SERVER_PORT);
    DSSE dsse;
    
    do
    {
        printf("Waiting for request......\n\n");
        while(!socket.connected());
        
        /* 1. Read the command sent by the client to determine the job */
        socket.recv(buffer,SOCKET_BUFFER_SIZE);
        
        int cmd;
        memcpy(&cmd,buffer,sizeof(cmd));
        
        switch(cmd)
        {
        case CMD_SEND_DATA_STRUCTURE:
            printf("*INIT ENCRYPTED INDEX!* requested\n");
            this->getEncrypted_data_structure(socket);
            break;
        case CMD_LOADSTATE:
            this->loadState();
            socket.send(CMD_SUCCESS,sizeof(CMD_SUCCESS),0);
            break;
        case CMD_SAVESTATE:
            this->saveState();
            socket.send(CMD_SUCCESS,sizeof(CMD_SUCCESS),0);
            break;
        case CMD_SEARCH_OPERATION:
            printf("*SEARCH!* requested\n");
            this->searchKeyword(socket);
            break;
        case CMD_REQUEST_BLOCK_DATA:
            printf("*GET ENCRYPTED UPDATE DATA!* requested\n");
            this->getBlock_data(socket,COL);
            break;
        case CMD_UPDATE_BLOCK_DATA:
            printf("*UPDATE ENCRYPTED INDEX!* requested\n");
            this->updateBlock_data(socket);
            break;
        case CMD_REQUEST_SEARCH_DATA:
            printf("*GET ENCRYPTED SEARCH DATA!* requested\n");
            this->getBlock_data(socket,ROW);
            break;
        default:
            break;
        }
        
    }while(1);


    memset(buffer,0,SOCKET_BUFFER_SIZE);
    return 0;
}

/**
 * Function Name: loadState()
 *
 * Description:
 * Load the previous state into the memory
 *
 * @return	0 if successful
 */
int Server_DSSE::loadState()
{
#if !defined(DISK_STORAGE_MODE)
    printf("   Loading encrypted index...");
    DSSE* dsse = new DSSE();
    
    dsse->loadEncrypted_matrix_from_files(this->I);
    printf("OK!\n");
    
  #if !defined(DECRYPT_AT_CLIENT_SIDE)
    printf("   Loading block state matrix...");
    dsse->loadBlock_state_matrix_from_file(this->block_state_mat);
    printf("OK!\n");
  #endif
    delete dsse;

#endif
#if !defined(DECRYPT_AT_CLIENT_SIDE)
    printf("   Loading block counter array...");
    Miscellaneous::read_array_from_file(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
    printf("OK!\n");
#endif

}


/**
 * Function Name: saveState()
 *
 * Description:
 * Save the current sate into memory
 *
 * @return	0 if successful
 */
int Server_DSSE::saveState()
{
#if !defined(DISK_STORAGE_MODE)
    printf("   Saving encrypted index...");
    DSSE* dsse = new DSSE();
    dsse->saveEncrypted_matrix_to_files(this->I);
    printf("OK!\n");
    
  #if !defined(DECRYPT_AT_CLIENT_SIDE)
    printf("   Saving block state matrix...");
    dsse->saveBlock_state_matrix_to_file(this->block_state_mat);
    printf("OK!\n");
  #endif
  
    delete dsse;
#endif

#if !defined(DECRYPT_AT_CLIENT_SIDE)
    printf("   Saving block counter array...");
    Miscellaneous::write_array_to_file(FILENAME_BLOCK_COUNTER_ARRAY,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
    printf("OK!\n");
#endif


    
}
/**
 * Function Name: updateBlock_data
 *
 * Description:
 * Process the update block data request from the client
 *
 * @param socket: (output) opening socket
 * @return	0 if successful
 */
int Server_DSSE::updateBlock_data(zmq::socket_t& socket)
{
    auto start = time_now;
    auto end = time_now;
   Miscellaneous misc;
    DSSE *dsse = new DSSE();
    
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    
    TYPE_INDEX block_index;
    
    
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE;
    MatrixType* serialized_buffer = new MatrixType[serialized_buffer_len]; //consist of data 
   start = time_now; 
    printf("1.  Receiving block index....");
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE,ZMQ_RCVMORE);
           
    memcpy(&block_index,buffer_in,sizeof(block_index));
    end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

    // Receive block data sent by the client 
    memset(serialized_buffer,0,serialized_buffer_len);
    start = time_now;
    printf("2.  Receiving block data....");
    socket.recv(serialized_buffer,serialized_buffer_len);
    end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    
    // Update the received I' 
    printf("4. Calling Update function...");

    start = time_now;
    
#if !defined(DISK_STORAGE_MODE)
    dsse->update(serialized_buffer,block_index,this->I,this->block_counter_arr,this->block_state_mat);
    
#else
    if(ENCRYPT_BLOCK_SIZE==1)
    {
        this->loadData_from_file(COL,block_index);
    }
    TYPE_INDEX idx = 0;
    if(ENCRYPT_BLOCK_SIZE < BYTE_SIZE)
        idx = block_index % (BYTE_SIZE / ENCRYPT_BLOCK_SIZE);
    dsse->update(serialized_buffer,
                    idx,
                    this->I_update,
                    NULL,NULL);
                    
 #if !defined(DECRYPT_AT_CLIENT_SIDE)
    this->block_counter_arr[block_index]+=1;

    int bit = (block_index) % BYTE_SIZE ;
    for(TYPE_INDEX row = 0 ; row < MATRIX_ROW_SIZE; row++)
    {
        BIT_SET(&this->block_state_mat_update[row][0].byte_data,bit);
    }
  #endif
  
    saveData_to_file(COL,block_index);

#endif
end = time_now;
cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS)); //Temp use
    
    delete[] serialized_buffer;
    delete dsse;
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    return 0 ; 
}

/**
 * Function Name: getBlock_data
 *
 * Description:
 * Process the block data request from the client
 *
 * @param socket: (output) opening socket
 * @return	0 if successful
 */
int Server_DSSE::getBlock_data(zmq::socket_t & socket, int dim)
{
    Miscellaneous misc;
    DSSE *dsse = new DSSE();
   
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    
    TYPE_INDEX block_index;
    TYPE_INDEX serialized_buffer_len;
auto start = time_now;
auto end = time_now;
#if !defined(DECRYPT_AT_CLIENT_SIDE)
    if(dim == COL)
        serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE+(MATRIX_ROW_SIZE/BYTE_SIZE);
    else
        serialized_buffer_len = MATRIX_COL_SIZE;
#else
    if(dim == COL)
        serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE;
    else
        serialized_buffer_len = MATRIX_COL_SIZE;
#endif
    MatrixType* serialized_buffer = new MatrixType[serialized_buffer_len]; //consist of data and block state
    memset(serialized_buffer,0,serialized_buffer_len);
    
    start = time_now;
    printf("1.  Receiving row/column index requested....");
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

    memcpy(&block_index,buffer_in,sizeof(block_index));
    
    printf("2.  Retrieving the requested row/column....");
start = time_now;
    
#if defined(DISK_STORAGE_MODE)
    this->loadData_from_file(dim,block_index);
    TYPE_INDEX idx = 0;
        if(ENCRYPT_BLOCK_SIZE < BYTE_SIZE)
            idx = block_index % (BYTE_SIZE / ENCRYPT_BLOCK_SIZE);
    if(dim==ROW)
    {
        dsse->getBlock(0,dim,this->I_search,serialized_buffer);
    }
    else
    {
        dsse->getBlock( idx,dim,this->I_update,serialized_buffer);
    }
#else
    dsse->getBlock(block_index,dim,this->I,serialized_buffer);
#endif  
    end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

    printf("3.  Serializing...");    
    start = time_now;
    if(dim == COL)
    {
#if !defined(DECRYPT_AT_CLIENT_SIDE)
        TYPE_INDEX row,ii,state_col,state_bit_position;
        for(row = 0, ii = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE) ; row < MATRIX_ROW_SIZE ; row++, ii++)
        {
            state_col = ii / BYTE_SIZE;
            state_bit_position = ii % BYTE_SIZE;
    #if !defined(DISK_STORAGE_MODE)
            TYPE_INDEX col = block_index / BYTE_SIZE;
            int bit = block_index % BYTE_SIZE;
            if(BIT_CHECK(&this->block_state_mat[row][col].byte_data,bit))
                BIT_SET(&serialized_buffer[state_col].byte_data,state_bit_position);
            else
                BIT_CLEAR(&serialized_buffer[state_col].byte_data,state_bit_position);
    #else
            int bit = (block_index) % BYTE_SIZE ;
            
            if(BIT_CHECK(&this->block_state_mat_update[row][0].byte_data,bit))
                BIT_SET(&serialized_buffer[state_col].byte_data,state_bit_position);
            else
                BIT_CLEAR(&serialized_buffer[state_col].byte_data,state_bit_position);
    #endif
        }
#endif
    }
    end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

    printf("4.  Sending data to client...");    
    
    start = time_now;
    socket.send(serialized_buffer,serialized_buffer_len,0);
    end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

    delete[] serialized_buffer;
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    delete dsse;
    return 0 ; 
}

/**
 * Function Name: getEncrypted_data_structure
 *
 * Description:
 * Process the encrypted data structure block data sent by the client
 *
 * @param socket: (output) opening socket
 * @return	0 if successful
 */
int Server_DSSE::getEncrypted_data_structure(zmq::socket_t& socket)
{
    Miscellaneous misc;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    int len;
    len = 0;
    FILE* foutput = NULL;
    size_t size_received = 0 ;
    size_t file_in_size;
    
    int64_t more;
    size_t more_size = sizeof(more);
    
    
    #if !defined(DISK_STORAGE_MODE)
        DSSE dsse;
    #endif
    
    printf("1. Receiving file name....");
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    string filename((char*)buffer_in);
    printf("OK!\t\t\t %s \n",filename.c_str());
    string filename_with_path = gcsDataStructureFilepath + filename;
    
    printf("2. Opening the file...");
    if((foutput =fopen(filename_with_path.c_str(),"wb+"))==NULL)
    {
        printf("Error!!\n");
        exit(1);
    }
    printf("OK!\n");
    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
    
    printf("3. Receiving file content");
    
    // Receive the file size in bytes first
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    
    memcpy(&file_in_size,buffer_in,sizeof(size_t));
    printf(" of size %zu bytes...",file_in_size);
    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
    
    // Receive the file content
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    size_received = 0;
    while(size_received<file_in_size)
    {
        len = socket.recv(buffer_in,SOCKET_BUFFER_SIZE,0);
        if(len == 0)
            break;
        size_received += len;
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
    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
    
        
    if (filename.compare(FILENAME_BLOCK_COUNTER_ARRAY)==0)
    {
        
    #if !defined(DISK_STORAGE_MODE)
        dsse.loadEncrypted_matrix_from_files(this->I);
      #if !defined(DECRYPT_AT_CLIENT_SIDE)
        dsse.loadBlock_state_matrix_from_file(this->block_state_mat);
      #endif
    #endif
    
        misc.read_array_from_file(filename,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
    }
    printf("OK!\n\t\t %zu bytes received\n",size_received);
    
    printf("4. Updating memory...");
    
    printf("OK!\n");

    return 0;
}


/**
 * Function Name: searchKeyword
 *
 * Description:
 * Process the search keyword request from client
 *
 * @param socket: (output) opening socket
 * @return	0 if successful
 */
int Server_DSSE::searchKeyword(zmq::socket_t &socket)
{
    DSSE* dsse = new DSSE();
    TYPE_COUNTER search_result_num = 0;
    SearchToken tau;
    vector<TYPE_INDEX> lstFile_id;
auto start = time_now;
auto end = time_now;

    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    
#if defined(SEND_SEARCH_FILE_INDEX)
    FILE* finput;
    string filename_search_result = gcsDataStructureFilepath + FILENAME_SEARCH_RESULT;
    Miscellaneous misc;
    size_t filesize;
    int n ;
#endif
    start = time_now;
    // Receive the SearchToken data from the client
    printf("1. Receiving Search Token...");
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    memcpy(&tau,&buffer_in,sizeof(SearchToken));
    end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
      
    // Perform the search 
    printf("2. Searching....");
    start = time_now;
    lstFile_id.clear();
#if defined(DISK_STORAGE_MODE)
    this->loadData_from_file(ROW,tau.row_index);
       
    TYPE_INDEX tmp = tau.row_index;
    tau.row_index = 0;
    
    if(dsse->search(lstFile_id,tau,this->I_search,
                        this->block_counter_arr,
                        this->block_state_mat_search)!=0)
    {
        printf("Error!!\n");
        exit(1);
    }
    
#else
    if(dsse->search( lstFile_id,tau,this->I,
                    this->block_counter_arr,this->block_state_mat))
    {
        printf("Error!!\n");
        exit(1);
    }
#endif
end = time_now;
cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;


    search_result_num = lstFile_id.size();
    
    /* Send the number of files in that the keyword exists to the server */
    printf("3. Sending the result back...");
start = time_now;
#if defined(SEND_SEARCH_FILE_INDEX)
    
    misc.write_list_to_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
    
    if((finput = fopen(filename_search_result.c_str(),"rb"))==NULL)
    {
        printf("Error!!\n");
        exit(1);
    }
    if((filesize = lseek(fileno(finput),0,SEEK_END))<0)
    {
        printf("Error!!\n");
        exit(1);
    }
    if(fseek(finput,0,SEEK_SET)<0)
    {
        printf("Error!!\n");
        exit(1);
    }   
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    memcpy(buffer_out,&filesize,sizeof(size_t));
    socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
    
    unsigned char* data_out = new unsigned char[filesize];
    fread(data_out,11,filesize,finput);
    socket.send(data_out,filesize,0);
    fclose(finput);

#else       //just send the number of files having this keyword
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    memcpy(buffer_out,&search_result_num,sizeof(search_result_num));
    socket.send(buffer_out,SOCKET_BUFFER_SIZE);

#endif
    end = time_now;
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;

#if defined(DISK_STORAGE_MODE)
    start = time_now;
    printf("4. Updating local data...");
    saveData_to_file(ROW,tmp);
    end = time_now; 
    cout<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;  
    
#endif


    delete dsse ;
    lstFile_id.clear();
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    
    return 0;
}

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
int Server_DSSE::loadData_from_file(int dim, TYPE_INDEX idx)
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
int Server_DSSE::saveData_to_file(int dim, TYPE_INDEX idx)
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

