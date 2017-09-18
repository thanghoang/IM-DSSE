#include "DSSE_FileCrypt.h"
#include "DSSE_Trapdoor.h"
#include "Miscellaneous.h"
#include "DSSE_Crypto.h"
FileCrypt::FileCrypt()
{
}

FileCrypt::~FileCrypt()
{
}

/**
 * Function Name: encryptFiles
 *
 * Description:
 * Encrypt files from file collection
 *
 * @param rT_F: (input) file hash table
 * @param rFileNames: (input) list of names of files in file collection
 * @param path: (input) location of file collection
 * @param encrypted_files_path: (input) location of file after being encrypted
 * @param pKey: (input) symmetric key used to encrypt files
 * @return	0 if successful
 */
int FileCrypt::encryptFiles(TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
              vector<string> &rFileNames,
              string path,
              string encrypted_files_path,
              MasterKey *pKey) 
{
	unsigned int file_num = 0;

	try
    {
		// Encrypts a set of files
		for(file_num = 0;file_num < rFileNames.size();file_num++) 
        {
			if(!(rFileNames[file_num].empty()))
				this->encryptFile_using_ccm(rT_F, rFileNames[file_num], path, encrypted_files_path, pKey);
			else
				cout << "file name of file : " << file_num << " is empty " << endl;
		}
	}
    catch(exception &e)
    {
		cout << "Error occurred in files_enc function " << e.what() << endl;
	}
	return 0;
}

/**
 * Function Name: encryptFile_using_ccm
 *
 * Description:
 * Encrypt a file using AES-CCM mode
 *
 * @param rT_F: (input) file hash table
 * @param file_name: (input) name of file
 * @param path: (input) location of file 
 * @param encrypted_files_path: (input) location of file after being encrypted
 * @param pKey: (input) symmetric key used to encrypt files
 * @return	0 if successful
 */
