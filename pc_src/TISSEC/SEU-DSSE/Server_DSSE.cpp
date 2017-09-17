
#include "Server_DSSE.h"
#include "DSSE_Param.h"
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
#if !defined(LOAD_FROM_DISK)
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
    int ret;
    const int CMD_SAVE = 123456;
    unsigned char buffer[SOCKET_BUFFER_SIZE];
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REP);
    socket.bind("tcp://*:"+SERVER_PORT);
#if defined(UPLOAD_DATA_STRUCTURE_MANUALLY_MODE) && !defined(LOAD_FROM_DISK)
    printf("Loading generated data structure....");
    DSSE* dsse = new DSSE();
    dsse->loadEncrypted_matrix_from_files(this->I);
#if !defined(DECRYPT_AT_CLIENT_SIDE)
    printf("OK!\n");
    printf("Loading block state matrix...");
    //dsse->loadBlock_state_matrix_from_file(this->block_state_mat);
#endif
    delete dsse;
    
    printf("OK!\n");
#endif
    
    do
    {
//        printf("Waiting for request......\n\n");
        while(!socket.connected());
        
        /* 1. Read the command sent by the client to determine the job */
        //socket.recv(buffer,SOCKET_BUFFER_SIZE,ZMQ_RCVBUF);
        socket.recv(buffer,SOCKET_BUFFER_SIZE);
        
        int cmd;
        memcpy(&cmd,buffer,sizeof(cmd));
//        printf("REQUESTED......");
        //socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        
        switch(cmd)
        {
        case CMD_SEND_DATA_STRUCTURE:
//            printf("\"BUILD DATA STRUCTURE!\"\n");
//            if(this->getEncrypted_data_structure(socket)==0)
            this->getEncrypted_data_structure(socket);
            
//                printf("\nDone!\n\n");
//            else
//                printf("\nError!\n\n");
            break;
        case CMD_ADD_FILE_PHYSICAL:
//            printf("\"ADD PHYSICAL ENCRYPTED FILE!\n");
//            if(this->getEncrypted_file(socket)==0)
            this->getEncrypted_file(socket);

//                printf("\nDone!\n\n");
//            else
//                printf("\nError!\n\n");
            break;
        case CMD_SEARCH_OPERATION:
//            printf("\"SEARCH!\"\n");
//            if(this->searchKeyword(socket)==0)
                this->searchKeyword(socket);
//                printf("\nDone!\n\n");
//            else
//                printf("\nError!\n\n");
            break;
        case CMD_REQUEST_BLOCK_DATA:
//            printf("\"GET BLOCK DATA!\"\n");
            this->getBlock_data(socket,COL);
//            if(this->getBlock_data(socket,COL)==0)
//                printf("\nDone!\n\n");
//            else
//                printf("\nError!\n\n");
            break;
        case CMD_UPDATE_BLOCK_DATA:
//            printf("\"UPDATE DATA STRUCTURE!\"\n");
                this->updateBlock_data(socket);
//            if(this->updateBlock_data(socket)==0)
//                printf("\nDone!\n\n");
//            else
//                printf("\nError!\n\n");
            break;
        case CMD_DELETE_FILE_PHYSICAL:
//            printf("\"DELETE FILE PHYSICAL!\"\n");
this->deleteFile(socket);
//            if(this->deleteFile(socket)==0)
//                printf("\nDone!\n\n");
//            else
//                printf("\nError!\n\n");
            break;
        case CMD_REQUEST_SEARCH_DATA:
//            printf("\"REQUEST SEARCH DATA!\"\n");
//            if(this->getBlock_data(socket,ROW)==0)
this->getBlock_data(socket,ROW);
//                printf("\nDone!\n\n");
//            else
//                printf("\nError!\n\n");
            break;
        case CMD_SAVE:
            double res[16];
            for(int i = 0 ; i < 16; i++)
            {
                res[i] = 0.0 ;
                for(int j = 0 ; j < server_stats[i].size(); j++)
                {
                    res[i] += server_stats[i][j] ;
                }
                res[i] = res[i]/server_stats[i].size();
                cout<<i<<" : " <<res[i]<<endl;
            }
            //write to file
            
            //clear the content
            for(int i = 0 ; i < 16 ; i++)
            {
                server_stats[i].clear();
            }
            socket.send(CMD_SUCCESS,SOCKET_BUFFER_SIZE);
            break;
        default:
            break;
        }
        
    }while(1);

//    printf("Server ended \n");
    ret = 0;

    memset(buffer,0,SOCKET_BUFFER_SIZE);
    return ret;
}

