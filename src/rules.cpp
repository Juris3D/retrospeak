// =====================================================================
//  RetroSpeak - text to phonemes
//  English spelling -> phonemes by LETTER-TO-SOUND RULES: the classic
//  set from the US Naval Research Laboratory (Elovitz, Johnson, McHugh,
//  Shore, "Automatic translation of English text to phonetics by means
//  of letter-to-sound rules", NRL Report 7948, 1976 - a public document;
//  the rules SAM's and many early-80s speech systems' "reciters" were
//  built on). + a small dictionary of words the rules get wrong, numbers
//  read out, and a simple stress guess.
//  Each rule: LEFT context, the letters it MATCHES, RIGHT context, the
//  phonemes. Context symbols:
//    ' ' a word boundary      '#' one or more vowels    ':' zero or more consonants
//    '^' one consonant        '.' a voiced consonant    '+' a front vowel (E I Y)
//    '%' a suffix (ER E ES ED ING ELY)   '&' a sibilant   '@' T S R D L Z N J (TH CH SH)
// =====================================================================
#include "retrospeak.h"
#include <cctype>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {
struct Rule { const char* left; const char* match; const char* right; const char* out; };
// (the NRL set, in its order: the first that matches wins)
const Rule RULES[] = {
    // A
    {"", "A", " ", "AX"}, {" ", "ARE", " ", "AA R"}, {" ", "AR", "O", "AX R"}, {"", "AR", "#", "EH R"},
    {"^", "AS", "#", "EY S"}, {"", "A", "WA", "AX"}, {"", "AW", "", "AO"}, {" :", "ANY", "", "EH N IY"},
    {"", "A", "^+#", "EY"}, {"#:", "ALLY", "", "AX L IY"}, {" ", "AL", "#", "AX L"}, {"", "AGAIN", "", "AX G EH N"},
    {"#:", "AG", "E", "IH JH"}, {"", "A", "^+:#", "AE"}, {" :", "A", "^+ ", "EY"}, {"", "A", "^%", "EY"},
    {" ", "ARR", "", "AX R"}, {"", "ARR", "", "AE R"}, {" :", "AR", " ", "AA R"}, {"", "AR", " ", "ER"},
    {"", "AR", "", "AA R"}, {"", "AIR", "", "EH R"}, {"", "AI", "", "EY"}, {"", "AY", "", "EY"}, {"", "AU", "", "AO"},
    {"#:", "AL", " ", "AX L"}, {"#:", "ALS", " ", "AX L Z"}, {"", "ALK", "", "AO K"}, {"", "AL", "^", "AO L"},
    {" :", "ABLE", "", "EY B AX L"}, {"", "ABLE", "", "AX B AX L"}, {"", "ANG", "+", "EY N JH"}, {"", "A", "", "AE"},
    // B
    {" ", "BE", "^#", "B IH"}, {"", "BEING", "", "B IY IH NG"}, {" ", "BOTH", " ", "B OW TH"}, {" ", "BUS", "#", "B IH Z"},
    {"", "BUIL", "", "B IH L"}, {"", "B", "", "B"},
    // C
    {" ", "CH", "^", "K"}, {"^E", "CH", "", "K"}, {"", "CH", "", "CH"}, {" S", "CI", "#", "S AY"}, {"", "CI", "A", "SH"},
    {"", "CI", "O", "SH"}, {"", "CI", "EN", "SH"}, {"", "C", "+", "S"}, {"", "CK", "", "K"}, {"", "COM", "%", "K AH M"},
    {"", "C", "", "K"},
    // D
    {"#:", "DED", " ", "D IH D"}, {".E", "D", " ", "D"}, {"#:^E", "D", " ", "T"}, {" ", "DE", "^#", "D IH"},
    {" ", "DO", " ", "D UW"}, {" ", "DOES", "", "D AH Z"}, {" ", "DOING", "", "D UW IH NG"}, {" ", "DOW", "", "D AW"},
    {"", "DU", "A", "JH UW"}, {"", "D", "", "D"},
    // E
    {"#:", "E", " ", ""}, {"':^", "E", " ", ""}, {" :", "E", " ", "IY"}, {"#", "ED", " ", "D"}, {"#:", "E", "D ", ""},
    {"", "EV", "ER", "EH V"}, {"", "E", "^%", "IY"}, {"", "ERI", "#", "IY R IY"}, {"", "ERI", "", "EH R IH"},
    {"#:", "ER", "#", "ER"}, {"", "ER", "#", "EH R"}, {"", "ER", "", "ER"}, {" ", "EVEN", "", "IY V EH N"},
    {"#:", "E", "W", ""}, {"@", "EW", "", "UW"}, {"", "EW", "", "Y UW"}, {"", "E", "O", "IY"},
    {"#:&", "ES", " ", "IH Z"}, {"#:", "E", "S ", ""}, {"#:", "ELY", " ", "L IY"}, {"#:", "EMENT", "", "M EH N T"},
    {"", "EFUL", "", "F UH L"}, {"", "EE", "", "IY"}, {"", "EARN", "", "ER N"}, {" ", "EAR", "^", "ER"},
    {"", "EAD", "", "EH D"}, {"#:", "EA", " ", "IY AX"}, {"", "EA", "SU", "EH"}, {"", "EA", "", "IY"},
    {"", "EIGH", "", "EY"}, {"", "EI", "", "IY"}, {" ", "EYE", "", "AY"}, {"", "EY", "", "IY"}, {"", "EU", "", "Y UW"},
    {"", "E", "", "EH"},
    // F
    {"", "FUL", "", "F UH L"}, {"", "F", "", "F"},
    // G
    {"", "GIV", "", "G IH V"}, {" ", "G", "I^", "G"}, {"", "GE", "T", "G EH"}, {"SU", "GGES", "", "G JH EH S"},
    {"", "GG", "", "G"}, {" B#", "G", "", "G"}, {"", "G", "+", "JH"}, {"", "GREAT", "", "G R EY T"}, {"#", "GH", "", ""},
    {"", "G", "", "G"},
    // H
    {" ", "HAV", "", "HH AE V"}, {" ", "HERE", "", "HH IY R"}, {" ", "HOUR", "", "AW ER"}, {"", "HOW", "", "HH AW"},
    {"", "H", "#", "HH"}, {"", "H", "", ""},
    // I
    {" ", "IN", "", "IH N"}, {" ", "I", " ", "AY"}, {"", "IN", "D", "AY N"}, {"", "IER", "", "IY ER"},
    {"#:R", "IED", "", "IY D"}, {"", "IED", " ", "AY D"}, {"", "IEN", "", "IY EH N"}, {"", "IE", "T", "AY EH"},
    {" :", "I", "%", "AY"}, {"", "I", "%", "IY"}, {"", "IE", "", "IY"}, {"", "I", "^+:#", "IH"}, {"", "IR", "#", "AY R"},
    {"", "IZ", "%", "AY Z"}, {"", "IS", "%", "AY Z"}, {"", "I", "D%", "AY"}, {"+^", "I", "^+", "IH"},
    {"", "I", "T%", "AY"}, {"#:^", "I", "^+", "IH"}, {"", "I", "^+", "AY"}, {"", "IR", "", "ER"}, {"", "IGH", "", "AY"},
    {"", "ILD", "", "AY L D"}, {"", "IGN", " ", "AY N"}, {"", "IGN", "^", "AY N"}, {"", "IGN", "%", "AY N"},
    {"", "IQUE", "", "IY K"}, {"", "I", "", "IH"},
    // J, K
    {"", "J", "", "JH"}, {" ", "K", "N", ""}, {"", "K", "", "K"},
    // L
    {"", "LO", "C#", "L OW"}, {"L", "L", "", ""}, {"#:^", "L", "%", "AX L"}, {"", "LEAD", "", "L IY D"}, {"", "L", "", "L"},
    // M
    {"", "MOV", "", "M UW V"}, {"", "M", "", "M"},
    // N
    {"E", "NG", "+", "N JH"}, {"", "NG", "R", "NG G"}, {"", "NG", "#", "NG G"}, {"", "NGL", "%", "NG G AX L"},
    {"", "NG", "", "NG"}, {"", "NK", "", "NG K"}, {" ", "NOW", " ", "N AW"}, {"", "N", "", "N"},
    // O
    {"", "OF", " ", "AX V"}, {"", "OROUGH", "", "ER OW"}, {"#:", "OR", " ", "ER"}, {"#:", "ORS", " ", "ER Z"},
    {"", "OR", "", "AO R"}, {" ", "ONE", "", "W AH N"}, {"", "OW", "", "OW"}, {" ", "OVER", "", "OW V ER"},
    {"", "OV", "", "AH V"}, {"", "O", "^%", "OW"}, {"", "O", "^EN", "OW"}, {"", "O", "^I#", "OW"}, {"", "OL", "D", "OW L"},
    {"", "OUGHT", "", "AO T"}, {"", "OUGH", "", "AH F"}, {" ", "OU", "", "AW"}, {"H", "OU", "S#", "AW"},
    {"", "OUS", "", "AX S"}, {"", "OUR", "", "AO R"}, {"", "OULD", "", "UH D"}, {"^", "OU", "^L", "AH"},
    {"", "OUP", "", "UW P"}, {"", "OU", "", "AW"}, {"", "OY", "", "OY"}, {"", "OING", "", "OW IH NG"}, {"", "OI", "", "OY"},
    {"", "OOR", "", "AO R"}, {"", "OOK", "", "UH K"}, {"", "OOD", "", "UH D"}, {"", "OO", "", "UW"}, {"", "O", "E", "OW"},
    {"", "O", " ", "OW"}, {"", "OA", "", "OW"}, {" ", "ONLY", "", "OW N L IY"}, {" ", "ONCE", "", "W AH N S"},
    {"", "ON'T", "", "OW N T"}, {"C", "O", "N", "AA"}, {"", "O", "NG", "AO"}, {" :^", "O", "N", "AH"},
    {"I", "ON", "", "AX N"}, {"#:", "ON", " ", "AX N"}, {"#^", "ON", "", "AX N"}, {"", "O", "ST ", "OW"},
    {"", "OF", "^", "AO F"}, {"", "OTHER", "", "AH DH ER"}, {"", "OSS", " ", "AO S"}, {"#:^", "OM", "", "AH M"},
    {"", "O", "", "AA"},
    // P
    {"", "PH", "", "F"}, {"", "PEOP", "", "P IY P"}, {"", "POW", "", "P AW"}, {"", "PUT", " ", "P UH T"}, {"", "P", "", "P"},
    // Q
    {"", "QUAR", "", "K W AO R"}, {"", "QU", "", "K W"}, {"", "Q", "", "K"},
    // R
    {" ", "RE", "^#", "R IY"}, {"", "R", "", "R"},
    // S
    {"", "SH", "", "SH"}, {"#", "SION", "", "ZH AX N"}, {"", "SOME", "", "S AH M"}, {"#", "SUR", "#", "ZH ER"},
    {"", "SUR", "#", "SH ER"}, {"#", "SU", "#", "ZH UW"}, {"#", "SSU", "#", "SH UW"}, {"#", "SED", " ", "Z D"},
    {"#", "S", "#", "Z"}, {"", "SAID", "", "S EH D"}, {"^", "SION", "", "SH AX N"}, {"", "S", "S", ""},
    {".", "S", " ", "Z"}, {"#:.E", "S", " ", "Z"}, {"#:^##", "S", " ", "Z"}, {"#:^#", "S", " ", "S"}, {"U", "S", " ", "S"},
    {" :#", "S", " ", "Z"}, {" ", "SCH", "", "S K"}, {"", "S", "C+", ""}, {"#", "SM", "", "Z M"}, {"#", "SN", "'", "Z AX N"},
    {"", "S", "", "S"},
    // T
    {" ", "THE", " ", "DH AX"}, {"", "TO", " ", "T UW"}, {"", "THAT", " ", "DH AE T"}, {" ", "THIS", " ", "DH IH S"},
    {" ", "THEY", "", "DH EY"}, {" ", "THERE", "", "DH EH R"}, {"", "THER", "", "DH ER"}, {"", "THEIR", "", "DH EH R"},
    {" ", "THAN", " ", "DH AE N"}, {" ", "THEM", " ", "DH EH M"}, {"", "THESE", " ", "DH IY Z"}, {" ", "THEN", "", "DH EH N"},
    {"", "THROUGH", "", "TH R UW"}, {"", "THOSE", "", "DH OW Z"}, {"", "THOUGH", " ", "DH OW"}, {" ", "THUS", "", "DH AH S"},
    {"", "TH", "", "TH"}, {"#:", "TED", " ", "T IH D"}, {"S", "TI", "#N", "CH"}, {"", "TI", "O", "SH"}, {"", "TI", "A", "SH"},
    {"", "TIEN", "", "SH AX N"}, {"", "TUR", "#", "CH ER"}, {"", "TU", "A", "CH UW"}, {" ", "TWO", "", "T UW"}, {"", "T", "", "T"},
    // U
    {" ", "UN", "I", "Y UW N"}, {" ", "UN", "", "AH N"}, {" ", "UPON", "", "AX P AO N"}, {"@", "UR", "#", "UH R"},
    {"", "UR", "#", "Y UH R"}, {"", "UR", "", "ER"}, {"", "U", "^ ", "AH"}, {"", "U", "^^", "AH"}, {"", "UY", "", "AY"},
    {" G", "U", "#", ""}, {"G", "U", "%", ""}, {"G", "U", "#", "W"}, {"#N", "U", "", "Y UW"}, {"@", "U", "", "UW"},
    {"", "U", "", "Y UW"},
    // V
    {"", "VIEW", "", "V Y UW"}, {"", "V", "", "V"},
    // W
    {" ", "WERE", "", "W ER"}, {"", "WA", "S", "W AA"}, {"", "WA", "T", "W AA"}, {"", "WHERE", "", "W EH R"},
    {"", "WHAT", "", "W AA T"}, {"", "WHOL", "", "HH OW L"}, {"", "WHO", "", "HH UW"}, {"", "WH", "", "W"},
    {"", "WAR", "", "W AO R"}, {"", "WOR", "^", "W ER"}, {"", "WR", "", "R"}, {"", "W", "", "W"},
    // X
    {"", "X", "", "K S"},
    // Y
    {"", "YOUNG", "", "Y AH NG"}, {" ", "YOU", "", "Y UW"}, {" ", "YES", "", "Y EH S"}, {" ", "Y", "", "Y"},
    {"#:^", "Y", " ", "IY"}, {"#:^", "Y", "I", "IY"}, {" :", "Y", " ", "AY"}, {" :", "Y", "#", "AY"},
    {" :", "Y", "^+:#", "IH"}, {" :", "Y", "^#", "AY"}, {"", "Y", "", "IH"},
    // Z
    {"", "Z", "", "Z"},
};

