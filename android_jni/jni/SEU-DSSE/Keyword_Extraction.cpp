#include "Keyword_Extraction.h"

KeywordExtraction::KeywordExtraction()
{
}

KeywordExtraction::~KeywordExtraction()
{
}

/**
 * Function Name: extractKeywords
 *
 * Description:
 * Extract unique keywords from input file which are tokenized by delimiters defined in DSSE_Params.h
 *
 * @param rKeywordsDictionary: (output) list of unique keyword being extracted
 * @param file_name: (input) name of input file 
 * @param path: (input) location of input file
 * @return	0 if successful
 */
int KeywordExtraction::extractKeywords(TYPE_KEYWORD_DICTIONARY &rKeywordsDictionary,
		string file_name,
		string path) 
{
	TYPE_COUNTER keyword_num = 0;
	string fname_with_path;

	// create a file-reading object
	ifstream fin;

	fname_with_path.append(path);
	fname_with_path.append(file_name);

	// open a file
	fin.open(fname_with_path.c_str(),ios::binary | ios::in);

	if (!fin.good())
		return -1; // exit if file not found

	// Extract keywords from a file
	extractWords_using_find_first_of(rKeywordsDictionary, &keyword_num, fin);

	fin.close();

	return 0;
}

int KeywordExtraction::extractWords_using_find_first_of(TYPE_KEYWORD_DICTIONARY &rKeywordsDictionary,
		TYPE_COUNTER *pKeywordNum,
		ifstream &rFin)
{
	string line, word;
	while(getline(rFin, line)) 
    {
		size_t prev = 0, pos;
		trim(line);
		while ((pos = line.find_first_of(delimiter, prev)) != std::string::npos)
		{
			if (pos > prev)
            {
				word = line.substr(prev, pos-prev);
				trim(word);
                //convert the word to lower case
                std::transform(word.begin(),word.end(),word.begin(),::tolower);
                rKeywordsDictionary.insert(word);
				*pKeywordNum = *pKeywordNum + 1;
			}

			prev = pos+1;
		}
		if (prev < line.length()){
			word = line.substr(prev, std::string::npos);
			trim(word);
            //convert the word to lower case
            std::transform(word.begin(),word.end(),word.begin(),::tolower);
            rKeywordsDictionary.insert(word);
			*pKeywordNum = *pKeywordNum + 1;
		}

	}

	return 0;
}