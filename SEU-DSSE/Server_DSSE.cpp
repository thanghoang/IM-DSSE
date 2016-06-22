
#include "Server_DSSE.h"
#include "DSSE_Param.h"
#include "DSSE_KeyGen.h"

#include "Miscellaneous.h"

#include "DSSE.h"
#include "zmq.hpp"
#include <sys/socket.h>
#include <sys/types.h>
using namespace zmq;
TYPE_COUNTER Server_DSSE::gc = 1;


Server_DSSE::Server_DSSE()
{
    TYPE_INDEX i;
    
    /* Allocate memory for data structure (matrix) */
    this->I = new MatrixType *[MATRIX_ROW_SIZE];
    for (i = 0; i < MATRIX_ROW_SIZE;i++ )
    {
        this->I[i] = new MatrixType[MATRIX_COL_SIZE];
    }
    /* Allocate memory for state matrix */
    this->block_state_mat = new bool *[MATRIX_ROW_SIZE];
    for(i = 0 ; i < MATRIX_ROW_SIZE; i++)
    {
        this->block_state_mat[i] = new bool [NUM_BLOCKS];
        memset(this->block_state_mat[i],ZERO_VALUE,NUM_BLOCKS);
    }
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
    unsigned char buffer[SOCKET_BUFFER_SIZE];
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REP);
    
    socket.bind(PEER_ADDRESS);
#if defined(UPLOAD_DATA_STRUCTURE_MANUALLY_MODE)
    printf("Loading generated data structure....");
    DSSE* dsse = new DSSE();
    dsse->loadEncrypted_matrix_from_files(this->I);
    printf("OK!\n");
#endif
    
    do
    {
        printf("Waiting for request......\n\n");
        while(!socket.connected());
        
        /* 1. Read the command sent by the client to determine the job */
        socket.recv(buffer,SOCKET_BUFFER_SIZE,ZMQ_RCVBUF);
        
        int cmd;
        memcpy(&cmd,buffer,sizeof(cmd));
        printf("REQUESTED......");
        socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        
        switch(cmd)
        {
        case CMD_SEND_DATA_STRUCTURE:
            printf("\"BUILD DATA STRUCTURE!\"\n");
            if(this->getEncrypted_data_structure(socket)==0)
                printf("\nDone!\n\n");
            else
                printf("\nError!\n\n");
            break;
        case CMD_ADD_FILE_PHYSICAL:
            printf("\"ADD PHYSICAL ENCRYPTED FILE!\n");
            if(this->getEncrypted_file(socket)==0)
                printf("\nDone!\n\n");
            else
                printf("\nError!\n\n");
            break;
        case CMD_SEARCH_OPERATION:
            printf("\"SEARCH!\"\n");
            if(this->searchKeyword(socket)==0)
                printf("\nDone!\n\n");
            else
                printf("\nError!\n\n");
            break;
        case CMD_REQUEST_BLOCK_DATA:
            printf("\"GET BLOCK DATA!\"\n");
            if(this->getBlock_data(socket)==0)
                printf("\nDone!\n\n");
            else
                printf("\nError!\n\n");
            break;
        case CMD_UPDATE_BLOCK_DATA:
            printf("\"UPDATE DATA STRUCTURE!\"\n");
            if(this->updateBlock_data(socket)==0)
                printf("\nDone!\n\n");
            else
                printf("\nError!\n\n");
            break;
        case CMD_DELETE_FILE_PHYSICAL:
            printf("\"DELETE FILE PHYSICAL!\"\n");
            if(this->deleteFile(socket)==0)
                printf("\nDone!\n\n");
            else
                printf("\nError!\n\n");
            break;
        default:
            break;
        }
        
    }while(1);

    printf("Server ended \n");
    ret = 0;

    memset(buffer,0,SOCKET_BUFFER_SIZE);
    return ret;
}

/**
 * Function Name: searchKeyword
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
    socket.send((unsigned char*) CMD_SUCCESS,sizeof(CMD_SUCCESS));
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
    
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE;
    MatrixType serialized_buffer[serialized_buffer_len]; //consist of data and block state
    
    printf("1.  Receiving block index....");
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    
    printf("OK!\n");    
    memcpy(&block_index,buffer_in,sizeof(block_index));
    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        
    /* Receive block data sent by the client */
    offset = 0;
    memset(serialized_buffer,0,serialized_buffer_len);
    while(offset<serialized_buffer_len)
    {
        len=socket.recv(&serialized_buffer[offset],SOCKET_BUFFER_SIZE,0);
        offset+=len;
        socket.getsockopt(ZMQ_RCVMORE,&more,&more_size);
        if(!more)
            break;
    }
    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
    
    /* Update the received I' */
    printf("4. Calling Update function...");
    if((ret = dsse->update(serialized_buffer,block_index,this->I,this->block_counter_arr,this->block_state_mat,this->gc))!=0)
    {
        printf("Failed!\n");
        goto exit;
    }
    printf("OK!\n");
    
