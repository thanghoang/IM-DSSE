#include "DSSE_Param.h"
#include "Krawczyk128_KDF.h"

#include <iostream>
#include "DSSE_Crypto.h"
using namespace std;

Krawczyk128_KDF::Krawczyk128_KDF()
{
    
}
Krawczyk128_KDF::~Krawczyk128_KDF()
{

}

int Krawczyk128_KDF::generate_128_SKM(unsigned char *pSKM, int SKM_len) {

	int error = 0;
	// NULL checks
	if(pSKM==NULL)
    {
        return -1;
    }

	if(SKM_len>0)
    {
		int SKM_length = SKM_len/4 + 1;
		unsigned int *pSourceKeyMaterial;

		pSourceKeyMaterial = new unsigned int[SKM_length];

		cout << "Generating Source Key Material..." << endl;
	
        if ((error = rdrand_get_n_uints_retry(SKM_length, RDRAND_RETRY_NUM, pSourceKeyMaterial)) != CRYPT_OK) 
        {
			printf("Error calling rdrand_get_n_units_retry: %d\n", error/*error_to_string(error)*/);
			return -1;
		}
		memcpy(pSKM,pSourceKeyMaterial,BLOCK_CIPHER_SIZE);

		delete[] pSourceKeyMaterial;
		pSourceKeyMaterial = NULL;
	}
    else
    {
		cout << "Length of Source Key Material is <= 0" << endl;
	}
	return 0;
}

int Krawczyk128_KDF::generate_XTS(unsigned char *pXTS,
                 int XTS_len) 
{

	int error = 0;

	// NULL check
	if(pXTS==NULL)
    {
        return -1;
    }

	if(XTS_len>0)
    {
		int XTS_length = (XTS_len/4) + 1;
		unsigned int *pExtractorSalt;
		pExtractorSalt = new unsigned int[XTS_length];
		cout << "Generating Extractor Salt..." << endl;
		if ((error = rdrand_get_n_uints_retry(XTS_length, RDRAND_RETRY_NUM, pExtractorSalt)) != CRYPT_OK) 
        {
			printf("Error calling rdrand_get_n_units_retry: %d\n", error/*error_to_string(error)*/);
			return -1;
		}

		memcpy(pXTS,pExtractorSalt,XTS_len);

		delete[] pExtractorSalt;
		pExtractorSalt = NULL;
	}
	else
    {
		cout << "Length of Extractor Salt is <= 0" << endl;
	}
	return 0;
}

int Krawczyk128_KDF::generate_128_PRK(unsigned char *pPRK,
                     int PRK_len,
                     unsigned char *pXTS,
                     int XTS_len,
                     unsigned char *pSKM,
                     int SKM_len) 
{
	int error = 0;
    
    if ( pPRK == NULL || pXTS == NULL || pSKM == NULL)
    {
        return -1;
    }
	
	if(XTS_len>0 && PRK_len>0 && pSKM>0)
    {
		cout << "Generating Pseudo Random Key..." << endl;
		if ((error = omac_aes128_intel(pPRK, BLOCK_CIPHER_SIZE, pXTS, XTS_len, pSKM)) != CRYPT_OK) {
			printf("Error calling omac_aes128_intel: %d\n", error/*error_to_string(error)*/);
			return -1;
		}
	}
    else
    {
		cout << "Either length of Pseudo Random Key or Extractor salt or Source Key Material is <= 0" << endl;
	}
	return 0;
}

int Krawczyk128_KDF::generate_krawczyk_128_KDF(unsigned char *pKM,
                     int KM_len,
                     unsigned char *pCTXinfo,
                     int CTXinfo_len,
                     unsigned char *pPRK,
                     int PRK_len)
{

	int error = 0;
	unsigned char *pKeyMaterial;

	// NULL checks
	if(pKM == NULL || pCTXinfo == NULL || pPRK==NULL)
    {
        return -1;
    }
    
	pKeyMaterial = new unsigned char[BLOCK_CIPHER_SIZE];

	if(KM_len>0 && CTXinfo_len &&  PRK_len)
    {
		cout << "Generating Random & Uniform Key \"" << pCTXinfo << "\" using Krawczyk Key Derivation Function..." << endl;
		if ((error = omac_aes128_intel(pKeyMaterial, BLOCK_CIPHER_SIZE, pCTXinfo, CTXinfo_len, pPRK)) != CRYPT_OK) {
			printf("Error calling omac_aes128_intel: %d\n", error/*error_to_string(error)*/);
			return -1;
		}

		memcpy(pKM,pKeyMaterial,KM_len);

		delete[] pKeyMaterial;
		pKeyMaterial = NULL;
	}
    else
    {
		cout << "Either length of Key Material or Label of Key Material or Pseudo Random Key is <= 0" << endl;
	}
	return 0;
}



