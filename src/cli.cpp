// RetroSpeak - the command line (both builds): retrospeak "text" -o out.wav [--female] [--future] [--pitch n] [--speed x] [--bits n] [--phonemes "..."] [--show]
#include "retrospeak.h"
#include <cstdio>
#include <cstdlib>
#include <string>

int rsCli(int argc, char** argv) {
    std::string text, ph, out = "retrospeak.wav"; RsVoice v; bool show = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
        if (a == "-o") out = next(); else if (a == "--female") v.female = true; else if (a == "--future") v.engine = RS_FUTURE;
        else if (a == "--pitch") v.pitch = (float)atof(next().c_str()); else if (a == "--speed") v.speed = (float)atof(next().c_str());
        else if (a == "--bits") v.bits = atoi(next().c_str()); else if (a == "--phonemes") ph = next(); else if (a == "--show") show = true;
        else if (a == "-h" || a == "--help" || a == "/?") {
            printf("RetroSpeak - an early-80s speech synthesiser\n"
                   "  retrospeak \"text to say\" -o out.wav [--female] [--future] [--pitch semitones] [--speed 1.0] [--bits 6] [--show]\n"
                   "  retrospeak --phonemes \"HH EH L OW1\" -o out.wav\n  (no arguments: the window)\n\n%s\n", rsPhonemeHelp());
            return 0; }
        else text += (text.empty() ? "" : " ") + a;
    }
    if (ph.empty()) ph = rsTextToPhonemes(text);
    if (show) printf("%s\n", ph.c_str());
    auto s = rsSpeak(ph, v); auto w = rsWav(s);
    FILE* f = fopen(out.c_str(), "wb"); if (!f) { perror(out.c_str()); return 1; }
    fwrite(w.data(), 1, w.size(), f); fclose(f);
    printf("%s: %.2f s\n", out.c_str(), s.size() / (float)RS_RATE);
    return 0;
}
