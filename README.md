# RetroSpeak

An early-80s style speech synthesiser: type a line, hear it said by a talking computer of 1982 (think SAM on the
Commodore 64, the Votrax chips, Speak & Spell), and save it as a WAV. Plus a **retro-future** voice: the speech chip
of a 1987 that never happened.

Made for Thunder Owl's games (ThunderVelocity's cockpit voice) - but it's a tool of its own.

## Use it

**The window** - run `RetroSpeak.exe`:

- **Text to say** - type English. Numbers are read out ("3 fighters" - "three fighters"). `,` makes a short pause,
  `.` `!` a long one, `?` a question (the retro-future voice raises its pitch for it).
- **Phonemes** - what it's going to say, shown as you type. When a word comes out wrong, tick
  *Speak the phonemes as written* and fix it by hand. Or, in the text, put phonemes in brackets: `Hello [TH AH1 N D ER] Owl`.
- **Voice** - male or female robot. **Engine** - *1982 speech chip* or *Retro-future*.
- **Pitch** (semitones), **Speed**, **Bits** (the 1982 chip's output resolution: 3 = very crunchy, 8 = clean).
- **Speak** (F5, or Enter), **Stop**, **Save WAV...** (22050 Hz, 16-bit mono).

**The command line** - the same exe with arguments:

```
RetroSpeak.exe "Missiles reloaded." -o missiles.wav
RetroSpeak.exe "Damage critical." -o damage.wav --female --future --pitch -2 --speed 0.9
RetroSpeak.exe --phonemes "SH IH1 P  R IY P EH1 R D" -o ship.wav --bits 5
RetroSpeak.exe "Some text" --show -o x.wav      (prints the phonemes it used)
RetroSpeak.exe --help
```

## How it works

No recorded voice, no human voice model - the speech is **constructed**, the way the early-80s chips did it.

1. **Text to phonemes** - English spelling is turned into phonemes by **letter-to-sound rules**: the classic set from
   the US Naval Research Laboratory (Elovitz, Johnson, McHugh & Shore, *Automatic translation of English text to
   phonetics by means of letter-to-sound rules*, NRL Report 7948, 1976) - the rules the "reciters" of SAM and its
   generation were built on. Each rule is *left context, letters, right context -> phonemes* ("C before E, I or Y is S").
   A small dictionary fixes the words the rules get wrong; numbers are spelled out; the stress goes on a word's first
   vowel unless it starts with an unstressed prefix (re-, de-, con-...).
2. **Phonemes to frames** - every sound of speech is described by the **formants** of the mouth shaping it: its three
   lowest resonances F1 F2 F3 (an "ee" ~ 270, 2290, 3010 Hz; an "ah" ~ 730, 1090, 2440 Hz - the classic measurements of
   Peterson & Barney). Consonants are **noise** in a band (S: a hiss over 4 kHz, SH lower), a **closure** (silence - the
   T, the P) and a **burst**. The phonemes become 1 ms frames; the formant targets are smoothed so they **slide** from
   sound to sound.
3. **Frames to sound** - two engines:
   - **1982 speech chip** - a buzz at **one flat pitch** (a robot never sings); every pulse of it strikes three damped
     sine waves at the formant frequencies (how SAM did it - that ringing is the metallic voice), the noise for the
     consonants; then the chip: the sound **held at 11 kHz** (the aliasing is the "metal") and cut to **6 bits**.
   - **Retro-future** (fictional) - a softer **glottal pulse** (Rosenberg's shape) through a **cascade of five
     resonators** (Klatt's formant synthesiser - the big lab machines of the late 70s), the sounds blending longer, the
     highs lifted for clear vowels. It has a **melody** - but **stepped to semitones**, gliding from step to step: a
     machine singing its sentences; phrase ends lengthened, a question rises. A faint shimmer of the 1982 chip under it,
     a short chorus over it. Still a robot - just a later, smoother one.
   - **Female**: the buzz an octave up, the formants 17 % higher (a smaller vocal tract).

## Phonemes

```
VOWELS  IY beet   IH bit   EY bait   EH bet   AE bat   AA hot   AO bought   OW boat
        UH book   UW boot   ER bird   AX about   AH but   AY bite   AW bout   OY boy
        a 1 after a vowel stresses it: AE1
OTHERS  P B T D K G   F V TH DH S Z SH ZH HH   CH JH   M N NG   L R W Y
PAUSES  _ short   __ long   ? a question's end
```

## Build

`./build.sh` - MinGW-w64 (cross from Linux, or MSYS2 on Windows) makes `build/RetroSpeak.exe` (static, no DLLs);
g++ makes `build/retrospeak`, the command line for Linux. Pure C++17, no libraries; the window is plain Win32.

- `src/rules.cpp` - text to phonemes (the NRL rules, the dictionary, numbers, stress)
- `src/synth.cpp` - phonemes to sound (both engines), the WAV writer
- `src/main_win.cpp` - the Windows app; `src/cli.cpp` - the command line

`samples/` - a few lines in both engines, both voices.
