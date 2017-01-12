#ifndef SERVER_DSSE_H
#define SERVER_DSSE_H

#include <MasterKey.h>
#include <DSSE_Param.h>
#include <struct_MatrixType.h>

#include <zmq.hpp>
using namespace zmq;
class Server_DSSE
{
private:
    
    // Global & static file counter
    //TYPE_COUNTER gc;

    //Data Structure 
    MatrixType** I;
	

    TYPE_COUNTER keyword_counter_arr[MAX_NUM_KEYWORDS];
    TYPE_COUNTER block_counter_arr[NUM_BLOCKS];
    MatrixType** block_state_mat;

    int updateBigMatrix_from_piece(MatrixType** I_big, MatrixType** I_piece, int col_pos);
    
#if defined (LOAD_FROM_DISK)
    MatrixType** I_search;
    MatrixType** I_update;
    
    MatrixType** block_state_mat_search;
    MatrixType** block_state_mat_update;
#endif

public:
    Server_DSSE();
    ~Server_DSSE();

    int getBlock_data(zmq::socket_t &socket, int dim);
    int updateBlock_data(zmq::socket_t &socket);
    int getEncrypted_data_structure(zmq::socket_t &socket);
    int getEncrypted_file(zmq::socket_t &socket);
    int searchKeyword(zmq::socket_t &socket);
    int deleteFile(zmq::socket_t &socket);
    
    
    int start();

#if defined(LOAD_FROM_DISK)
    int loadData_from_file(int dim, TYPE_INDEX idx);
    int saveData_to_file(int dim, TYPE_INDEX idx);
#endif
    vector<unsigned long int> server_stats[16];

};

#endif // SERVER_DSSE_H
