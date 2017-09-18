#ifndef FILECRYPT_H
#define FILECRYPT_H

#include "DSSE_Param.h"
#include "MasterKey.h"
class FileCrypt
{
public:
    FileCrypt();
    ~FileCrypt();
int encryptFiles(TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                vector<string> &rFileNames,
                string path,
                string encrypted_files_path,
                MasterKey *pKey);
int encryptFile_using_ccm(TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                        string file_name,
                        string path,
                        string encrypted_files_path,
                        MasterKey *pKey) ;
                       
int encryptFile_using_aes_ctr(TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                            string file_name,
                            string path,
                            string encrypted_files_path,
                            MasterKey *pKey);
int decryptFile_using_ccm(string file_name,
                        string encrypted_files_path,
                        TYPE_INDEX file_index,
                        MasterKey *pKey);
                       
int decryptFile_using_aes_ctr(string file_name,
                            string encrypted_files_path,
                            TYPE_INDEX file_index,
                            MasterKey *pKey);
                           
};

#endif // FILECRYPT_H
