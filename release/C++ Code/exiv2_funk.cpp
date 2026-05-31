#include "exiv2_funk.hpp"

void printExifData(
    const Exiv2::ExifData& exifData,
    const map<string, string>& exifToUser
    ) {
    cout << "\nEXIF DATEN\n";
    cout << "===================================\n";

    for (const auto& entry : exifData) {

        string key = entry.key();
        string value = entry.value().toString();

        if (exifToUser.find(key) != exifToUser.end()) {

            cout << exifToUser.at(key)
                 << " = "
                 << value
                 << "\n";
        }
        else {

            cout << key
                 << " = "
                 << value
                 << "\n";
        }
    }
}

//*Datenschreiben
void setExif(
    Exiv2::Image& image,
    const string& key,
    const string& value
    ) {

    image.readMetadata();

    Exiv2::ExifData& exifData = image.exifData();

    exifData[key] = value;

    image.setExifData(exifData);

    image.writeMetadata();

    cout << "EXIF Wert geschrieben.\n";
}

//*Datenlöschen
void removeExif(
    Exiv2::Image& image,
    const string& key
    ) {

    image.readMetadata();

    Exiv2::ExifData& exifData = image.exifData();

    auto pos = exifData.findKey(Exiv2::ExifKey(key));

    if (pos == exifData.end()) {

        cout << "Tag nicht gefunden.\n";
        return;
    }

    exifData.erase(pos);

    image.setExifData(exifData);

    image.writeMetadata();

    cout << "EXIF Tag gelöscht.\n";
}