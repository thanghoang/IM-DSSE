#include <stdio.h>

#include <Client_DSSE.h>
#include <DSSE_Param.h>
#include <Miscellaneous.h>
#include <unistd.h>


#include <fstream>
using namespace std;
vector<TYPE_COUNTER> gt;

void printMenu();

string exec(const char* cmd);

bool fexists(string filename_with_path)
{
  ifstream ifile(filename_with_path.c_str());
  return ifile;
}

int main(int argc, char **argv)
{

	string search_word;
    Miscellaneous misc;
    std::string update_loc = gcsUpdateFilepath;
    string updating_filename;
    
    TYPE_COUNTER search_result;
    setbuf(stdout,NULL);
    
    Client_DSSE*  client_dsse = new Client_DSSE();
    
    string str_keyword;
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
            client_dsse->createEncrypted_data_structure();
            break;
        case 2:
            cout<<"Keyword search: ";
            cin>>str_keyword;
             std::transform(str_keyword.begin(),str_keyword.end(),str_keyword.begin(),::tolower);
            client_dsse->searchKeyword(str_keyword,search_result);
            cout<<"Keyword "<<str_keyword.c_str()<<" appears in "<<search_result<<" files!"<<endl;
#if (defined(SEND_SEARCH_FILE_INDEX) || !defined(CLIENT_SERVER_MODE))
            if(search_result>0)
            {
                cout<<"See "<<gcsDataStructureFilepath<<FILENAME_SEARCH_RESULT<<" for file ID specification!"<<endl;
                
            }
#endif
            break;
        case 3:
            cout<<"Specify the filename want to add: ";
            cin>>updating_filename;
            if(!fexists(gcsUpdateFilepath+updating_filename))
            {
                cout<<endl<<"File not found! Please put/check the file into/in update folder"<<endl;
                break;
            }
            client_dsse->addFile(updating_filename,gcsUpdateFilepath);
            break;
        case 4:
            cout<<"Specify the filename want to delete: ";
            cin>>updating_filename;
            client_dsse->delFile(updating_filename,gcsUpdateFilepath);
            break;
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