int FileCrypt::encryptFile_using_ccm(TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                       string file_name,
                       string path,
                       string encrypted_files_path,
                       MasterKey *pKey) {

	TYPE_INDEX file_index = 0;
	unsigned long taglen = 0, num_char = 0;
	unsigned char file_trapdoor[TRAPDOOR_SIZE], nonce[NONCE_SIZE], tag[BLOCK_CIPHER_SIZE];
	unsigned char *pInData, *pOutData;
	string file_name_with_path;
	ostringstream tar_command, zip_command, nonce_source, enc_fname_stream, enc_fname_with_path_stream, tarname_stream, tag_fname_with_path_stream, tag_fname_stream;
	hashmap_key_class hmap_file_trapdoor;

	// NULL checks
	if(rT_F.empty() || pKey->isNULL())
    {
        return -1;
	}
    
	try
    {
		taglen = BLOCK_CIPHER_SIZE;

		file_name_with_path.append(file_name);

		if(file_name.size()>0){
		    DSSE_Trapdoor* tfunc = new DSSE_Trapdoor();
			tfunc->generateTrapdoor_single_input(file_trapdoor, TRAPDOOR_SIZE, (unsigned char *)file_name.c_str(), file_name.size(), pKey);
            delete tfunc;
        } else {
			cout << "File name is empty!" << endl;
        }
		hmap_file_trapdoor = hashmap_key_class(file_trapdoor,TRAPDOOR_SIZE);

		file_index = rT_F.bucket(hmap_file_trapdoor);
		
		// Source of the file labelled using index of the file where encrypted data is stored
		enc_fname_with_path_stream << encrypted_files_path << "encFile" << file_index << ".txt";

		// Name of the encrypted file labelled using index of the file where encrypted data is stored
		enc_fname_stream << "encFile" << file_index << ".txt";

		// Source of the MAC file labelled using index of the file where MAC of unencrypted data is stored
		tag_fname_with_path_stream << encrypted_files_path <<MAC_NAME << file_index << ".txt";

		// Name of the MAC file labelled using index of the file where MAC of unencrypted data is stored
		tag_fname_stream << MAC_NAME << file_index << ".txt";

		// Source of the tarball labelled using index of the file where encrypted data is stored
		tarname_stream << encrypted_files_path << "encTar" << file_index << ".tar";

		// Setting the input for generating nonce with label of encrypted file name and file index appended to it
		nonce_source << enc_fname_stream.str() << file_index;

		// Generates the nonce used for CCM mode using nonce source
		omac_aes128(nonce, NONCE_SIZE, (unsigned char *)nonce_source.str().c_str(), nonce_source.str().size(), pKey->key1);

		// Computes the size of the file
        Miscellaneous misc;
		num_char = misc.read_filesize_cpp(file_name_with_path);

		// ptr_size = num_blocks*BLOCK_CIPHER_SIZE;

		pInData = new unsigned char[num_char];

		pOutData = new unsigned char[num_char];

		// Read content from the file
		misc.read_file_cpp(pInData, num_char, file_name_with_path);

		ccm_128_enc_dec(0, pKey->key1, BLOCK_CIPHER_SIZE, NULL, nonce, NONCE_SIZE, NULL, 0, pInData, num_char, pOutData, tag, &taglen, 0);

		// Write the encrypted data to new file labelled using file index
		misc.write_file_cpp(enc_fname_with_path_stream.str(), pOutData, num_char);

		// Write the MAC of unencrypted data to a MAC file labelled using file index
		misc.write_file_cpp(tag_fname_with_path_stream.str(), tag, (int)taglen);

		// Prepare the system command for tarring the files together
		// command << "tar -cvf " << tarname_stream.str() << " " << gcsEncFilepath.c_str() << "files/" << enc_fname_stream.str() << " " << gcsEncFilepath.c_str() << "tags/" << tag_fname_stream.str();
        
        //remove file before tarring it : added by Thang Hoang        
        string tmp = tarname_stream.str() + ".gz";
        remove(tmp.c_str());
        
        
		tar_command << "tar -cf " << tarname_stream.str() << " -C " << encrypted_files_path.c_str() << " " << enc_fname_stream.str() << " -C " << encrypted_files_path.c_str() << " " << tag_fname_stream.str();
		zip_command << "gzip " << tarname_stream.str();

		// Execute the system command
        cout<<tar_command.str().c_str()<<endl;
		system(tar_command.str().c_str());
		system(zip_command.str().c_str());

		remove(enc_fname_with_path_stream.str().c_str());
		remove(tag_fname_with_path_stream.str().c_str());

		// generate_tarball(enc_tarname_stream.str(), enc_fname_with_path_stream.str() , enc_fname_stream.str(), pTag, taglen);
        
		// Freeing memory
		delete[] pInData;
		pInData = NULL;

		delete[] pOutData;
		pOutData = NULL;

		file_name_with_path.clear();

		tar_command.str(std::string());
		tar_command.clear();

		enc_fname_stream.str(std::string());
		enc_fname_stream.clear();

		enc_fname_with_path_stream.str(std::string());
		enc_fname_with_path_stream.clear();

		nonce_source.str(std::string());
		nonce_source.clear();

		tag_fname_stream.str(std::string());
		tag_fname_stream.clear();

		tag_fname_with_path_stream.str(std::string());
		tag_fname_with_path_stream.clear();

	}catch(exception &e){
		cout << "Error occured in file_enc function " << e.what() << endl;
	}

	return 0;
}

/**
 * Function Name: encryptFile_using_aes_ctr
 *
 * Description:
 * Encrypt a file using AES-CTR mode
 *
 * @param rT_F: (input) file hash table
 * @param file_name: (input) name of file
 * @param path: (input) location of file 
 * @param encrypted_files_path: (input) location of file after being encrypted
 * @param pKey: (input) symmetric key used to encrypt files
 * @return	0 if successful
 */