/**
 * Function Name: deleteFile
 *
 * Description:
 * Process the file deletion (in file collection) request from client
 *
 * @param socket: (output) opening socket
 * @return	0 if successful
 */
int Server_DSSE::deleteFile(zmq::socket_t& socket)
{
    Miscellaneous misc;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    int ret;
    FILE* foutput = NULL;
    stringstream filename_with_path;
    TYPE_INDEX file_index;
    
    /* Receive the file index sent by the client */
    printf("1. Receiving file index....");
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    
    memcpy(&file_index,buffer_in,sizeof(file_index));
    printf("OK!\n-- Received index: %lu \n",file_index);
    
    printf("2. Deleting the file...");
    filename_with_path<<gcsEncFilepath <<"encTar" <<file_index <<".tar.gz";
    if(remove(filename_with_path.str().c_str())!=0)
        printf("Error! File not found...\n");
    //socket.send((unsigned char*) CMD_SUCCESS,sizeof(CMD_SUCCESS));
    printf("OK!\n");
    
    ret = 0;
    
    delete foutput;
    filename_with_path.clear();
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);

    return ret;
    
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
    int ret,len;
    
    int64_t more;
    size_t more_size = sizeof(more);
    TYPE_INDEX block_index;
    
    off_t offset;
    TYPE_INDEX n = 0;
        TYPE_COUNTER* null_ptr = NULL;
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE;
    MatrixType* serialized_buffer = new MatrixType[serialized_buffer_len]; //consist of data 
   start = time_now; 
//    printf("1.  Receiving block index....");
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE,ZMQ_RCVMORE);
    
//    printf("OK!\n"); 
           
    memcpy(&block_index,buffer_in,sizeof(block_index));
    //socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
end = time_now;
//cout<<"Get Block index time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"1: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[0].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
    /* Receive block data sent by the client */
    offset = 0;
    memset(serialized_buffer,0,serialized_buffer_len);
start = time_now;
//    printf("2.  Receiving block data....");
    socket.recv(serialized_buffer,serialized_buffer_len);
//    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));

//    printf("OK!\n");    
end = time_now;
//cout<<"Receive data time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"2: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[1].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
    
    /* Update the received I' */
//    printf("4. Calling Update function...");

#if !defined(LOAD_FROM_DISK)
start = time_now;
    if((ret = dsse->update(serialized_buffer,block_index,this->I,this->block_counter_arr,this->block_state_mat))!=0)
    {
//        printf("Failed!\n");
        goto exit;
    }
end = time_now;
//cout<<"Update ds time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"3: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[2].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       

#else
    //load data from disk if block size is 1
    if(ENCRYPT_BLOCK_SIZE==1)
    {
        start = time_now;
        this->loadData_from_file(COL,block_index);
        end = time_now;
//        cout<<"load Data from disk time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//        cout<<"4: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    server_stats[3].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       

    }
    start = time_now;
    TYPE_INDEX idx = 0;
    if(ENCRYPT_BLOCK_SIZE < BYTE_SIZE)
        idx = block_index % (BYTE_SIZE / ENCRYPT_BLOCK_SIZE);
//    cout<<"idx: "<<idx<<endl;
    if((ret = dsse->update(serialized_buffer,
                    idx,
                    this->I_update,
                    NULL,NULL)) !=0)
        {
//            printf("Failed!\n");
        }
 #if !defined(DECRYPT_AT_CLIENT_SIDE)
    this->block_counter_arr[block_index]+=1;

    int bit = (block_index) % BYTE_SIZE ;
    for(TYPE_INDEX row = 0 ; row < MATRIX_ROW_SIZE; row++)
    {
        BIT_SET(&this->block_state_mat_update[row][0].byte_data,bit);
    }
  #endif
end = time_now;
//cout<<"Update data time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"5: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[4].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       

//    printf("5. Saving new data to files...");
start = time_now;
    saveData_to_file(COL,block_index);
end = time_now;
//cout<<"Save updated data to disk time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"6: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[5].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       

#endif
//    printf("OK!\n");
socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS)); //Temp use
    
exit:
    delete[] serialized_buffer;
    delete dsse;
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    return ret ; 
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
    int ret = 0;
    
    off_t offset;
    TYPE_INDEX n;
    
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
//    printf("1.  Receiving block index requested....");
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
//    printf("OK!\n");
end = time_now;
//cout<<"Receive block index time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"7: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[6].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       

    memcpy(&block_index,buffer_in,sizeof(block_index));
