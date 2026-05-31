#include <exiv2/exiv2.hpp>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <filesystem>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif


using namespace std;
namespace fs = std::filesystem;
using json  = nlohmann::json;

string createEditableCopy(const string& originalPath) {

    fs::path original(originalPath);

    fs::path copyPath =
        original.stem().string() + "_kopie" + original.extension().string();

    fs::copy_file(
        original,
        copyPath,
        fs::copy_options::overwrite_existing
    );

    return copyPath.string();
}

//*Texabgleich
string toLower(string text) {

    transform(
        text.begin(),
        text.end(),
        text.begin(),
        ::tolower
    );

    return text;
}

string removeQuotes(string text) {

    text.erase(
        remove(text.begin(), text.end(), '"'),
        text.end()
    );

    return text;
}

size_t writeCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    string* output
) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

#ifdef _WIN32

void printUtf8(const string& text) {

    int wideSize = MultiByteToWideChar(
        CP_UTF8,
        0,
        text.c_str(),
        -1,
        NULL,
        0
    );

    wstring wideText(wideSize, 0);

    MultiByteToWideChar(
        CP_UTF8,
        0,
        text.c_str(),
        -1,
        &wideText[0],
        wideSize
    );

    DWORD written;

    WriteConsoleW(
        GetStdHandle(STD_OUTPUT_HANDLE),
        wideText.c_str(),
        wideSize - 1,
        &written,
        NULL
    );
}

#endif

string translateWithDeepL(const string& text) {

    static map<string, string> cache;

    // keine doppelten Übersetzungen
    if (cache.find(text) != cache.end()) {
        return cache[text];
    }

    const char* apiKey = getenv("DEEPL_API_KEY");

    // Falls kein API-Key gesetzt ist
    if (!apiKey) {
        cout << "[DeepL Fehler] API Key nicht gefunden.\n";
        return text;
    }
    else {
        cout << "[DeepL] API Key gefunden.\n";
    }

    CURL* curl = curl_easy_init();

    if (!curl) {
        return text;
    }

    string response;

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        "https://api-free.deepl.com/v2/translate"
    );

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        writeCallback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );

    struct curl_slist* headers = NULL;

    string authHeader =
        "Authorization: DeepL-Auth-Key " +
        string(apiKey);

    headers = curl_slist_append(
        headers,
        authHeader.c_str()
    );

    headers = curl_slist_append(
        headers,
        "Content-Type: application/x-www-form-urlencoded"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );

    char* escapedText =
        curl_easy_escape(
            curl,
            text.c_str(),
            text.length()
        );

    string postData =
        "text=" + string(escapedText) +
        "&source_lang=EN" +
        "&target_lang=DE";
        "&tag_handling=html";

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        postData.c_str()
    );

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_free(escapedText);

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        cout << "[DeepL Fehler] "
             << curl_easy_strerror(res)
            << "\n";
        return text;
    }

    try {

        json result = json::parse(response);

        string translated =
            result["translations"][0]["text"];

        cache[text] = translated;

        return translated;
    }
    catch (...) {

        return text;
    }
}

//*Datenausgabe
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

            string translated =
                translateWithDeepL(entry.tagLabel());

            #ifdef _WIN32

                printUtf8(translated);

                cout << " ("
                     << key
                     << ") = "
                     << value
                     << "\n";

            #else

                cout << translated
                     << " ("
                     << key
                     << ") = "
                     << value
                     << "\n";

            #endif
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

//*Datenexport
void exportExifToFile(
    const Exiv2::ExifData& exifData,
    const map<string, string>& exifToUser
    ){

    string name;
    string format;

    ofstream file;

    cout << "Im welchen Format Exportieren?\n";
    cout << "TXT / CSV\n";

    getline(cin, format);

    format = toLower(format);

    if (format == "txt") {
        cout << "Name des .txt Datei eingeben:\n";
        getline(cin, name);
        file.open(name + ".txt");
    }
    else if (format == "csv") {
        cout << "Name des .csv Datei eingeben:\n";
        getline(cin, name);
        file.open(name + ".csv");
    }
    else {
        cout << "Unbekanntes Format.\n";
        return;
    }

    if (!file.is_open()) {
        cout << "Datei konnte nicht erstellt werden.\n";
        return;
    }

    for (const auto& entry : exifData) {

        string key = entry.key();
        string value = entry.value().toString();

        string label;

        if (exifToUser.find(key) != exifToUser.end()) {
            label = exifToUser.at(key);
        }
        else {
            label = key;
        }

        if (format == "csv")
            file << label << ";" << value << "\n";
        else
            file << label << " = " << value << "\n";
    }

    file.close();

    cout << "Export abgeschlossen: " << name << "." << format << "\n";
}

