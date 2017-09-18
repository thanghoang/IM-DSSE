#include <DSSE_Param.h>

#include <Server_DSSE.h>

int main(int argc, char ** argv)
{
    setbuf(stdout,NULL);
    Server_DSSE* server_dsse = new Server_DSSE();
    server_dsse->start();
    
    return 0;
}