//    cout<<"BLOCK INDEX:  "<<block_index<<endl;;
    
    /* Get the block data requested by the client */
    
#if defined(LOAD_FROM_DISK)
start = time_now;
    this->loadData_from_file(dim,block_index);
end = time_now;
//cout<<"load Data from disk time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"8: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[7].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
          
    TYPE_INDEX idx = 0;
        if(ENCRYPT_BLOCK_SIZE < BYTE_SIZE)
            idx = block_index % (BYTE_SIZE / ENCRYPT_BLOCK_SIZE);
start = time_now;
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
//cout<<"Get Block data locally time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"9: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[8].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
        
start = time_now;
    if(dim == COL)
    {
#if !defined(DECRYPT_AT_CLIENT_SIDE)
        TYPE_INDEX row,ii,state_col,state_bit_position;
        for(row = 0, ii = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE) ; row < MATRIX_ROW_SIZE ; row++, ii++)
        {
            state_col = ii / BYTE_SIZE;
            state_bit_position = ii % BYTE_SIZE;
    #if !defined(LOAD_FROM_DISK)
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
//cout<<"Serialize data time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"10: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[9].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       

start = time_now;
    /* Send block data requested by the client */
    socket.send(serialized_buffer,serialized_buffer_len,0);
    ret = 0 ;
end = time_now;
//cout<<"Send serialized data time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"11: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[10].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
        
exit:
    delete[] serialized_buffer;
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    delete dsse;
    return ret ; 
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
    int ret, len;
    len = 0;
    FILE* foutput = NULL;
    size_t size_received = 0 ;
    size_t file_in_size;
    
    int64_t more;
    size_t more_size = sizeof(more);
    
    
    #if !defined(LOAD_FROM_DISK)
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
        printf("Error! File opened failed!\n");
        ret = FILE_OPEN_ERR;
        goto exit;
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
        
    #if !defined(LOAD_FROM_DISK)
        dsse.loadEncrypted_matrix_from_files(this->I);
        #if !defined(DECRYPT_AT_CLIENT_SIDE)/*
            for(TYPE_INDEX row = 0 ; row < MATRIX_ROW_SIZE; row++)
            {
                memset(this->block_state_mat[row],ZERO_VALUE,NUM_BLOCKS);
            }*/
            dsse.loadBlock_state_matrix_from_file(this->block_state_mat);
        #endif
    #endif
    
        misc.read_array_from_file(filename,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
    }
    printf("OK!\n\t\t %zu bytes received\n",size_received);
    
    printf("4. Updating memory...");
    
    printf("OK!\n");
    ret = 0;

exit:
    return ret ;
}
int Server_DSSE::updateBigMatrix_from_piece(MatrixType** I_big, MatrixType** I_piece, int col_pos)
{
    int n; 
    TYPE_INDEX col, row, I_big_col_idx;
    Miscellaneous misc;
    n = MATRIX_COL_SIZE/MATRIX_PIECE_COL_SIZE;
    for(col = 0; col < MATRIX_PIECE_COL_SIZE; col++)
    {
        I_big_col_idx = col+ (col_pos*MATRIX_PIECE_COL_SIZE);
        for(row = 0 ; row < MATRIX_ROW_SIZE ; row++)
        {
            I_big[row][I_big_col_idx].byte_data = I_piece[row][col].byte_data;
        }
    }
}

/**
 * Function Name: getEncrypted_file
 *
 * Description:
 * Process the encrypted files in file collectionsent by the client
 *
 * @param socket: (output) opening socket
 * @return	0 if successful
 */
