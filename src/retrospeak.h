// =====================================================================
//  RetroSpeak - an early-80s style speech synthesiser (+ a retro-future
//  one that never existed). Text -> phonemes (letter-to-sound rules) ->
//  sound (formants rung by a buzz, noise for the consonants).
//  Pure C++ (no platform code): main_win.cpp is the Windows app, main_cli.cpp
//  a command line build for Linux.
// =====================================================================
#pragma once
#include <string>
#include <vector>

enum RsEngine { RS_1982 = 0, RS_FUTURE = 1 };

struct RsVoice {
    int   engine = RS_1982;
    bool  female = false;
    float pitch = 0;          // semitones up / down
    float speed = 1.0f;       // 1 = as made; 2 = twice as fast
    int   bits = 6;           // (1982) the chip's output bits: 4 .. 8
};

// English text -> phonemes ("M IH1 S AX L Z ..."; "_" a short pause, "__" a long one, "?" a question's end).
// Text in [ ] is taken as phonemes already.
std::string rsTextToPhonemes(const std::string& text);
// phonemes -> sound: mono, 22050 Hz, -1..1
std::vector<float> rsSpeak(const std::string& phonemes, const RsVoice& v);
const int RS_RATE = 22050;
// a 16-bit mono WAV (in memory - to save it, or to play it)
std::vector<unsigned char> rsWav(const std::vector<float>& s, int rate = RS_RATE);
// the phonemes it knows (for the help text)
const char* rsPhonemeHelp();