exit:
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
int Server_DSSE::getBlock_data(zmq::socket_t & socket)
{
    Miscellaneous misc;
    DSSE *dsse = new DSSE();
   
    unsigned char buffer_in[SOCKET_BUFFER_SIZE] = {'\0'};
    unsigned char buffer_out[SOCKET_BUFFER_SIZE] = {'\0'};
    int ret = 0;
    
    off_t offset;
    TYPE_INDEX n;
    
    TYPE_INDEX block_index;
   
    TYPE_INDEX serialized_buffer_len = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE)/BYTE_SIZE+(MATRIX_ROW_SIZE/BYTE_SIZE);
    
    MatrixType serialized_buffer[serialized_buffer_len]; //consist of data and block state
    memset(serialized_buffer,0,serialized_buffer_len);
    
    printf("1.  Receiving block index requested....");
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    printf("OK!\n");
    
    memcpy(&block_index,buffer_in,sizeof(block_index));
    
    /* Get the block data and state array requested by the client (LOCAL PROCESSING) */
    dsse->fetchBlock_data(block_index,serialized_buffer,this->I);
    
    TYPE_INDEX row,ii,state_col,state_bit_position;
    for(row = 0, ii = (MATRIX_ROW_SIZE*ENCRYPT_BLOCK_SIZE) ; row < MATRIX_ROW_SIZE ; row++, ii++)
    {
        state_col = ii / BYTE_SIZE;
        state_bit_position = ii % BYTE_SIZE;
        if(this->block_state_mat[row][block_index]==ONE_VALUE)
            BIT_SET(&serialized_buffer[state_col].byte_data,state_bit_position);
    }
    
    /* Send block data requested by the client */
    for(offset = 0 ; offset < serialized_buffer_len; offset +=SOCKET_BUFFER_SIZE)
    {
        n = (serialized_buffer_len - offset > SOCKET_BUFFER_SIZE) ? SOCKET_BUFFER_SIZE : (int) 
            (serialized_buffer_len - offset);
            if(offset + n == serialized_buffer_len)
                socket.send(&serialized_buffer[offset],n,0);
            else
                socket.send(&serialized_buffer[offset],n,ZMQ_SNDMORE);
    }
    ret = 0 ;
exit:
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
    
    printf("OK!\n\t\t %zu bytes received\n",size_received);
    
    printf("4. Updating memory...");
    if(filename.compare(FILENAME_BLOCK_STATE_MATRIX)==0)
    {
        for(TYPE_INDEX row = 0 ; row < MATRIX_ROW_SIZE; row++)
        {
            memset(this->block_state_mat[row],ZERO_VALUE,NUM_BLOCKS);
        }
    }
    else if (filename.compare(FILENAME_BLOCK_COUNTER_ARRAY)==0)
    {
        misc.read_array_from_file(filename,gcsDataStructureFilepath,this->block_counter_arr,NUM_BLOCKS);
    }
    else if (filename.compare(FILENAME_GLOBAL_COUNTER)==0)
    {
        misc.read_counter_from_file(filename,gcsDataStructureFilepath,this->gc);
    }
    else //update pieces of the big encrypted data structure
    {
        MatrixType** I_piece = new MatrixType*[MATRIX_ROW_SIZE];
        int i = stoi(filename);
        for(TYPE_INDEX row = 0 ; row < MATRIX_ROW_SIZE; row++)
        {
            I_piece[row] = new MatrixType[MATRIX_PIECE_COL_SIZE];
            memset(I_piece[row],0,MATRIX_PIECE_COL_SIZE);
        }
        misc.read_matrix_from_file(filename,gcsDataStructureFilepath,I_piece,MATRIX_ROW_SIZE,MATRIX_PIECE_COL_SIZE);
        this->updateBigMatrix_from_piece(this->I,I_piece,i);
    }
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
    
    /* Receive the SearchToken data from the client */
    printf("1. Receiving Search Token...");
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    memcpy(&tau,&buffer_in,sizeof(SearchToken));
    printf("OK!\n");
    printf("-- Index received: %lu \n",tau.row_index);
    
    /* Perform the search */
    printf("2. Doing local search....");
    lstFile_id.clear();
    if((ret = dsse->search( lstFile_id,tau,this->I,
                    this->block_counter_arr,this->block_state_mat))!=0)
    {
        printf("Error!");
        goto exit;
    }
    printf("OK!\n");
    search_result_num = lstFile_id.size();
    
    /* Send the number of files in that the keyword exists to the server */
    printf("\nKeyword requested appears in %lu files \n", search_result_num );  
    printf("3. Sending the result back...");
    
#if defined(SEND_SEARCH_FILE_INDEX)
    
    printf("\n   3.1. Write result to file...");
    misc.write_list_to_file(FILENAME_SEARCH_RESULT,gcsDataStructureFilepath,lstFile_id);
    printf("OK!\n");
    
    printf("   3.2 Opening result file...");
    if((finput = fopen(filename_search_result.c_str(),"rb"))==NULL)
    {
        printf("Error! File not found\n");
        ret = FILE_OPEN_ERR;
        goto exit;
    }
    if((filesize = lseek(fileno(finput),0,SEEK_END))<0)
    {
        perror("lseek");
        ret = FILE_OPEN_ERR;
        goto exit;
    }
    if(fseek(finput,0,SEEK_SET)<0)
    {
        printf("fseek(0,SEEK_SET) failed \n");
        ret = FILE_OPEN_ERR;
        goto exit;
    }
    printf("OK!\n");
    printf("   3.3. Sending file content...");
    size_sent = 0;
    
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    memcpy(buffer_out,&filesize,sizeof(size_t));
    socket.send(buffer_out,SOCKET_BUFFER_SIZE);
    socket.recv(buffer_in,SOCKET_BUFFER_SIZE);
    
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
    printf("\n---------------------------\n");
    ret = 0;
    
exit:
    delete dsse ;
    lstFile_id.clear();
    memset(buffer_in,0,SOCKET_BUFFER_SIZE);
    memset(buffer_out,0,SOCKET_BUFFER_SIZE);
    
    return ret;
 }