int Server_DSSE::getEncrypted_file(zmq::socket_t& socket)
{
    Miscellaneous misc;
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    int ret, len;
    len = 0;
    FILE* foutput = NULL;
    size_t size_received = 0 ;
    size_t file_in_size;
    
    int64_t more;
    size_t more_size = sizeof(more);
    
    printf("1. Receiving file name....");
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    string filename((char*)buffer_in);
    printf("OK!\t\t\t %s \n",filename.c_str());
    string filename_with_path = gcsEncFilepath + filename;
    
    printf("2. Opening the file...");
    if((foutput =fopen(filename_with_path.c_str(),"wb+"))==NULL)
    {
        printf("Error! File opened failed!\n");
        ret = FILE_OPEN_ERR;
        goto exit;
    }
    printf("OK!\n");
    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
 
    /* Receive file content sent by client, write it to the storage */
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
        if(len ==0)
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
    printf("OK!\n\t\t %zu bytes received \n",size_received);
    ret = 0;
    
exit:
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    filename_with_path.clear();
    filename.clear();    
    return ret;
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
    size_t filesize, offset, size_sent;
    int n ;
#endif

    int ret = 0 ;
    start = time_now;
    /* Receive the SearchToken data from the client */
//    printf("1. Receiving Search Token...");
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    memcpy(&tau,&buffer_in,sizeof(SearchToken));
//    printf("OK!\n");
//    printf("-- Index received: %lu \n",tau.row_index);
    end = time_now;
//    cout<<"Receive search token time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"12: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
  server_stats[11].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
      
    /* Perform the search */
//    printf("2. Doing local search....");
    lstFile_id.clear();
#if defined(LOAD_FROM_DISK)
    start = time_now;
    this->loadData_from_file(ROW,tau.row_index);
    end = time_now;
//    cout<<"Load ds from disk time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"13: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
 server_stats[12].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
       
    TYPE_INDEX tmp = tau.row_index;
    tau.row_index = 0;
    start = time_now;
    if((ret=dsse->search(lstFile_id,tau,this->I_search,
                        this->block_counter_arr,
                        this->block_state_mat_search))!=0)
    {
//        printf("Error during performing local search...\n");
        goto exit;
    }
    end = time_now;
//    cout<<"Load search time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"14: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[13].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
    
#else
start = time_now;
    if((ret = dsse->search( lstFile_id,tau,this->I,
                    this->block_counter_arr,this->block_state_mat))!=0)
    {
//        printf("Error!");
        goto exit;
    }
end = time_now;
//cout<<"search time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"14: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[13].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       

#endif


//    printf("OK!\n");
    search_result_num = lstFile_id.size();
    
    /* Send the number of files in that the keyword exists to the server */
//    printf("\nKeyword requested appears in %lu files \n", search_result_num );  
//    printf("3. Sending the result back...");
start = time_now;
#if defined(SEND_SEARCH_FILE_INDEX)
    
//    printf("\n   3.1. Write result to file...");
    misc.write_list_to_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
//    printf("OK!\n");
    
//    printf("   3.2 Opening result file...");
    if((finput = fopen(filename_search_result.c_str(),"rb"))==NULL)
    {
//        printf("Error! File not found\n");
        ret = FILE_OPEN_ERR;
        goto exit;
    }
    if((filesize = lseek(fileno(finput),0,SEEK_END))<0)
    {
//        perror("lseek");
        ret = FILE_OPEN_ERR;
        goto exit;
    }
    if(fseek(finput,0,SEEK_SET)<0)
    {
//        printf("fseek(0,SEEK_SET) failed \n");
        ret = FILE_OPEN_ERR;
        goto exit;
    }
//    printf("OK!\n");
//    printf("   3.3. Sending file content...");
    size_sent = 0;
    
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    memcpy(buffer_out,&filesize,sizeof(size_t));
    socket.send(buffer_out,SOCKET_BUFFER_SIZE,ZMQ_SNDMORE);
    //socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    
    // Send the file content
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    for(offset = 0 ; offset < filesize; offset +=SOCKET_BUFFER_SIZE)
    {
        n = (filesize - offset > SOCKET_BUFFER_SIZE) ? SOCKET_BUFFER_SIZE : (int) 
            (filesize - offset);
        if(fread(buffer_out,1,n,finput)!=(size_t)n)
        {
            printf("Read input file error at block %d",n);
            break;
        }
        
        if(offset+n == filesize)
            socket.send(buffer_out,n,0);
        else
            socket.send(buffer_out,n,ZMQ_SNDMORE);
        size_sent +=n;
    }
    printf("OK!\n\t\t %zu bytes sent \n",size_sent);
    fclose(finput);

#else       //just send the number of files having this keyword
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    memcpy(buffer_out,&search_result_num,sizeof(search_result_num));
    socket.send(buffer_out,SOCKET_BUFFER_SIZE);

#endif
end = time_now;
//cout<<"Send search result time : "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//cout<<"15: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
server_stats[14].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       

#if defined(LOAD_FROM_DISK)
start = time_now;
//    printf("3. Saving new data to files...");
    saveData_to_file(ROW,tmp);
end = time_now;
//cout<<"Save data to disk time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
//    cout<<"16: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
  server_stats[15].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());       
  
#endif

//    printf("\n---------------------------\n");
    ret = 0;
    
exit:
    delete dsse ;
    lstFile_id.clear();
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    
    return ret;
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
#if defined(LOAD_FROM_DISK)
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


#endif