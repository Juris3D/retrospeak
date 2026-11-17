// =====================================================================
//  RetroSpeak - phonemes to sound
//  Every sound of speech is described by the FORMANTS of the vocal tract
//  shaping it - the three lowest resonances F1 F2 F3 (an "ee": 270, 2290,
//  3010 Hz; an "ah": 730, 1090, 2440) - or, for the consonants, by NOISE
//  in some band (an S: a hiss over 4 kHz; an SH: lower) and by SILENCE
//  (the closure of a T, a P...) and a BURST. The phonemes are stepped
//  through as 1 ms frames, the targets smoothed so they slide.
//
//  1982 CHIP (SAM on the C64 did it this way): a buzz at ONE flat pitch;
//    every pulse strikes three damped sine waves at the formants - the
//    metallic ring; then the chip: the sound held at 11 kHz (aliasing)
//    and cut to a few bits.
//  RETRO-FUTURE (fictional: the speech chip of a 1987 that never came):
//    a softer glottal pulse through a CASCADE of five resonators (as the
//    big lab synthesisers did), the sounds blending longer, a melody -
//    but STEPPED to semitones, gliding from step to step: a machine
//    singing its sentences; phrase ends lengthened, a question rises;
//    a thin metal shimmer of the old chip under it and a short chorus.
// =====================================================================
#include "retrospeak.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {
const float SR = (float)RS_RATE;
const float PI = 3.14159265f;

struct F3 { float a, b, c; };
const std::map<std::string, F3>& vowels() {
    static const std::map<std::string, F3> V = {
        {"IY", {270, 2290, 3010}}, {"IH", {390, 1990, 2550}}, {"EH", {530, 1840, 2480}}, {"AE", {660, 1720, 2410}},
        {"AA", {730, 1090, 2440}}, {"AO", {570, 840, 2410}}, {"UH", {440, 1020, 2240}}, {"UW", {300, 870, 2240}},
        {"AH", {520, 1190, 2390}}, {"ER", {490, 1350, 1690}}, {"AX", {500, 1500, 2500}}};
    return V;
}
struct Son { F3 f; float amp; };
const std::map<std::string, Son>& sonorants() {
    static const std::map<std::string, Son> S = {
        {"L", {{360, 1300, 2700}, 0.7f}}, {"R", {{330, 1060, 1380}, 0.75f}}, {"W", {{300, 610, 2200}, 0.7f}},
        {"Y", {{260, 2070, 3020}, 0.7f}}, {"M", {{280, 900, 2200}, 0.45f}}, {"N", {{280, 1700, 2600}, 0.45f}},
        {"NG", {{280, 2300, 2750}, 0.45f}}};
    return S;
}
struct Fric { float lo, hi, amp; bool voiced; bool hasLoc; F3 loc; };
const std::map<std::string, Fric>& fricatives() {
    static const std::map<std::string, Fric> M = {
        {"S", {3800, 8000, 0.55f, false, true, {320, 1700, 2600}}}, {"Z", {3800, 8000, 0.35f, true, true, {320, 1700, 2600}}},
        {"SH", {1700, 5200, 0.6f, false, true, {300, 1800, 2400}}}, {"ZH", {1700, 5200, 0.4f, true, true, {300, 1800, 2400}}},
        {"F", {1000, 7500, 0.22f, false, true, {330, 1100, 2400}}}, {"V", {1000, 7500, 0.18f, true, true, {330, 1100, 2400}}},
        {"TH", {1300, 7500, 0.2f, false, true, {330, 1400, 2500}}}, {"DH", {1300, 7500, 0.15f, true, true, {330, 1400, 2500}}},
        {"HH", {500, 3500, 0.3f, false, false, {0, 0, 0}}}};
    return M;
}
struct Stop { float lo, hi; bool voiced; F3 loc; };
const std::map<std::string, Stop>& stops() {
    static const std::map<std::string, Stop> M = {
        {"P", {300, 2500, false, {300, 900, 2200}}}, {"B", {300, 2500, true, {300, 900, 2200}}},
        {"T", {3000, 7500, false, {330, 1750, 2600}}}, {"D", {3000, 7500, true, {330, 1750, 2600}}},
        {"K", {1500, 3500, false, {300, 2000, 2600}}}, {"G", {1500, 3500, true, {300, 2000, 2600}}}};
    return M;
}
const std::map<std::string, std::pair<std::string, std::string>>& diphthongs() {
    static const std::map<std::string, std::pair<std::string, std::string>> D = {
        {"AY", {"AA", "IY"}}, {"EY", {"EH", "IY"}}, {"OW", {"AO", "UW"}}, {"AW", {"AA", "UW"}}, {"OY", {"AO", "IY"}}};
    return D;
}

