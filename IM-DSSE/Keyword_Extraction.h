#ifndef KEYWORD_FILE_PROCESSING_H
#define KEYWORD_FILE_PROCESSING_H

#include "config.h"
class KeywordExtraction
{
public:
    KeywordExtraction();
    ~KeywordExtraction();
    
    int extractKeywords(TYPE_KEYWORD_DICTIONARY &rKeywordsDictionary,string file_name,string path);
    int extractWords_using_find_first_of(TYPE_KEYWORD_DICTIONARY &rKeywordsDictionary,TYPE_COUNTER *pKeywordNum,ifstream &rFin);
};

#endif // KEYWORD_FILE_PROCESSING_H
