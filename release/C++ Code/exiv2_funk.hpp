#ifndef EXIV2FUNK_HPP
#define EXIV2FUNK_HPP

#include <iostream>
#include <map> //*Mapping
#include <string>
#include <exiv2/exiv2.hpp>

using namespace std;

void printExifData(
    const Exiv2::ExifData& exifData,
    const map<string, string>& exifToUser
    );

void setExif(
    Exiv2::Image& image,
    const string& key,
    const string& value
    );

void removeExif(
    Exiv2::Image& image,
    const string& key
    );

#endif