struct Frame { float F[3]; float av, an; int band; bool st; bool pause; bool asp; int phrase; };
struct Band { float lo, hi; };

// the phonemes -> 1 ms frames
void build(const std::string& ph, float speed, bool future, std::vector<Frame>& fr, std::vector<Band>& bands, std::vector<bool>& question) {
    std::vector<std::string> tok; { size_t a = 0; while (a < ph.size()) { size_t b = ph.find(' ', a); if (b == std::string::npos) b = ph.size(); if (b > a) tok.push_back(ph.substr(a, b - a)); a = b + 1; } }
    auto bandOf = [&](float lo, float hi) { for (size_t i = 0; i < bands.size(); i++) if (bands[i].lo == lo && bands[i].hi == hi) return (int)i; bands.push_back({lo, hi}); return (int)bands.size() - 1; };
    // (retro-future: the last vowel of each phrase lengthened)
    std::vector<bool> finalV(tok.size(), false);
    if (future) { int last = -1;
        for (size_t i = 0; i <= tok.size(); i++) {
            const bool brk = i == tok.size() || tok[i] == "_" || tok[i] == "__" || tok[i] == "?";
            if (brk) { if (last >= 0) finalV[last] = true; last = -1; continue; }
            std::string p = tok[i]; if (!p.empty() && p.back() == '1') p.pop_back();
            if (vowels().count(p) || diphthongs().count(p)) last = (int)i; } }
    int phrase = 0; question.assign(1, false);
    const float k = 1.0f / std::max(0.3f, speed);
    F3 prevF = {500, 1500, 2500};
    auto add = [&](float ms, F3 F, float av, float an, int band, bool st, bool asp = false) {
        const int n = std::max(1, (int)lroundf(ms * k));
        for (int i = 0; i < n; i++) fr.push_back({{F.a, F.b, F.c}, av, an, band, st, false, asp, phrase});
        prevF = F; };
    auto pause = [&](float ms) { const int n = (int)lroundf(ms * k); for (int i = 0; i < n; i++) fr.push_back({{prevF.a, prevF.b, prevF.c}, 0, 0, -1, false, true, false, phrase}); };
    for (size_t i = 0; i < tok.size(); i++) {
        std::string p = tok[i]; bool st = false;
        if (!p.empty() && (p.back() == '1' || p.back() == '2')) { st = true; p.pop_back(); }
        if (p == "0") continue;
        const float vl = (finalV[i] ? 1.35f : 1.0f) * (future ? (st ? 1.0f : 0.85f) : 1.0f);
        if (p == "_") { pause(150); phrase++; question.push_back(false); continue; }
        if (p == "__") { pause(320); phrase++; question.push_back(false); continue; }
        if (p == "?") { question[phrase] = true; pause(320); phrase++; question.push_back(false); continue; }
        if (p == "WH") p = "W";
        if (vowels().count(p)) { add((st ? 175.0f : 105.0f) * vl, vowels().at(p), 1.0f, 0.0f, -1, st); continue; }
        if (diphthongs().count(p)) { auto d = diphthongs().at(p);
            add((st ? 110.0f : 70.0f) * vl, vowels().at(d.first), 1.0f, 0, -1, st); add((st ? 110.0f : 70.0f) * vl, vowels().at(d.second), 1.0f, 0, -1, st); continue; }
        if (sonorants().count(p)) { const Son& s = sonorants().at(p); add(75, s.f, s.amp, 0.0f, -1, false); continue; }
        if (p == "JH" || p == "CH") {
            const bool v = p == "JH"; const F3 L = {300, 1800, 2400};
            add(v ? 45 : 55, L, v ? 0.15f : 0.0f, 0.0f, -1, false);
            add(25, L, v ? 0.3f : 0.0f, v ? 0.5f : 0.7f, bandOf(2000, 6000), false);
            add(v ? 60 : 70, L, v ? 0.35f : 0.0f, v ? 0.4f : 0.55f, bandOf(1700, 5200), false); continue; }
        if (fricatives().count(p)) { const Fric& f = fricatives().at(p);
            const bool asp = p == "HH";
            add(f.voiced ? 95 : 120, f.hasLoc ? f.loc : prevF, f.voiced ? 0.4f : 0.0f, f.amp, bandOf(f.lo, f.hi), false, asp); continue; }
        if (stops().count(p)) { const Stop& s = stops().at(p);
            add(55, s.loc, s.voiced ? 0.12f : 0.0f, 0.0f, -1, false);                        // the closure (a voice bar if voiced)
            add(15, s.loc, 0.0f, s.voiced ? 0.6f : 0.9f, bandOf(s.lo, s.hi), false);          // the burst
            if (!s.voiced) add(35, s.loc, 0.0f, 0.3f, bandOf(500, 3500), false, true);          // its aspiration
            continue; }
        // (unknown: skipped)
    }
}

