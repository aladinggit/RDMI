#ifndef _UTILS_H
#define _UTILS_H

#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <iostream>
#include <fstream>
#include <regex>
#include <arpa/inet.h>

using namespace std;

/**
 * Split a string into tokens by deliminators
*/
vector<string> split(const string& str, const string& delim) {
	vector<string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == string::npos) pos = str.length();
		string token = str.substr(prev, pos-prev);
		if (!token.empty()) tokens.push_back(token);
		prev = pos + delim.length();
	}
	while (pos < str.length() && prev < str.length());
	return tokens;
}

/**
 * trim functions
 */
string & ltrim(string & str)
{
  auto it2 =  find_if(str.begin(), str.end(), [](char ch){ return !isspace<char>(ch, locale::classic()); } );
  str.erase( str.begin() , it2);
  return str;
}

string & rtrim(string & str)
{
  auto it1 =  find_if(str.rbegin(), str.rend(), [](char ch){ return !isspace<char>(ch, locale::classic()); } );
  str.erase(it1.base(), str.end());
  return str;
}

/**
 * Use this for triming a string's starting and ending white space.
 */
string & trim(string & str)
{
   return ltrim(rtrim(str));
}


#endif