# IM-DSSE
Basic implementation of IM-DSSE. The full paper will be available soon. This project is built on CodeLite IDE (link: http://codelite.org). It is recommended to install CodeLite to load the full IM-DSSE workspace. 


# Required Libraries
1. ZeroMQ (download link: http://zeromq.org/intro:get-the-software)

2. Libtomcrypt (download link: https://github.com/libtom/libtomcrypt)

3. Google sparsehash (download link: https://github.com/sparsehash/sparsehash)

4. Intel AES-NI (*optional*) (download link: https://software.intel.com/en-us/articles/download-the-intel-aesni-sample-library)

## Intel AES-NI installation guide (optional)

IM-DSSE leverages Intel AES-NI to accelerate cryptographic operations. The Intel-AES-NI is available in Intel® Core™ i5, Intel® Core™ i7, Intel® Xeon® 5600 series and newer processor (see https://ark.intel.com/Search/FeatureFilter?productType=processors&AESTech=true for a complete list). This functionality can be *disabled* to test IM-DSSE with other CPU models (see the Configuration Section below). Here the brief instruction to install Intel-AES-NI:


1. Extract the .zip file downloaded from https://software.intel.com/en-us/articles/download-the-intel-aesni-sample-library
2. Open the Terminal and go to `Intel_AESNI_Sample_Library_v1.2/intel_aes_lib`
3. Run ``./mk_lnx_lib64.sh``, which will generate the header and library files in `intel_aes_lib/include` and `intel_aes_lib/include` directories, resp.
4. Copy header files and library files to your local folders (e.g., `/usr/local/include` and `/usr/local/lib`).


# Configuration
All IM-DSSE configurations are located in ```IM-DSSE/config.h```. 

## Highlighted Parameters:
```

#define INTEL_AES_NI				-> If enabled, use Intel AES-NI library

#define VARIANT_I                  		-> Set 1 of 4 options: VARIANT_MAIN, VARIANT_I, VARIANT_II, VARIANT_III

#define DISK_STORAGE_MODE            	   	-> If enabled, encrypted index will be stored on HDD (RAM if disabled)
	
#define SEND_SEARCH_FILE_INDEX        		-> If enabled, search result will contain specific file indexes

#define PEER_ADDRESS "tcp://localhost:5555"	-> Server IP Address & Port

const std::string SERVER_PORT = "5555";		-> Server Port number

#define  MAX_NUM_OF_FILES 1024              	-> Maximum number of files (It MUST be the power of 2 and divisible by 8)

#define  MAX_NUM_KEYWORDS 12000             	-> Maximum number of keywords

```

### Notes

The folder ``IM-DSSE/data`` as well as its structure are required to store generated IM-DSSE data structures. The database is located in ``IM-DSSE/data/DB``. The implementation recognize DB as a set of document files so that you can copy your DB files to this location. The current DB contains a small subset of enron DB (link: https://www.cs.cmu.edu/~./enron/).

# Build & Compile
Goto folder ``IM-DSSE/`` and execute
``` 
make
```

, which produces the binary executable file named ```IM-DSSE``` in ``IM-DSSE/Debug/``.

### If there is an error regarding to BOOL/bool type when compiling with Intel-aes-ni

- Access the header file named ``iaesni.h``, go to line 51, and comment that line as follows:

```
#ifndef bool
//#define bool BOOL 			-> line 51
#endif
```

### If the hardware does not support Intel-aes-ni

1. Disable INTEL_AES_NI in ``IM-DSSE/config.h``

2. Change the make file in ``IM-DSSE/MakeFile`` by removing the library linker ``-lintel-aes64`` 



# Usage

Run the binary executable file ```IM-DSSE```, which will ask for either Client or Server mode. The IM-DSSE implementation can be tested using either **single** machine or **multiple** machines with network:


## Local Testing:
1. Set ``PEER_ADDRESS`` in ``IM-DSSE/config.h`` to be ``localhost``. 
2. Choose  ``SERVER_PORT`` identical with what indicated in ``PEER_ADDRESS``. 
3. Compile the code with ``make`` in the ``IM-DSSE/`` folder. 
4. Go to ``IM-DSSE/Debug`` and run the compiled ``IM-DSSE`` file with two different Terminals, each playing the client/server role.

## Real Network Testing:
1. Set ``PEER_ADDRESS`` and  ``SERVER_PORT`` in ``IM-DSSE/config.h`` with the corresponding server's IP address  and port number.
2. Run ``make`` in ``IM-DSSE/`` to compile and generate executable file ``IM-DSSE`` in ``IM-DSSE/Debug`` folder.
3. Copy the file ``IM-DSSE`` in ``IM-DSSE/Debug`` to different machines
4. Execute the file and follow the instruction on the screen.


# Android Build

(To be updated)


# Further Information
For any inquiries, bugs, and assistance on building and running the code, please contact Thang Hoang (hoangmin@oregonstate.edu).
