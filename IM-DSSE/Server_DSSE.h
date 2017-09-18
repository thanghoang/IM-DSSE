#ifndef SERVER_DSSE_H
#define SERVER_DSSE_H

#include <MasterKey.h>
#include <config.h>
#include <struct_MatrixType.h>

#include <zmq.hpp>
using namespace zmq;
class Server_DSSE
{
private:
    
    
    // Encrypted index
    MatrixType** I;
	

    TYPE_COUNTER* block_counter_arr;
    MatrixType** block_state_mat;

    MatrixType** I_search;
    MatrixType** I_update;
    
    MatrixType** block_state_mat_search;
    MatrixType** block_state_mat_update;

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


    int loadState();
    int saveState();
    
    
    int loadData_from_file(int dim, TYPE_INDEX idx);
    int saveData_to_file(int dim, TYPE_INDEX idx);

};

#endif // SERVER_DSSE_H
