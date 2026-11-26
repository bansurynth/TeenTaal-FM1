# TeenTaal — user manual

TeenTaal turns the FM-1 into a practice instrument for Indian classical music: a **tanpura**, **tabla** playing ten
taals, and a **reference tone** for intonation. Everything is played from one screen. At power-on everything is
silent; the saved settings come back as you left them.

> Version 1.1, AI-generated, tested only in emulators: see the [Disclaimer](../README.md#disclaimer). Updates may
> follow soon.

## The screen

```
 TEENTAAL                    UNSAVED     <- name; UNSAVED when there are changes not saved yet
 SA A3         TANPURA A2                <- your Sa; the tanpura's Sa
 Teentaal > Ektaal          120 /MIN     <- the taal (and the next one, when you changed it); the tempo
 9 /16         KHALI                     <- the matra playing now: red on sam, blue in khali
 X       2       0       3               <- the cycle: a cell for each matra, grouped by vibhag
 # . . . . . . . # . . . . . . .
 TANPURA ON    PA J75 V100               <- first string, jawari, volume
 TONE    OFF   SA V70                    <- the tone's swara, octave, volume
 CPU ...  TABLA V100
```

## Controls

| Control | What it does |
|---|---|
| **SEQ** | tanpura: both on / 2nd muted / 1st muted / both off |
| **PLAY** | tabla: start from the sam / stop (green light while playing) |
| **ARP** | reference tone on / off |
| **a key** | the tone on that key's swara **while you hold it** (like an harmonium) |
| **SEL + a key** | Sa becomes that key's note (the cents are kept) |
| **FX + a key from C4 to G4** | with both tanpuras off, pluck one string an octave below the key without changing Sa |
| **SEL + OCT− / OCT+** | Sa an octave down / up |
| **PRESETS** | Sa up or down by one cent per click |
| **SELECT** | the taal; while the tabla play, the new taal starts **at the end of the cycle** |
| **KNOB1** | tempo, 1 matra a minute per click (**SEL + KNOB1**: 10) |
| **KNOB2 / KNOB3 / KNOB4** | volume of tanpura / tabla / tone |
| **ALGORITHM** | jawari, the position of the cotton thread: 75 or 100. Clockwise sets 100, counter-clockwise 75 |
| **OCT− / OCT+** | the tone's octave (mandra, madhya, tar) |
| **MASTER** | overall volume |
| **EDIT** | the settings page (below); **HOME** goes back |
| **SAVE** | saves the settings (UNSAVED disappears) |

The keys light from your Sa upwards: **Sa and Pa bright**, the other shuddha swaras (Re, Ga, Ma, Dha, Ni) dim, so the
keyboard shows where the swaras are for the Sa you chose.

## Sa

The default Sa is **A3 (220 Hz)**, for a bansuri in A (the flute's Sa is an octave higher). For a bansuri in G: **SEL
+ the G key** (G3, 196 Hz). The 27 keys play F2 to G4, one note each. SEL + OCT− / OCT+ moves Sa by an octave; the cents are kept. The tanpura plays at the note shown (default A3). The dayan of the tabla tunes itself to Sa (to the octave between 190 and 380 Hz).

With both tanpuras off, hold FX and press a key from C4 to G4 to pluck one string on the first tanpura an octave below
that key, without changing Sa. If either tanpura is on, this FX chord does not preview; SEL + key always sets Sa.

The default tanpura cycle (5 seconds) plucks its first string, Sa, Sa and the lower kharaj at 0, 1.0, 1.75 and 2.5 seconds,
then pauses. With EXTRA STRING enabled, it plucks after the first string; the two Sa strings and kharaj follow at 1.75,
2.5 and 3.25 seconds. Both tanpuras start off. Each SEQ press moves to the next state: both tanpuras on; the second
muted (the first plays); the first muted (the second plays); both off. A muted tanpura keeps its cycle running, so it
comes back in time. While a tanpura is muted, the main page shows MUTE instead of ON. The second tanpura has its own
first string (NI by default), phase (20%) and optional extra string; the pluck cycle and the fine tuning of each string
are shared with the first tanpura.

## Settings (EDIT)

SELECT picks the row, PRESETS changes the value, HOME goes back. The dots at the bottom show where you are in the list.

| Row | What it sets |
|---|---|
| TEMPERAMENT | the tuning of the swaras: EQUAL (default, equal temperament), JUST 5 (5-limit just intonation) or JUST PYTH (Pythagorean just intonation). Sa does not change; the first and extra strings of both tanpuras and the reference tone follow the choice; the keyboard pluck (FX) stays equal |
| PLUCK CYCLE | the length of the plucking cycle, shared by both tanpuras: 2.5–60 s (steps of 0.1 s up to 10 s, then 1 s) |
| FIRST STRING | the tanpura's first string: Pa (default), Ma, Ni or any swara |
| EXTRA STRING | OFF by default; select a swara to add a string after the first string and before the two Sa strings and kharaj |
| 2ND: FIRST STRING, 2ND: EXTRA STRING | the second tanpura's first string and its independent optional string, OFF by default |
| FINE: 1ST STRING, EXTRA, SA (2ND), SA (3RD), KHARAJ | fine tuning of each string, ±50 cents; the second tanpura follows these |
| 2ND: PHASE | the phase of the second tanpura |
| TANPURA IRREG. | how irregular the plucking is, like a real hand |
| TABLA IRREG. | the same for the tabla (timing and strength of the strokes) |
| TONE SWARA | the swara of the tone switched on with ARP |
| A REFERENCE | the reference A, 415–466 Hz (default 440) |
| BLUETOOTH | ON: the radio is up and looks for speakers or headphones (turn right again: look again); OFF |
| BT SPEAKER | the speaker to connect: the remembered ones first (four are kept), then the ones just found |
| BT LINK | its state; turn right to connect the picked speaker (pairing if it is new), left to disconnect |
| BT OUTPUT | SPEAKER (the sound goes to Bluetooth) or FM-1 (its own output, the link kept) |

## The taals

| Taal | Matras | Vibhags | Theka |
|---|---|---|---|
| Teentaal | 16 | 4+4+4+4 | Dha Dhin Dhin Dha, Dha Dhin Dhin Dha, Dha Tin Tin Ta, Ta Dhin Dhin Dha |
| Ektaal | 12 | 2×6 | Dhin Dhin, DhaGe TiRaKiTa, Tu Na, Kat Ta, DhaGe TiRaKiTa, Dhin Na |
| Jhaptaal | 10 | 2+3+2+3 | Dhi Na, Dhi Dhi Na, Ti Na, Dhi Dhi Na |
| Rupak | 7 | 3+2+2 | Tin Tin Na, Dhi Na, Dhi Na (sam on khali) |
| Keherwa | 8 | 4+4 | Dha Ge Na Ti, Na Ka Dhi Na |
| Dadra | 6 | 3+3 | Dha Dhi Na, Dha Ti Na |
| Jhoomra | 14 | 3+4+3+4 | Dhin –Dha TiRaKiTa, Dhin Dhin DhaGe TiRaKiTa, Tin –Ta TiRaKiTa, Dhin Dhin DhaGe TiRaKiTa |
| Deepchandi | 14 | 3+4+3+4 | Dha Dhin –, Dha Dha Tin –, Ta Tin –, Dha Dha Dhin – |
| Dhamar | 14 | 5+2+3+4 | Ka Dhi Te Dhi Te, Dha –, Ga Ti Te, Ti Te Ta – |
| Chautaal | 12 | 2×6 | Dha Dha, Din Ta, KiTa Dha, Din Ta, TiTa KaTa, GaDi GaNa |
| Addha | 16 | 4+4+4+4 | Dha TiRaKiTa Dhin Na, Ta TiRaKiTa Dhin Na, Dha TiRaKiTa Dhin Na, Dha-Dha TiRaKiTa Dhin Na (Punjabi theka; variants exist) |
Tempo: 20 to 600 matras a minute. At slow (vilambit) tempos the theka is played as it is, without the fillers a tabla
player would add. Thekas vary between schools; the sources are in each file of `data/taal/`. To add a taal, write a
new file there (format: [data/taal/FORMAT.md](../data/taal/FORMAT.md)) and rebuild the firmware.

## Saving

**SAVE** keeps: Sa, the reference A, the taal and tempo, the three volumes, jawari, first string, the strings' fine
tuning, the tanpura's register and cycle, both irregularities, the tone's swara and octave. Which sounds are on is not
saved: the FM-1 always starts silent. While it saves, the sound dips for a moment (under a tenth of a second): writing
the flash stops the audio for about 12 ms, and the firmware fades out and in around it to avoid a click.

On the HOME page, hold **SEL + SAVE** for 2 seconds to restore TeenTaal's default settings. This stops the tanpuras,
tabla, and reference tone, then saves the defaults with every source off. Releasing before the 2-second hold completes
cancels the reset. FM-1 calibration and Bluetooth pairings are not changed.

## Credits

The tabla strokes are recordings from the Freesound pack "tabla bols" by **mmiron** (CC0). The tanpura follows the
model by van Walstijn, Bridges and Mehes (DAFx-16). Platform: **fm1-base** by Charles Vestal and **felucca-bsp**
by Leo Kuroshita (MPL-2.0). TeenTaal-FM1 is by **ElectroNick** (<https://github.com/ELENICK97>). See
[CREDITS.md](../CREDITS.md).