int FileCrypt::encryptFile_using_aes_ctr(TYPE_GOOGLE_DENSE_HASH_MAP &rT_F,
                           string file_name,
                           string path,
                           string encrypted_files_path,
                           MasterKey *pKey) 
{

	int index = 0, num_blocks = 0, num_char = 0, ptr_size = 0;
	TYPE_INDEX file_index = 0;
	unsigned char *pFileTrapdoor, *pInData, *pOutData, *pInitialCounter, *pEncryptedData;
	string file_name_with_path;
	ostringstream fname_stream;
    Miscellaneous misc;
	hashmap_key_class file_trapdoor;

	// NULL checks
	if(rT_F.empty() || pKey->isNULL())
        return -1;
	
	try
    {
		pInitialCounter = new unsigned char[BLOCK_CIPHER_SIZE];
		pFileTrapdoor = new unsigned char[TRAPDOOR_SIZE];

		// Prepare filename with it's absolute path
		file_name_with_path.append(file_name);

		// Typecast the file name (string type to unsigned char array)
        
		cout << "Generating the trapdoor for file " << file_name_with_path << " ... " << endl;
		if(file_name_with_path.size()>0)
        {
			// Generates the file trapdoor for the new file using it's name
            DSSE_Trapdoor* tfunc = new DSSE_Trapdoor(); 
			tfunc->generateTrapdoor_single_input(pFileTrapdoor, TRAPDOOR_SIZE, (unsigned char *)file_name_with_path.c_str(), file_name_with_path.size(), pKey);
            
		}
        else
        {
			cout << "File name is empty!" << endl;
        }
		// Typecast file trapdoor to hashmap entry type (hashmap_key_class)
		file_trapdoor = hashmap_key_class(pFileTrapdoor,TRAPDOOR_SIZE);

		// Get the file index from the hashmap
		file_index = rT_F.bucket(file_trapdoor);
		cout << "file_index of the file " << file_name_with_path << " : " << file_index<< endl;

		// Computes the size of the file
		num_char = misc.read_filesize_cpp(file_name_with_path);

		// Computes the number of blocks of data to be decrypted
		if(num_char%BLOCK_CIPHER_SIZE == 0)
        {
			num_blocks = (num_char/BLOCK_CIPHER_SIZE);
		}
		else
        {
			num_blocks = (num_char/BLOCK_CIPHER_SIZE)+1;
		}

		ptr_size = num_blocks*BLOCK_CIPHER_SIZE;

		pInData = new unsigned char[num_char];

		pOutData = new unsigned char[ptr_size];

		pEncryptedData = new unsigned char[num_char];

		// Read content from the file
		misc.read_file_cpp(pInData, num_char, file_name_with_path);

		// Preparing the initial counter
		misc.prepare_initial_counter(pInitialCounter, BLOCK_CIPHER_SIZE, &file_index);

		cout << "Encrypting the file " << file_name_with_path << " ... " << endl;
		//numChar we get from read_filesize_cpp() are one byte blocks but have to pass 16 bytes blocks=>so numBlocks
		aes128_ctr_encdec(pInData, pOutData, pKey->key1, pInitialCounter, num_blocks);

		// pOutData stores encrypted data in the size of multiples of BLOCK_CIPHER_SIZE, so need to trucate the unwanted crap
		for(index = 0; index < num_char; index++)
			pEncryptedData[index] = pOutData[index];

		// Name of the file labelled using index of the file where encrypted data is stored
		fname_stream << gcsEncFilepath << "encFile" << file_index << ".txt";
		// fname_stream << path << "/encrypted_files/encFile" << file_index << ".txt";

		cout << "Writing the encrypted data from file " << file_name_with_path << " to file " << fname_stream.str() << " ... " << endl;
		// Write the encrypted data to new file labelled using file index
		misc.write_file_cpp(fname_stream.str(), pEncryptedData, num_char);

		// Freeing memory
		delete[] pFileTrapdoor;
		pFileTrapdoor = NULL;

		delete[] pInData;
		pInData = NULL;

		delete[] pOutData;
		pOutData = NULL;

		delete[] pInitialCounter;
		pInitialCounter = NULL;

		delete[] pEncryptedData;
		pEncryptedData = NULL;

		file_name_with_path.clear();


		fname_stream.str(std::string());
		fname_stream.clear();

	}
    catch(exception &e)
    {
		cout << "Error occured in file_enc function " << e.what() << endl;
	}

	return 0;
}