std::vector<float> smooth(const std::vector<float>& x, int w) {
    if (w <= 1 || x.empty()) return x;
    const int n = (int)x.size(), h = w / 2; std::vector<double> c(n + 1, 0.0);
    for (int i = 0; i < n; i++) c[i + 1] = c[i] + x[i];
    std::vector<float> y(n);
    for (int i = 0; i < n; i++) { const int a = std::max(0, i - h), b = std::min(n, i + h + 1);
        double s = c[b] - c[a]; s += (double)x[0] * std::max(0, h - i) + (double)x[n - 1] * std::max(0, i + h + 1 - n);   // (the edges held)
        y[i] = (float)(s / (2 * h + 1)); }
    return y;
}

struct Biquad { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float run(float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void band(float lo, float hi) { const float f0 = sqrtf(lo * hi), Q = f0 / std::max(1.0f, hi - lo), w = 2 * PI * std::min(f0, SR * 0.45f) / SR, al = sinf(w) / (2 * Q), a0 = 1 + al;
        b0 = al / a0; b1 = 0; b2 = -al / a0; a1 = -2 * cosf(w) / a0; a2 = (1 - al) / a0; }
    void low(float f) { const float w = 2 * PI * std::min(f, SR * 0.45f) / SR, al = sinf(w) / (2 * 0.7071f), a0 = 1 + al, c = cosf(w);
        b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = b0; a1 = -2 * c / a0; a2 = (1 - al) / a0; } };

struct Noise { uint32_t s = 0x9E3779B9u; float uni() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s >> 8) / 16777216.0f; }
    float operator()() { return (uni() + uni() + uni() + uni() - 2.0f) * 1.7f; } };

// the noise of every band used, mixed by the frames
std::vector<float> noiseTrack(const std::vector<Frame>& fr, const std::vector<Band>& bands, int n, bool skipAsp) {
    std::vector<float> out(n, 0.0f); const int nms = (int)fr.size();
    for (size_t b = 0; b < bands.size(); b++) {
        std::vector<float> m(nms, 0.0f); bool any = false;
        for (int i = 0; i < nms; i++) if (fr[i].band == (int)b && !(skipAsp && fr[i].asp)) { m[i] = 1; any = true; }
        if (!any) continue;
        m = smooth(m, 5);
        Noise nz; nz.s += (uint32_t)b * 7919u; Biquad f1, f2; f1.band(bands[b].lo, bands[b].hi); f2.band(bands[b].lo, bands[b].hi);
        for (int i = 0; i < n; i++) { const float v = f2.run(f1.run(nz())); out[i] += v * m[std::min(nms - 1, (int)(i * 1000.0f / SR))] * 0.6f; }
    }
    return out;
}

void normalize(std::vector<float>& y, float peak) { float m = 0; for (float v : y) m = std::max(m, fabsf(v)); if (m > 1e-9f) for (float& v : y) v *= peak / m; }
}

