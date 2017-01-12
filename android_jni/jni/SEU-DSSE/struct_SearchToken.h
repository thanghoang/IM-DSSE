#ifndef SEARCH_TOKEN_H
#define SEARCH_TOKEN_H

typedef struct SearchToken{
	TYPE_INDEX row_index;
	unsigned char row_key[BLOCK_CIPHER_SIZE] = {'\0'};       
    unsigned char row_old_key[BLOCK_CIPHER_SIZE] = {'\0'};
    bool hasRow_key = false;
}SEARCH_TOKEN;

#endif 