/**
 * Function Name: decryptFile_using_ccm
 *
 * Description:
 * Decrypt a file using AES-CCM mode
 *
 * @param file_name: (input) name of file
 * @param encrypted_files_path: (input) location of file 
 * @param file_index: (input) index of file in the DSSE encrypted data structure
 * @param pKey: (input) symmetric key used to encrypt files
 * @return	0 if successful
 */
int FileCrypt::decryptFile_using_ccm(string file_name,
                       string encrypted_files_path,
                       TYPE_INDEX file_index,
                       MasterKey *pKey)
{

	int msg_original = 0;
	unsigned long num_char = 0, taglen = 0;
	unsigned char *pInData, *pOutData, *pNonce, *pTag, *pTagcp;
	string tar_name_with_path, tar_extract_path;
	ostringstream tar_command, zip_command, nonce_source, file_name_with_path, tag_fname_with_path, enc_fname_stream;
    Miscellaneous misc;
	if(pKey->isNULL())
        return -1;
	try
    {
		pNonce = new unsigned char[NONCE_SIZE];
		pTag = new unsigned char[BLOCK_CIPHER_SIZE];
		pTagcp = new unsigned char[BLOCK_CIPHER_SIZE];
		taglen = BLOCK_CIPHER_SIZE;

		// Prepare filename with it's absolute path
		tar_name_with_path.append(encrypted_files_path);
		tar_name_with_path.append(file_name);

		tar_extract_path.append(encrypted_files_path);
		tar_extract_path.append("extracted_files/");

//		zip_command << "gunzip " << tar_name_with_path.c_str();
		tar_command << "tar -xf " << tar_name_with_path.c_str() << " -C " << tar_extract_path.c_str();

		system(tar_command.str().c_str());

		file_name_with_path << encrypted_files_path << "extracted_files/encFile" << file_index << ".txt";

		tag_fname_with_path << encrypted_files_path << "extracted_files/MAC" << file_index << ".txt";

		// Name of the encrypted file labeled using index of the file where encrypted data is stored
		enc_fname_stream << "encFile" << file_index << ".txt";

		// Computes the size of the file
		num_char = misc.read_filesize_cpp(file_name_with_path.str());

		pInData = new unsigned char[num_char];

		pOutData = new unsigned char[num_char];

		// Read content from the file
		misc.read_file_cpp(pInData, num_char, file_name_with_path.str());

		misc.read_file_cpp(pTagcp, BLOCK_CIPHER_SIZE, tag_fname_with_path.str());
        
		// Setting the input for generating nonce with label of encrypted file name and file index appended to it
		nonce_source << enc_fname_stream.str() << file_index;

		// Generates the nonce used for CCM mode using nonce source
		omac_aes128(pNonce, NONCE_SIZE, (unsigned char *)nonce_source.str().c_str(), nonce_source.str().size(), pKey->key1);

		//numChar we get from read_filesize_cpp() are one byte blocks but have to pass 16 bytes blocks=>so numBlocks
		ccm_128_enc_dec(0, pKey->key1, BLOCK_CIPHER_SIZE, NULL, pNonce, NONCE_SIZE, NULL, 0, pOutData, num_char, pInData, pTag, &taglen, 1);

		msg_original = memcmp(pTagcp,pTag,taglen);

		if(!msg_original)
		cout << "The file " << file_name << " is authenticated & original!" << endl;
		else
			cout << "Encrypted file has been modified or tampered!" << endl;

		// Write the decrypted data back to file
		misc.write_file_cpp(file_name_with_path.str(), pOutData, num_char);

		// Freeing memory
		file_name_with_path.clear();
		tar_name_with_path.clear();
		tar_extract_path.clear();

		tar_command.str();
		tar_command.clear();

		nonce_source.str(std::string());
		nonce_source.clear();

		file_name_with_path.str(std::string());
		file_name_with_path.clear();

		tag_fname_with_path.str(std::string());
		tag_fname_with_path.clear();

		enc_fname_stream.str(std::string());
		enc_fname_stream.clear();

		delete[] pInData;
		pInData = NULL;

		delete[] pOutData;
		pOutData = NULL;

		delete[] pNonce;
		pNonce = NULL;

		delete[] pTag;
		pTag = NULL;


	}
    catch(exception &e)
    {
		cout << "Error in file_dec function " << e.what() << endl;
	}
	return 0;
}