//!Hauptfunktion
int main() {

#ifdef _WIN32
#include <windows.h>
#endif

while (true) { //*Schleife über gesamte Programm
    
    string filename;

    cout << "\nBilddatei Verzeichnis eingeben\n";
    cout << "(oder 'exit' zum Beenden):\n";

    getline(cin, filename);

    filename = removeQuotes(filename);

    if (filename == "exit")
        return 0;

    shared_ptr<Exiv2::Image> image;
    
    try{
        image = Exiv2::ImageFactory::open(filename);
    }
    catch (const Exiv2::Error& e) {
        cout << "Datei konnte nicht geoffnet werden (Exiv2-Fehler):\n"<< e.what() << "\n";
        continue; // zurück zur Dateiauswahl
    }  
    if (image.get() == 0) {
        cout << "Datei konnte nicht geoffnet werden.\n";
        continue;
    } 

    // Original bleibt unangetastet
    string backup = createEditableCopy(filename);

    image->readMetadata(); //*Lesen von Exif Daten
    
    Exiv2::ExifData& exifData = image->exifData(); //*Speichern von Exif Daten in Buffer

    if (exifData.empty()) { //*Fehler: Daten wurden nicht gefunden
        cout << "Keine EXIF Daten gefunden.\n";
        continue;
    }
    
    //?Angezeigte Daten
    map<string, string> exiftoUser = {
        {"Exif.Image.Make", "Hersteller"},
        {"Exif.Image.Model", "Kameramodell"},
        {"Exif.Image.Orientation", "Orientierung"},
        {"Exif.Image.XResolution", "X Aufloesung"},
        {"Exif.Image.YResolution", "Y Aufloesung"},
        {"Exif.Image.ResolutionUnit", "Aufloesungseinheit"},
        {"Exif.Image.Software", "Software"},
        {"Exif.Image.DateTime", "Datum"},
        {"Exif.Image.YCbCrPositioning", "YCbCr Positionierung"},
        {"Exif.Image.ExifTag", "EXIF Tag"},
        {"Exif.Photo.0x0100", "Bildbreite"},
        {"Exif.Photo.0x0101", "Bildhoehe"},
        {"Exif.Photo.ExposureTime", "Belichtungszeit"},
        {"Exif.Photo.FNumber", "Blende"},
        {"Exif.Photo.ExposureProgram", "Belichtungsprogramm"},
        {"Exif.Photo.ISOSpeedRatings", "ISO"},
        {"Exif.Photo.ExifVersion", "EXIF Version"},
        {"Exif.Photo.DateTimeOriginal", "Aufnahmedatum"},
        {"Exif.Photo.DateTimeDigitized", "Digitalisiert"},
        {"Exif.Photo.ComponentsConfiguration", "Komponenten"},
        {"Exif.Photo.ShutterSpeedValue", "Verschlusszeit"},
        {"Exif.Photo.ApertureValue", "Blendenwert"},
        {"Exif.Photo.BrightnessValue", "Helligkeit"},
        {"Exif.Photo.ExposureBiasValue", "Belichtungskorrektur"},
        {"Exif.Photo.MaxApertureValue", "Max Blende"},
        {"Exif.Photo.MeteringMode", "Messmodus"},
        {"Exif.Photo.Flash", "Blitz"},
        {"Exif.Photo.FocalLength", "Brennweite"},
        {"Exif.Photo.SubSecTime", "Millisekunden"},
        {"Exif.Photo.SubSecTimeOriginal", "Original Millisekunden"},
        {"Exif.Photo.SubSecTimeDigitized", "Digitalisiert Millisekunden"},
        {"Exif.Photo.ColorSpace", "Farbraum"},
        {"Exif.Photo.WhiteBalance", "Weissabgleich"},
        {"Exif.Photo.FocalLengthIn35mmFilm", "35mm Brennweite"},
        {"Exif.Photo.SceneCaptureType", "Szenentyp"},
        {"Exif.Photo.ImageUniqueID", "Bild ID"}
    };

    //?Eingegebene Daten
    map<string, string> userToExif = {
        {"hersteller", "Exif.Image.Make"},
        {"kameramodell", "Exif.Image.Model"},
        {"orientierung", "Exif.Image.Orientation"},
        {"xaufloesung", "Exif.Image.XResolution"},
        {"yaufloesung", "Exif.Image.YResolution"},
        {"aufloesungseinheit", "Exif.Image.ResolutionUnit"},
        {"software", "Exif.Image.Software"},
        {"datum", "Exif.Image.DateTime"},
        {"ycbcr positionierung", "Exif.Image.YCbCrPositioning"},
        {"exif tag", "Exif.Image.ExifTag"},
        {"bildbreite", "Exif.Photo.0x0100"},
        {"bildhoehe", "Exif.Photo.0x0101"},
        {"belichtungszeit", "Exif.Photo.ExposureTime"},
        {"blende", "Exif.Photo.FNumber"},
        {"belichtungsprogramm", "Exif.Photo.ExposureProgram"},
        {"iso", "Exif.Photo.ISOSpeedRatings"},
        {"exif version", "Exif.Photo.ExifVersion"},
        {"aufnahmedatum", "Exif.Photo.DateTimeOriginal"},
        {"digitalisiert", "Exif.Photo.DateTimeDigitized"},
        {"komponenten", "Exif.Photo.ComponentsConfiguration"},
        {"verschlusszeit", "Exif.Photo.ShutterSpeedValue"},
        {"blendenwert", "Exif.Photo.ApertureValue"},
        {"helligkeit", "Exif.Photo.BrightnessValue"},
        {"belichtungskorrektur", "Exif.Photo.ExposureBiasValue"},
        {"maxblende", "Exif.Photo.MaxApertureValue"},
        {"messmodus", "Exif.Photo.MeteringMode"},
        {"blitz", "Exif.Photo.Flash"},
        {"brennweite", "Exif.Photo.FocalLength"},
        {"millisekunden", "Exif.Photo.SubSecTime"},
        {"originalmillisekunden", "Exif.Photo.SubSecTimeOriginal"},
        {"digitalisiert millisekunden", "Exif.Photo.SubSecTimeDigitized"},
        {"farbraum", "Exif.Photo.ColorSpace"},
        {"weissabgleich", "Exif.Photo.WhiteBalance"},
        {"35mmbrennweite", "Exif.Photo.FocalLengthIn35mmFilm"},
        {"szenentyp", "Exif.Photo.SceneCaptureType"},
        {"bildid", "Exif.Photo.ImageUniqueID"}
    };

    printExifData(exifData, exiftoUser); //*Datenausgabe

    //*Aktion wählen
    string command;

    while(true){ //*Schleife über Datei bearbeitung
        cout << "\nBefehl eingeben:\n";
        cout << "lesen / schreiben / loeschen / export / zuruck / exit\n";

        getline(cin, command);
        command = toLower(command); //Prüft eingegebenen Text auf Großbuchstaben
            
        if (command == "zuruck") //Gang zurück zum Datei Auswahl
            break;

        if (command == "exit") //Beendet das gesamte Programm
            return 0;

        //!Block zur Bearbeitung von Daten
        try { 

            if (command == "lesen"){ //*Daten werden gedruckt
                printExifData(exifData, exiftoUser);
            }
            else if (command == "schreiben") { //*Daten werden umgeschrieben
                string key;
                string value;

                cout << "EXIF Name:\n";
                getline(cin, key);
                key = toLower(key);

                cout << "Neue Wert:\n";
                getline(cin, value);

                if (userToExif.find(key) != userToExif.end()) 
                {
                    setExif(*image, userToExif[key], value);
                    image->readMetadata();
                }
                else 
                    cout << "Unbekannter EXIF Name.\n";
            }
            else if (command == "loeschen") { //*Daten werden gelöscht
                string key;

                cout << "EXIF Name:\n";
                getline(cin, key);
                key = toLower(key);
                if (userToExif.find(key) != userToExif.end()) 
                {
                    removeExif(*image, userToExif[key]);
                    image->readMetadata();
                }
                else 
                    cout << "Unbekannter EXIF Name.\n"; 
            }
            else if(command == "export"){
                exportExifToFile(exifData, exiftoUser);
            }
            else
                cout << "Unbekannter Befehl.\n";

            }
        catch (const Exiv2::Error& e) { //*Fehler von Bibliothek

            cout << "\nEXIV2 FEHLER:\n";
            cout << e.what() << "\n";
        }
        catch (const exception& e) { //*Allgemeine Fehler
            cout << "\nALLGEMEINER FEHLER:\n"; 
            cout << e.what() << "\n";
        }
    }
}

cout << "\nProgramm beendet.\n";

return 0;
}