std::vector<float> rsSpeak(const std::string& phonemes, const RsVoice& v) {
    const bool future = v.engine == RS_FUTURE;
    std::vector<Frame> fr; std::vector<Band> bands; std::vector<bool> question;
    build(phonemes, v.speed, future, fr, bands, question);
    if (fr.empty()) return {};
    const int nms = (int)fr.size(), n = (int)(nms / 1000.0f * SR);
    const float fs = v.female ? 1.17f : 1.0f, pk = powf(2.0f, v.pitch / 12.0f);
    // the tracks (1 ms): formants slide, amplitudes ease
    std::vector<float> Ft[3], av(nms), an(nms), st(nms), pos(nms, 0);
    for (int k = 0; k < 3; k++) { Ft[k].resize(nms); for (int i = 0; i < nms; i++) Ft[k][i] = fr[i].F[k] * fs; }
    for (int i = 0; i < nms; i++) { av[i] = fr[i].av; an[i] = fr[i].an; st[i] = fr[i].st ? 1.0f : 0.0f; }
    for (int k = 0; k < 3; k++) { Ft[k] = smooth(Ft[k], future ? 40 : 28); if (future) Ft[k] = smooth(Ft[k], 24); }
    av = smooth(av, future ? 14 : 9); an = smooth(an, 5); st = smooth(st, future ? 70 : 40);
    { int a = 0; while (a < nms) { int b = a; while (b < nms && fr[b].phrase == fr[a].phrase) b++;   // where in its phrase (0..1)
        int s0 = a, s1 = b - 1; while (s0 < b && fr[s0].pause) s0++; while (s1 > s0 && fr[s1].pause) s1--;
        for (int i = a; i < b; i++) pos[i] = s1 > s0 ? std::clamp((float)(i - s0) / (s1 - s0), 0.0f, 1.0f) : 0.0f; a = b; } }
    // the PITCH (per ms)
    std::vector<float> f0(nms);
    if (!future) {
        const float a = (v.female ? 190.0f : 92.0f) * pk, b = (v.female ? 165.0f : 80.0f) * pk;
        for (int i = 0; i < nms; i++) f0[i] = (a + (b - a) * pos[i]) * (1.0f + 0.10f * st[i]);
    } else {
        const float base = (v.female ? 196.0f : 98.0f) * pk;
        for (int i = 0; i < nms; i++) {
            const bool q = fr[i].phrase < (int)question.size() && question[fr[i].phrase];
            float f = base * (1.08f - 0.18f * pos[i]) * (1.0f + 0.22f * st[i]);
            if (q && pos[i] > 0.6f) f *= 1.0f + 0.55f * (pos[i] - 0.6f) / 0.4f;
            const float semi = roundf(12.0f * log2f(f / base));            // STEPPED to semitones - the machine sings
            f0[i] = base * powf(2.0f, semi / 12.0f);
        }
        f0 = smooth(f0, 22);                                               // (gliding from step to step)
    }
    auto ms = [&](int i) { return std::min(nms - 1, (int)(i * 1000.0f / SR)); };
    std::vector<float> y(n, 0.0f);
    if (!future) {
        // ---- 1982: three damped sines struck by every pulse, the noise, then the chip ----
        const float B[3] = {90, 120, 180}, A[3] = {1.0f, 0.55f, 0.32f};
        double ph = 0; float tp = 0;
        std::vector<float> nz = noiseTrack(fr, bands, n, false);
        for (int i = 0; i < n; i++) {
            const int m = ms(i); const float f = f0[m];
            ph += f / SR; if (ph >= 1.0) { ph -= floor(ph); tp = 0; } else tp += 1.0f / SR;
            float vv = 0; for (int k = 0; k < 3; k++) vv += A[k] * sinf(2 * PI * Ft[k][m] * tp) * expf(-PI * B[k] * tp);
            y[i] = vv * av[m] * 0.55f + nz[i] * an[m] * 0.8f;
        }
        normalize(y, 1.0f);
        const float lv = (float)((1 << (std::clamp(v.bits, 3, 8) - 1)) - 1);
        for (int i = 0; i < n; i++) { const float h = y[i & ~1]; y[i] = roundf(h * lv) / lv; }   // held at 11 kHz, few bits
        Biquad lp; lp.low(7500); for (float& s : y) s = lp.run(s);
    } else {
        // ---- RETRO-FUTURE: a glottal pulse through five cascaded resonators ----
        std::vector<float> nz = noiseTrack(fr, bands, n, true);
        // the aspiration noise (HH, a stop's breath) goes through the vocal tract with the voice (it sounds of the vowel)
        std::vector<float> aspM(nms, 0.0f); for (int i = 0; i < nms; i++) aspM[i] = fr[i].asp ? fr[i].an : 0.0f; aspM = smooth(aspM, 6);
        const float F4 = 3300 * fs, F5 = 3900 * fs, BW[5] = {60, 90, 150, 250, 300};
        float r1[5] = {0}, r2[5] = {0}; double ph = 0; float prevG = 0, preX = 0; Noise an2; an2.s = 777;
        double ph2 = 0; float tp2 = 0;                                       // (the old chip's shimmer, faint)
        std::vector<float> shimmer(n, 0.0f);
        for (int i = 0; i < n; i++) {
            const int m = ms(i); const float f = f0[m];
            ph += f / SR; if (ph >= 1.0) ph -= floor(ph);
            // the Rosenberg pulse: opens over 40 % of the period, closes over 16 %, then shut - its slope (what the lips radiate)
            const float t = (float)ph; float g;
            if (t < 0.4f) g = 0.5f * (1 - cosf(PI * t / 0.4f)); else if (t < 0.56f) g = cosf(0.5f * PI * (t - 0.4f) / 0.16f); else g = 0;
            float src = (g - prevG) * 6.0f; prevG = g;
            src = src * av[m] + an2() * aspM[m] * 0.35f;
            float x = src;
            const float Fv[5] = {Ft[0][m], Ft[1][m], Ft[2][m], F4, F5};
            for (int k = 0; k < 5; k++) {                                   // Klatt's resonator: y = A x + B y1 + C y2
                const float C = -expf(-2 * PI * BW[k] / SR), Bc = 2 * expf(-PI * BW[k] / SR) * cosf(2 * PI * Fv[k] / SR), Ac = 1 - Bc - C;
                const float yy = Ac * x + Bc * r1[k] + C * r2[k]; r2[k] = r1[k]; r1[k] = yy; x = yy;
            }
            const float pe = x - 0.86f * preX; preX = x;                   // PRE-EMPHASIS: the highs lifted (crisp, clear vowels)
            y[i] = pe * 2.2f + nz[i] * an[m] * 0.36f;
            ph2 += f / SR; if (ph2 >= 1.0) { ph2 -= floor(ph2); tp2 = 0; } else tp2 += 1.0f / SR;
            float vv = 0; const float B[3] = {90, 120, 180}, A[3] = {1.0f, 0.55f, 0.32f};
            for (int k = 0; k < 3; k++) vv += A[k] * sinf(2 * PI * Ft[k][m] * tp2) * expf(-PI * B[k] * tp2);
            shimmer[i] = vv * av[m];
        }
        normalize(y, 1.0f); normalize(shimmer, 1.0f);
        for (int i = 0; i < n; i++) y[i] = y[i] * 0.86f + shimmer[i] * 0.16f;
        // a short CHORUS (a second voice, 7 +- 2 ms behind, wavering) - the "sheen"
        std::vector<float> z(y);
        for (int i = 0; i < n; i++) { const float d = (0.007f + 0.002f * sinf(2 * PI * 0.6f * i / SR)) * SR; const float p = i - d;
            if (p >= 1) { const int a = (int)p; const float fr2 = p - a; z[i] += 0.22f * (y[a] * (1 - fr2) + y[a + 1] * fr2); } }
        y.swap(z);
        normalize(y, 1.0f);
        for (float& s : y) s = tanhf(1.3f * s);
        Biquad lp; lp.low(9000); for (float& s : y) s = lp.run(s);
    }
    normalize(y, 0.95f);
    std::vector<float> out((size_t)(0.03f * SR), 0.0f); out.insert(out.end(), y.begin(), y.end()); out.resize(out.size() + (size_t)(0.05f * SR), 0.0f);
    return out;
}