/**
 * Function Name: decryptFile_using_aes_ctr
 *
 * Description:
 * Decrypt a file using AES-CTR mode
 *
 * @param file_name: (input) name of file
 * @param path: (input) location of file 
 * @param encrypted_files_path: (input) location of file
 * @param file_index: (input) index of file in the DSSE encrypted data structure
 * @param pKey: (input) symmetric key used to encrypt files
 * @return	0 if successful
 */
int FileCrypt::decryptFile_using_aes_ctr(string file_name,
                           string encrypted_files_path,
                           TYPE_INDEX file_index,
                           MasterKey *pKey) 
{
	int index = 0, num_blocks = 0, num_char = 0, ptr_size = 0;
	unsigned char *pInData, *pOutData, *pDecryptedData, *pInitialCounter;
	string file_name_with_path;
    Miscellaneous misc;
	if(pKey->isNULL())
        return -1;
	try
    {
		pInitialCounter = new unsigned char[BLOCK_CIPHER_SIZE];

		// Prepare filename with it's absolute path
		file_name_with_path.append(encrypted_files_path);
		file_name_with_path.append(file_name);

		// Computes the size of the file
		num_char = misc.read_filesize_cpp(file_name_with_path);

		// Computes the number of blocks of data to be decrypted
		if(num_char%BLOCK_CIPHER_SIZE == 0){
			num_blocks = (num_char/BLOCK_CIPHER_SIZE);
		}
		else
        {
			num_blocks = (num_char/BLOCK_CIPHER_SIZE)+1;
		}

		ptr_size = num_blocks*BLOCK_CIPHER_SIZE;

		pInData = new unsigned char[num_char];

		pOutData = new unsigned char[ptr_size];

		pDecryptedData = new unsigned char[num_char];

		// Read content from the file
		misc.read_file_cpp(pInData, num_char, file_name_with_path);

		// Preparing the initial counter
		misc.prepare_initial_counter(pInitialCounter, BLOCK_CIPHER_SIZE, &file_index);

		cout << "Decrypting the file " << file_name_with_path << " ... " << endl;
		//numChar we get from read_filesize_cpp() are one byte blocks but have to pass 16 bytes blocks=>so numBlocks
		aes128_ctr_encdec(pInData, pOutData, pKey->key1, pInitialCounter, num_blocks);
		//aes128_cbc_encrypt(pInData, pOutData, pKey, initial_counter, numBlocks);

		// print_ucharstring(pOutData, ptr_size);

		for(index = 0; index < num_char; index++)
			pDecryptedData[index] = pOutData[index];

		// Write the decrypted data back to file
		misc.write_file_cpp(file_name_with_path, pDecryptedData, num_char);

		// Freeing memory
		file_name_with_path.clear();

		delete[] pInData;
		pInData = NULL;

		delete[] pOutData;
		pOutData = NULL;

		delete[] pInitialCounter;
		pInitialCounter = NULL;

		delete[] pDecryptedData;
		pDecryptedData = NULL;

	}
    catch(exception &e)
    {
		cout << "Error in file_dec function " << e.what() << endl;
	}
	return 0;
}