bool isVowel(char c) { return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U'; }
bool isCons(char c) { return c >= 'A' && c <= 'Z' && !isVowel(c); }
bool isVoiced(char c) { return strchr("BDVGJLMNRWZ", c) && c; }
bool isFront(char c) { return c == 'E' || c == 'I' || c == 'Y'; }
bool isLetter(char c) { return (c >= 'A' && c <= 'Z') || c == '\''; }

// the LEFT context: read the pattern backwards from just before the match
bool leftOk(const std::string& w, int pos, const char* pat) {
    int i = pos - 1;
    for (int k = (int)strlen(pat) - 1; k >= 0; k--) {
        const char p = pat[k]; const char c = i >= 0 ? w[i] : ' ';
        switch (p) {
        case ' ': if (isLetter(c)) return false; i--; break;
        case '#': if (!isVowel(c)) return false; while (i >= 0 && isVowel(w[i])) i--; break;
        case ':': while (i >= 0 && isCons(w[i])) i--; break;
        case '^': if (!isCons(c)) return false; i--; break;
        case '.': if (!isVoiced(c)) return false; i--; break;
        case '+': if (!isFront(c)) return false; i--; break;
        case '&': if (strchr("SCGZXJ", c) && c) { i--; break; }
                  if (i >= 1 && w[i] == 'H' && (w[i - 1] == 'C' || w[i - 1] == 'S')) { i -= 2; break; } return false;
        case '@': if (strchr("TSRDLZNJ", c) && c) { i--; break; }
                  if (i >= 1 && w[i] == 'H' && strchr("TCS", w[i - 1])) { i -= 2; break; } return false;
        default: if (c != p) return false; i--;
        }
    }
    return true;
}
// the RIGHT context: from just after the match
bool rightOk(const std::string& w, int pos, const char* pat) {
    int i = pos; const int n = (int)w.size();
    for (int k = 0; pat[k]; k++) {
        const char p = pat[k]; const char c = i < n ? w[i] : ' ';
        switch (p) {
        case ' ': if (isLetter(c)) return false; i++; break;
        case '#': if (!isVowel(c)) return false; while (i < n && isVowel(w[i])) i++; break;
        case ':': while (i < n && isCons(w[i])) i++; break;
        case '^': if (!isCons(c)) return false; i++; break;
        case '.': if (!isVoiced(c)) return false; i++; break;
        case '+': if (!isFront(c)) return false; i++; break;
        case '&': if (strchr("SCGZXJ", c) && c) { i++; break; }
                  if (i + 1 < n && (c == 'C' || c == 'S') && w[i + 1] == 'H') { i += 2; break; } return false;
        case '@': if (strchr("TSRDLZNJ", c) && c) { i++; break; }
                  if (i + 1 < n && strchr("TCS", c) && w[i + 1] == 'H') { i += 2; break; } return false;
        case '%': {                                                  // a suffix: ER E ES ED ING ELY
            static const char* SUF[] = {"ING", "ELY", "ER", "ES", "ED", "E"};
            bool hit = false;
            for (const char* s : SUF) { const int L = (int)strlen(s);
                if (w.compare(i, L, s) == 0 && !(i + L < n && isLetter(w[i + L]))) { i += L; hit = true; break; } }
            if (!hit) return false; break; }
        default: if (c != p) return false; i++;
        }
    }
    return true;
}

// words the rules say wrong (and a few the game uses)
const std::map<std::string, std::string>& dict() {
    static const std::map<std::string, std::string> D = {
        {"A", "AX"}, {"AN", "AE N"}, {"AND", "AE N D"}, {"ARE", "AA R"}, {"AS", "AE Z"}, {"BE", "B IY"}, {"BY", "B AY"},
        {"FOR", "F AO R"}, {"FROM", "F R AH M"}, {"HAVE", "HH AE V"}, {"HE", "HH IY"}, {"IS", "IH Z"}, {"IT", "IH T"},
        {"ME", "M IY"}, {"MY", "M AY"}, {"NO", "N OW"}, {"OF", "AX V"}, {"ON", "AA N"}, {"OR", "AO R"}, {"SHE", "SH IY"},
        {"SO", "S OW"}, {"THE", "DH AX"}, {"TO", "T UW"}, {"WAS", "W AA Z"}, {"WE", "W IY"}, {"WITH", "W IH DH"},
        {"YOU", "Y UW"}, {"YOUR", "Y AO R"}, {"ONE", "W AH N"}, {"TWO", "T UW"}, {"EIGHT", "EY T"}, {"ZERO", "Z IY R OW"},
        {"SEVEN", "S EH V AX N"}, {"ELEVEN", "IH L EH V AX N"}, {"WARNING", "W AO R N IH NG"}, {"MISSILE", "M IH S AX L"},
        {"MISSILES", "M IH S AX L Z"}, {"RELOADED", "R IY L OW D IH D"}, {"REPAIRED", "R IY P EH R D"},
        {"CRITICAL", "K R IH T IH K AX L"}, {"DAMAGE", "D AE M IH JH"}, {"SECTOR", "S EH K T ER"}, {"CLEAR", "K L IY R"},
        {"SYSTEM", "S IH S T AX M"}, {"SYSTEMS", "S IH S T AX M Z"}, {"COMPUTER", "K AX M P Y UW T ER"},
        {"ROBOT", "R OW B AA T"}, {"HELLO", "HH EH L OW"}, {"ENEMY", "EH N AX M IY"}, {"TARGET", "T AA R G IH T"},
        {"LOCKED", "L AA K T"}, {"SHIELD", "SH IY L D"}, {"SHIELDS", "SH IY L D Z"}, {"ENERGY", "EH N ER JH IY"},
        {"LASER", "L EY Z ER"}, {"OWL", "AW L"}, {"THUNDER", "TH AH N D ER"}, {"VELOCITY", "V AX L AA S AX T IY"},
        {"ONLINE", "AA N L AY N"}, {"OFFLINE", "AO F L AY N"}, {"POWER", "P AW ER"}, {"LOW", "L OW"}, {"FUEL", "F Y UW AX L"},
        {"ALERT", "AX L ER T"}, {"MISSION", "M IH SH AX N"}, {"COMPLETE", "K AX M P L IY T"}, {"WELCOME", "W EH L K AX M"},
        {"PILOT", "P AY L AX T"}, {"READY", "R EH D IY"}, {"SPEECH", "S P IY CH"}, {"VOICE", "V OY S"}, {"RETRO", "R EH T R OW"},
        {"FUTURE", "F Y UW CH ER"}, {"PLEASE", "P L IY Z"}, {"SAY", "S EY"}, {"WHAT", "W AH T"}, {"DOES", "D AH Z"},
        {"GAME", "G EY M"}, {"OVER", "OW V ER"}, {"INCOMING", "IH N K AH M IH NG"}, {"ENGAGE", "EH N G EY JH"},
        {"HULL", "HH AH L"}, {"ARMOR", "AA R M ER"}, {"ARMOUR", "AA R M ER"}, {"SPEED", "S P IY D"}, {"ALTITUDE", "AE L T IH T UW D"},
        {"TURRET", "T ER IH T"}, {"BROWN", "B R AW N"}, {"DOWN", "D AW N"}, {"TOWN", "T AW N"}, {"CROWN", "K R AW N"}, {"COW", "K AW"},
        {"WOW", "W AW"}, {"ALLOW", "AX L AW"}, {"SOFTWARE", "S AO F T W EH R"}, {"HARDWARE", "HH AA R D W EH R"}, {"WANT", "W AA N T"},
        {"OK", "OW K EY"}, {"OKAY", "OW K EY"}, {"PLAY", "P L EY"}, {"GOOD", "G UH D"}, {"BYE", "B AY"}, {"GOODBYE", "G UH D B AY"}, {"TURRETS", "T ER IH T S"}, {"TRAIN", "T R EY N"}, {"RIVER", "R IH V ER"},
    };
    return D;
}

std::string numberWords(long long n) {
    static const char* ONES[] = {"ZERO", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE", "TEN", "ELEVEN",
                                 "TWELVE", "THIRTEEN", "FOURTEEN", "FIFTEEN", "SIXTEEN", "SEVENTEEN", "EIGHTEEN", "NINETEEN"};
    static const char* TENS[] = {"", "", "TWENTY", "THIRTY", "FORTY", "FIFTY", "SIXTY", "SEVENTY", "EIGHTY", "NINETY"};
    if (n < 0) return "MINUS " + numberWords(-n);
    if (n < 20) return ONES[n];
    if (n < 100) return std::string(TENS[n / 10]) + (n % 10 ? std::string(" ") + ONES[n % 10] : "");
    if (n < 1000) return std::string(ONES[n / 100]) + " HUNDRED" + (n % 100 ? " " + numberWords(n % 100) : "");
    if (n < 1000000) return numberWords(n / 1000) + " THOUSAND" + (n % 1000 ? " " + numberWords(n % 1000) : "");
    if (n < 1000000000LL) return numberWords(n / 1000000) + " MILLION" + (n % 1000000 ? " " + numberWords(n % 1000000) : "");
    return numberWords(n / 1000000000LL) + " BILLION" + (n % 1000000000LL ? " " + numberWords(n % 1000000000LL) : "");
}

bool isVowelPh(const std::string& p) {
    static const char* VS[] = {"IY", "IH", "EY", "EH", "AE", "AA", "AO", "OW", "UH", "UW", "ER", "AX", "AH", "AY", "AW", "OY"};
    for (const char* v : VS) if (p == v) return true; return false;
}

// one word (upper case letters / apostrophes) -> phonemes, with a STRESS mark on one vowel (a guess: the first, unless it's
// an unstressed prefix; function words none)
std::string wordPhonemes(const std::string& word) {
    std::string out;
    auto it = dict().find(word);
    if (it != dict().end()) out = it->second;
    else {
        const std::string w = " " + word + " ";
        int i = 1; const int n = (int)w.size() - 1;
        while (i < n) {
            const char c = w[i]; bool hit = false;
            for (const Rule& r : RULES) {
                if (r.match[0] != c) continue;
                const int L = (int)strlen(r.match);
                if (w.compare(i, L, r.match) != 0) continue;
                if (!leftOk(w, i, r.left) || !rightOk(w, i + L, r.right)) continue;
                if (r.out[0]) { if (!out.empty()) out += ' '; out += r.out; }
                i += L; hit = true; break;
            }
            if (!hit) i++;                                         // (no rule for it: skipped - an apostrophe, say)
        }
    }
    static const char* FUNC[] = {"A", "AN", "THE", "OF", "TO", "AND", "OR", "IN", "ON", "AT", "BY", "FOR", "IS", "IT", "AS", "BE",
                                 "ARE", "WAS", "WITH", "FROM", "YOUR", "MY", "WE", "HE", "SHE", "THEY", "BUT", "THAT", "THIS", "IF"};
    for (const char* f : FUNC) if (word == f) return out;
    // the stressed vowel
    std::vector<std::string> ph; { size_t a = 0; while (a < out.size()) { size_t b = out.find(' ', a); if (b == std::string::npos) b = out.size(); if (b > a) ph.push_back(out.substr(a, b - a)); a = b + 1; } }
    int vowels = 0; for (auto& p : ph) vowels += isVowelPh(p);
    int want = 0;
    if (vowels > 1) {
        static const char* PRE[] = {"RE", "DE", "BE", "A", "CON", "COM", "EX", "PRE", "IN", "UN", "DIS", "E"};
        for (const char* p : PRE) if (word.compare(0, strlen(p), p) == 0 && word.size() > strlen(p) + 3 && it == dict().end()) { want = 1; break; }
    }
    // (the dictionary's own words: the first vowel unless a known second-syllable stress)
    static const std::map<std::string, int> STR = {{"RELOADED", 1}, {"REPAIRED", 1}, {"ALERT", 1}, {"COMPLETE", 1}, {"COMPUTER", 1},
        {"ENGAGE", 1}, {"VELOCITY", 1}, {"ELEVEN", 1}, {"AGAIN", 1}, {"HELLO", 1}};
    auto si = STR.find(word); if (si != STR.end()) want = si->second;
    int k = 0;
    for (auto& p : ph) if (isVowelPh(p)) { if (k == want) { p += "1"; break; } k++; }
    std::string r; for (auto& p : ph) { if (!r.empty()) r += ' '; r += p; }
    return r;
}
}

