#include "valid.hpp"
#include "mapping.hpp"

//*Validiert das Datum
string normaldate(string input) {

    // Entfernt Leerzeichen
    input.erase(remove_if(input.begin(), input.end(), ::isspace), input.end());

    // Begrenzt den Anzahl von Ziffern für DDMMYYYY
    if (input.length() != 8) {
        return "";
    }

    //Zerlegt das eingegebene Datum
    string day = input.substr(0, 2);
    string month = input.substr(2, 2);
    string year = input.substr(4, 4);

    return year + ":" + month + ":" + day + " 00:00:00";
    }

//*Zullast nur Ziffern, sonst Fehlermeldung
bool digits(const string& text) {

    if (text.empty())
        return false;

    for (char c : text) {

        if (!isdigit(c))
            return false;
    }

    return true;
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

//*Falls beim Pfad werde "" Angegeben, löscht die.
string removeQuotes(string text) {

    text.erase(
        remove(text.begin(), text.end(), '"'),
        text.end()
    );

    return text;
}