std::vector<unsigned char> rsWav(const std::vector<float>& s, int rate) {
    std::vector<unsigned char> b; const uint32_t data = (uint32_t)s.size() * 2;
    auto u32 = [&](uint32_t v) { for (int i = 0; i < 4; i++) b.push_back((unsigned char)(v >> (8 * i))); };
    auto u16 = [&](uint16_t v) { b.push_back((unsigned char)v); b.push_back((unsigned char)(v >> 8)); };
    auto tag = [&](const char* t) { for (int i = 0; i < 4; i++) b.push_back((unsigned char)t[i]); };
    tag("RIFF"); u32(36 + data); tag("WAVE"); tag("fmt "); u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
    tag("data"); u32(data);
    for (float v : s) { const int q = (int)lroundf(std::clamp(v, -1.0f, 1.0f) * 32767.0f); u16((uint16_t)(int16_t)q); }
    return b;
}

const char* rsPhonemeHelp() {
    return "VOWELS  IY beet   IH bit   EY bait   EH bet   AE bat   AA hot   AO bought   OW boat\r\n"
           "        UH book   UW boot   ER bird   AX about   AH but   AY bite   AW bout   OY boy\r\n"
           "        a 1 after a vowel stresses it: AE1\r\n"
           "OTHERS  P B T D K G   F V TH DH S Z SH ZH HH   CH JH   M N NG   L R W Y\r\n"
           "PAUSES  _ short   __ long   ? a question's end.    In the text: [phonemes in brackets] are said as written.";
}
