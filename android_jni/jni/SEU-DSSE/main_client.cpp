#include <stdio.h>

#include <Client_DSSE.h>
#include <DSSE_Param.h>
#include <Miscellaneous.h>
#include <unistd.h>
#include "DSSE.h"
#include <fstream>
#include "zmq.hpp"
#include <limits>
using namespace std;
int nTime = 100;
int nUpdateTime  = 50;
vector<TYPE_COUNTER> gt;

void printMenu();
typedef std::numeric_limits<double> dbl;
string exec(const char* cmd);

bool fexists(string filename_with_path)
{
  ifstream ifile(filename_with_path.c_str());
  if(!ifile.is_open())
      return false;
  return true;
}
void reset(int dim)
{
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    int CMD_SAVE = 123455;
    
    double res[15];
    for(int i = 0 ; i < 15; i++)
    {
        if(Client_DSSE::stats[i].size()>0)
	{
		Client_DSSE::stats[i].erase(Client_DSSE::stats[i].begin());
	}   
	res[i] = 0.0 ;
        for(int j = 0 ; j < Client_DSSE::stats[i].size(); j++)
        {
            res[i] += Client_DSSE::stats[i][j] ;
        }
	if(Client_DSSE::stats[i].size()>0)
	{
	    res[i] = res[i]/(Client_DSSE::stats[i].size());
	}
        //cout<<i<<" : "<<res[i]<<endl;
	
	cout.precision(dbl::max_digits10);
	
	cout<<fixed<<res[i]<<endl;
	
    }
    //write to file (incld. res);
string filename = PEER_ADDRESS;
filename.erase(0,5);
filename+="_" + noFile;      
    if (dim == ROW)
        filename+="_search";
    else
        filename +="_update";
#if defined (VARIANT_MAIN)
    filename+="_MAIN";
#elif defined(VARIANT_I)
    filename+="_I";
#elif defined(VARIANT_II)
    filename+="_II";
#elif defined(VARIANT_III)
    filename+="_III";
#endif
#if defined (LOAD_FROM_DISK)
    filename+="_DISKLOAD";
#endif

    for(TYPE_INDEX i = 0 ; i < 15; i ++)
    {
        string str = filename +"_" + Miscellaneous::to_string(i);
        Miscellaneous::write_list_to_file(str,gcsDataStructureFilepath,Client_DSSE::stats[i]);
    }
    string stats = filename+"_ClientStats";
    Miscellaneous::write_array_to_file(stats,gcsDataStructureFilepath,res, 15);
    
    
    //clear the content
    for(int i = 0 ; i < 15;i++)
    {
        Client_DSSE::stats[i].clear();
    }
    unsigned char buf[SOCKET_BUFFER_SIZE];
    socket.connect(PEER_ADDRESS);
    memset(buf,0,SOCKET_BUFFER_SIZE);
    memcpy(buf,&CMD_SAVE,sizeof(CMD_SAVE));
            
    socket.send(buf,SOCKET_BUFFER_SIZE);
    socket.recv(buf,SOCKET_BUFFER_SIZE);
    socket.disconnect(PEER_ADDRESS);
}
int main(int argc, char **argv)
{  
cout<<"VARIANT: "<<VARIANT<<endl;
#if defined(LOAD_FROM_DISK)
	cout<<"MODE: LOAD DISK"<<endl;
#else
	cout<<"MODE: MEMORY"<<endl;
#endif
cout<<"# FILE:"<<MAX_NUM_OF_FILES<<endl;
cout<<"# KW: "<<MAX_NUM_KEYWORDS<<endl;
cout<<"Server ID: "<<PEER_ADDRESS<<endl;

    auto start = time_now;
    auto end = time_now;
	string search_word;
    Miscellaneous misc;
    std::string update_loc = gcsUpdateFilepath;
    string updating_filename;
    
    TYPE_COUNTER search_result;
    setbuf(stdout,NULL);
    
    Client_DSSE*  client_dsse = new Client_DSSE();


    string str_keyword;
    
    for(int i = 0 ; i < 15;i++)
    {
        client_dsse->stats[i].reserve(nTime);
    }
    while (1)
    {
        int selection =-1;
        do
        {
            printMenu();
            cout<<"Select your choice: ";
            while(!(cin>>selection))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(),'\n');
                cout<<"Invalid input. Try again: ";
            }
            
        }while(selection < 0 && selection >4);
        switch(selection)
        {
        case 0:
            client_dsse->genMaster_key();
            break;
        case 1:
            start = time_now;
            client_dsse->createEncrypted_data_structure();
            end = time_now;
            cout<<"Elapsed time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ms"<<endl;
            break;
        case 2:
            str_keyword = "the";
            //cout<<"Keyword search: ";
            //cin>>str_keyword;
            std::transform(str_keyword.begin(),str_keyword.end(),str_keyword.begin(),::tolower);
            for(int i = 0 ; i <= nTime; i ++)
            {
                cout<<i<<endl;
//		cin.get();
                client_dsse->searchKeyword(str_keyword,search_result);
//                cout<<Client_DSSE::stats[5][Client_DSSE::stats[5].size()-1]<<endl;
            }
            reset(ROW);
            
            updating_filename = "add";
            if(!fexists(gcsUpdateFilepath+updating_filename))
            {
                cout<<endl<<"File not found! Please put/check the file into/in update folder"<<endl;
                break;
            }
            //start = time_now;
            for(int i = 0; i <=nUpdateTime; i++)
            {
                cout<<i<<endl;
                client_dsse->addFile(updating_filename,gcsUpdateFilepath);
                client_dsse->delFile(updating_filename,gcsUpdateFilepath);
            }
            reset(COL);
            
            
            //end = tClient_DSSEime_now;
            /*
            cout<<"Total Elapsed time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ms"<<endl;
            cout<<"Keyword "<<str_keyword.c_str()<<" appears in "<<search_result<<" files!"<<endl;
#if (defined(SEND_SEARCH_FILE_INDEX) || !defined(CLIENT_SERVER_MODE))
            if(search_result>0)
            {
                cout<<"See "<<gcsDataStructureFilepath<<FILENAME_SEARCH_RESULT<<" for file ID specification!"<<endl;
                
            }
#endif*/
            break;    
	case 3:
/*
            cout<<"Specify the filename want to add: ";
            cin>>updating_filename;
            if(!fexists(gcsUpdateFilepath+updating_filename))
            {
                cout<<endl<<"File not found! Please put/check the file into/in update folder"<<endl;
                break;
            }
            //start = time_now;
            for(int i = 0; i <nUpdateTime; i++)
            {
                client_dsse->addFile(updating_filename,gcsUpdateFilepath);
                client_dsse->delFile(updating_filename,gcsUpdateFilepath);
            }
            reset(COL);
            //end = time_now;
            //cout<<"Total Elapsed time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ms"<<endl;*/
updating_filename = "add";
            if(!fexists(gcsUpdateFilepath+updating_filename))
            {
                cout<<endl<<"File not found! Please put/check the file into/in update folder"<<endl;
                break;
            }
            //start = time_now;
            for(int i = 0; i <=nUpdateTime; i++)
            {
                cout<<i<<endl;
                client_dsse->addFile(updating_filename,gcsUpdateFilepath);
                client_dsse->delFile(updating_filename,gcsUpdateFilepath);
            }
            reset(COL);

            break;
        /*case 4:
            cout<<"Specify the filename want to delete: ";
            cin>>updating_filename;
            start = time_now;
            client_dsse->delFile(updating_filename,gcsUpdateFilepath);
            end = time_now;
            cout<<"Total Elapsed time: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ms"<<endl;
            
            break;*/
        
        default:
            break;
        }
    }
    
    
    
    return 0;
    
}
void printMenu()
{
    cout<<"---------------"<<endl<<endl;
    cout<<"0. (Re)generate keys"<<endl;
    cout<<"1. (Re)build data structure"<<endl;
    cout<<"2. Keyword search: "<<endl;
    cout<<"3. Add files"<<endl;
    cout<<"4. Delete files"<<endl<<endl;;
    cout<<"---------------"<<endl;
}
