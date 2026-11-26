# TeenTaal — manuale d'uso

> Versione italiana. English: [MANUAL.md](MANUAL.md). Firmware generato con l'IA (Claude) e provato solo negli emulatori: si usa a proprio rischio (vedi il Disclaimer in [README.md](../README.md#disclaimer)).

TeenTaal trasforma l'FM-1 in uno strumento per esercitarsi nella musica classica indiana: un **tanpura**, i
**tabla** che suonano undici taal, e un **tono di riferimento** per l'intonazione. Tutto si suona da una sola
schermata. All'accensione è tutto spento; le impostazioni salvate tornano come le avevi lasciate.

## La schermata

```
 TEENTAAL                     UNSAVED      ← nome; UNSAVED se ci sono modifiche non salvate
 SA A3          TANPURA A2                 ← il tuo Sa; il Sa del tanpura
 Teentaal > Ektaal            120 /MIN     ← il taal (e il prossimo, se l'hai cambiato); il tempo
 9 /16          KHALI                      ← la matra che suona ora: rossa sul sam, blu nel khali
 X       2       0       3                 ← il ciclo: una casella per matra, divise in vibhag
 ■ □ □ □ □ □ □ □ ■ □ □ □ □ □ □ □
 TANPURA ON     PA J75 V100                ← prima corda, jawari, volume
 TONE    OFF    SA V70                     ← swara del tono, ottava, volume
 CPU …  TABLA V100
```

## I comandi

| Comando | Cosa fa |
|---|---|
| **SEQ** | tanpura: entrambi accesi / 2° in MUTE / 1° in MUTE / entrambi spenti |
| **PLAY** | tabla: partono dal sam / si fermano (luce verde mentre suonano) |
| **ARP** | tono di riferimento acceso / spento |
| **un tasto** | il tono su quello swara **finché lo tieni premuto** (come un harmonium) |
| **SEL + un tasto** | imposta Sa alla nota (i cents restano) |
| **FX + un tasto da C4 a G4** | con entrambi i tanpura spenti, pizzica una corda un'ottava sotto la nota premuta, senza cambiare Sa |
| **SEL + OCT− / OCT+** | il Sa un'ottava sotto / sopra |
| **PRESETS** | il Sa di un cent per scatto |
| **SELECT** | il taal; se i tabla suonano, il nuovo taal parte **alla fine del ciclo** |
| **KNOB1** | il tempo, 1 matra al minuto per scatto (**SEL + KNOB1**: 10) |
| **KNOB2 / KNOB3 / KNOB4** | volume di tanpura / tabla / tono |
| **ALGORITHM** | jawari: la posizione del filo di cotone, 75 o 100. In senso orario 100, in antiorario 75 |
| **OCT− / OCT+** | l'ottava del tono (mandra, madhya, tar) |
| **MASTER** | volume generale |
| **EDIT** | le impostazioni (sotto); **HOME** torna indietro |
| **SAVE** | salva le impostazioni (scompare UNSAVED) |

I tasti si illuminano dal Sa in su: **Sa e Pa accesi**, gli altri swara shuddha (Re, Ga, Ma, Dha, Ni) tenui. Così
la tastiera mostra dove sono gli swara rispetto al Sa scelto.

## Il Sa

