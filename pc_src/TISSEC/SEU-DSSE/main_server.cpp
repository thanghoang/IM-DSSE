#include <DSSE_Param.h>

#include <Server_DSSE.h>

int main(int argc, char ** argv)
{
    
    Server_DSSE* server_dsse = new Server_DSSE();
    server_dsse->start();
    
    return 0;
}


