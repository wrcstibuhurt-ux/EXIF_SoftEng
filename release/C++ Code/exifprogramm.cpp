#include <exiv2/exiv2.hpp>
#include <iostream>
#include <string>
#include <fstream> //* Bibliothek für export
#include <filesystem> //*Bibliothek zur Kopieerstellung
#include "mapping.hpp"
#include "valid.hpp"
#include "exiv2_funk.hpp"
#include "valid.hpp"

using namespace std;
namespace fs = std::filesystem;

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
                
                if (fieldType[key] == "date")
                    value = normaldate(value);

                if (userToExif.find(key) != userToExif.end()) 
                {
                    setExif(*image, userToExif[key], value);
                    image->readMetadata();
                }
                else{
                    try { //*Schreibt neues Exif Key
                        Exiv2::ExifKey exifKey(key);
                        setExif(*image, key, value);
                        image->readMetadata();
                        cout << "Neuer Tag wurde erstellt.\n";
                    }
                    catch (...) {
                        cout << "Unbekannter EXIF Name.\n";
                    }
                } 
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