// TEXT -> phonemes: words, numbers read out, punctuation as pauses ("," a short one, ". ! ?" a long one - "?" kept: the
// retro-future voice raises its pitch for it); [ ... ] = phonemes written by hand, passed as they are
std::string rsTextToPhonemes(const std::string& text) {
    std::string out; auto put = [&](const std::string& s) { if (s.empty()) return; if (!out.empty()) out += ' '; out += s; };
    size_t i = 0; const size_t n = text.size();
    while (i < n) {
        const unsigned char c = (unsigned char)text[i];
        if (c == '[') { size_t e = text.find(']', i); if (e == std::string::npos) e = n; std::string ph = text.substr(i + 1, e - i - 1);
            for (char& q : ph) q = (char)toupper((unsigned char)q); put(ph); i = e + 1; continue; }
        if (isalpha(c) || c == '\'') {
            std::string w; while (i < n && (isalpha((unsigned char)text[i]) || text[i] == '\'')) w += (char)toupper((unsigned char)text[i++]);
            put(wordPhonemes(w)); continue; }
        if (isdigit(c)) {
            long long v = 0; int digits = 0; while (i < n && isdigit((unsigned char)text[i]) && digits < 15) { v = v * 10 + (text[i++] - '0'); digits++; }
            std::string words = numberWords(v); size_t a = 0;
            while (a < words.size()) { size_t b = words.find(' ', a); if (b == std::string::npos) b = words.size(); put(wordPhonemes(words.substr(a, b - a))); a = b + 1; }
            continue; }
        if (c == ',' || c == ';' || c == ':' || c == '-') put("_");
        else if (c == '.' || c == '!') put("__");
        else if (c == '?') put("?");
        else if (c == '\n') put("__");
        i++;
    }
    // a consonant said twice in a row ("want to", "command") is said once
    std::vector<std::string> t; { size_t a = 0; while (a < out.size()) { size_t b = out.find(' ', a); if (b == std::string::npos) b = out.size(); if (b > a) t.push_back(out.substr(a, b - a)); a = b + 1; } }
    std::string r;
    for (size_t k = 0; k < t.size(); k++) {
        if (k > 0 && t[k] == t[k - 1] && !isVowelPh(t[k]) && t[k] != "_" && t[k] != "__" && t[k] != "?") continue;
        if (!r.empty()) r += ' '; r += t[k];
    }
    return r;
}