Il Sa di default è **La3 (220 Hz)**, adatto a un bansuri in La (il Sa del flauto è un'ottava sopra). Per un bansuri
in Sol: **SEL + il tasto del Sol3** (196 Hz). I 27 tasti danno da F2 a G4, una nota per tasto; SEL + OCT− / OCT+ sposta il Sa di un'ottava. Il tanpura suona sulla nota del Sa che vedi (di default La3). Il dayan dei tabla si accorda da solo sul Sa.

Con entrambi i tanpura spenti, tieni premuto FX e premi un tasto da C4 a G4: parte una sola corda del primo tanpura
un'ottava sotto la nota premuta, senza cambiare Sa. Se uno dei tanpura è acceso, la combinazione FX non avvia la corda;
SEL + un tasto imposta sempre Sa.

Il ciclo predefinito del tanpura (5 secondi) pizzica la prima corda, Sa, Sa e la kharaj grave a 0, 1,0, 1,75 e 2,5 secondi,
poi fa una pausa. Con EXTRA STRING attiva, la corda aggiuntiva viene pizzicata dopo la prima; le due corde Sa e la kharaj
seguono a 1,75, 2,5 e 3,25 secondi. Entrambi i tanpura sono spenti all'avvio. Ogni pressione di SEQ passa allo stato
successivo: entrambi accesi; il secondo in MUTE (suona il primo); il primo in MUTE (suona il secondo); entrambi spenti.
Un tanpura in MUTE continua il suo ciclo, quindi riparte in tempo; nella schermata principale compare MUTE al posto di ON.
Il secondo tanpura ha la propria prima corda (NI di default), fase (20%) e corda aggiuntiva opzionale; il ciclo di
pizzico e l'accordatura fine di ogni corda sono condivisi con il primo tanpura.

## Le impostazioni (EDIT)

SELECT sceglie la riga, PRESETS cambia il valore, HOME torna alla schermata principale. I puntini in basso mostrano
dove sei nell'elenco.

| Riga | Cosa regola |
|---|---|
| TEMPERAMENT | l'intonazione degli swara: EQUAL (default, temperamento equabile), JUST 5 (intonazione giusta a 5 limiti) o JUST PYTH (giusta pitagorica). Il Sa non cambia; la prima e la corda aggiuntiva di entrambi i tanpura e il tono di riferimento seguono la scelta; il pizzico FX dalla tastiera resta equabile |
| PLUCK CYCLE | la durata del ciclo di pizzico, condivisa dai due tanpura: 2,5–60 s (passi di 0,1 s fino a 10 s, poi 1 s) |
| FIRST STRING | la prima corda del tanpura: Pa (default), Ma, Ni o qualsiasi swara |
| EXTRA STRING | OFF di default; scegli uno swara per aggiungere una corda dopo la prima e prima delle due corde Sa e del kharaj |
| 2ND: FIRST STRING, 2ND: EXTRA STRING | la prima corda del secondo tanpura e la sua corda aggiuntiva indipendente, OFF di default |
| FINE: 1ST STRING, EXTRA, SA (2ND), SA (3RD), KHARAJ | accordatura fine delle corde, ±50 cents; il secondo tanpura segue questi valori |
| 2ND: PHASE | la fase del secondo tanpura |
| TANPURA IRREG. | quanto il pizzico è irregolare, come una mano vera |
| TABLA IRREG. | lo stesso per i tabla (tempi e forza dei colpi) |
| TONE SWARA | lo swara del tono acceso con ARP |
| A REFERENCE | il La di riferimento, 415–466 Hz (default 440) |

## I taal

Teentaal (16), Ektaal (12), Jhaptaal (10), Rupak (7), Keherwa (8), Dadra (6), Jhoomra (14), Deepchandi (14),
Dhamar (14), Chautaal (12), Addha (16). Il tempo va da 20 a 600 matra al minuto. Ogni taal suona la sua theka; le fonti sono
nei file `data/taal/`. Per aggiungere un taal basta scrivere un file nuovo (formato in `data/taal/FORMAT.md`) e
ricompilare.

## Salvare

**SAVE** salva: Sa, La di riferimento, il taal e il tempo, i tre volumi, jawari, prima corda, accordature fini,
registro, ciclo del tanpura, le irregolarità, swara e ottava del tono. Non salva se le sorgenti sono accese:
all'accensione l'FM-1 parte in silenzio. Durante il salvataggio il suono si abbassa per un istante (meno di un decimo di secondo): la scrittura nella
memoria flash ferma l'audio per circa 12 ms, e il firmware sfuma prima e dopo per non fare un click.

Dalla schermata HOME, tieni premuti **SEL + SAVE** per 2 secondi per ripristinare le impostazioni predefinite di
TeenTaal. Il reset interrompe i tanpura, i tabla e il tono di riferimento, poi salva i default lasciando tutte le sorgenti
spente. Rilasciando i tasti prima dei 2 secondi, il reset viene annullato. La calibrazione dell'FM-1 e gli abbinamenti
Bluetooth non vengono modificati.

## Crediti

I colpi dei tabla sono registrazioni del pacchetto Freesound "tabla bols" di **mmiron** (CC0). Il tanpura segue il
modello di van Walstijn, Bridges e Mehes (DAFx-16). Piattaforma: **fm1-base** di Charles Vestal e felucca-bsp
(MPL-2.0).
