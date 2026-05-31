#ifndef VALID_HPP
#define VALID_HPP

#include <iostream>
#include <algorithm>
#include <cctype> //für funktion digits
#include <string>
#include <algorithm> //* wird für die Funktion removeQuotes benutzt

using namespace std;

string normaldate(string input);

bool digits(const string& text);

string toLower(string text);

string removeQuotes(string text);

#endif