# GRITWAVE — Sintetizador VST3 de bass music moderna

Sintetizador wavetable (JUCE 8 / C++17) pensado para el sonido de screeches, growls vocales,
reeses y stabs del dubstep/bass house moderno (referencias de estilo: Skrillex y Virtual Riot).

## Motor de sonido

| Sección | Qué hace |
|---|---|
| **OSC A / OSC B** | Wavetable de 8 frames (Sine, Triangle, Saw, Square, Formant Saw, Vowel, Pulse, Digital Buzz) con morph continuo, hasta 7 voces de unison con detune/spread, oct/semi/fine. |
| **Warp** | Off, **Bend** (distorsión de fase), **Sync** (hard-sync), **FM** (auto-FM). Es la base del sonido "screech". |
| **Sub / Noise** | Seno una octava abajo de la nota + ruido blanco. |
| **Filter** | LP24, LP12, BP, HP, Notch y **Vowel** (3 formantes A-E-I-O-U; el Cutoff recorre las vocales, ideal para growls "yoy"). Drive, Reso, KeyTrack. |
| **Amp Env / Mod Env** | ADSR. La Mod Env tiene 2 destinos (Cutoff, WT A/B, Warp, Pitch, Drive, Volume, Reso). Pitch con env = caídas tipo screech/808. |
| **LFO 1 / LFO 2** | 6 formas, sincronizados al tempo del host (1/1 … 1/16D, tresillos y con puntillo) o libres, con Key Sync. Un destino cada uno. |
| **Mono / Glide** | Modo monofónico legato con portamento. |
| **Distortion** | Soft, Hard, Fold (wavefolder), Crush, Tube. Con 2x oversampling. |
| **OTT** | Compresor multibanda de 3 bandas (140 Hz / 2.4 kHz) que comprime hacia abajo y expande hacia arriba. |
| **Chorus, Delay, Reverb** | Delay ping-pong sincronizado al tempo. |
| **Limiter** | Siempre activo al final de la cadena. |

MIDI: notas, velocity, pitch bend (±2 semitonos), all-notes-off. 8 voces.

## Compilar

Necesitas: CMake ≥ 3.22, compilador C++17 y conexión a internet (descarga JUCE 8.0.4 automáticamente).

```bash
cd Gritwave
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

El VST3 se copia solo a la carpeta estándar de plugins
(Windows: `C:\Program Files\Common Files\VST3`, macOS: `~/Library/Audio/Plug-Ins/VST3`,
Linux: `~/.vst3`). También se genera una versión Standalone para probar sin DAW.

Requisitos por sistema: Windows → Visual Studio 2022 (Desktop C++); macOS → Xcode;
Linux → `libasound2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev`.

> JUCE tiene licencia dual (GPLv3 / comercial). Si vas a distribuir o vender el plugin, revisa https://juce.com/legal/juce-8-licence/

## Los 10 presets de fábrica

Aparecen en el selector superior de la interfaz y en la lista de programas del DAW.
Están **inspirados en el carácter** de lanzamientos recientes de cada artista
(no son réplicas de los patches originales).

| # | Preset | Inspiración | Carácter |
|---|---|---|---|
| 01 | Fuze – Hyper Screech Lead | Skrillex | Lead mono con FM + sync, caída de pitch y fold. |
| 02 | Duro – Dembow Sub Stab | Skrillex | Stab corto con sub, filtro que "ladra" al inicio. |
| 03 | Thistle – Warehouse Wobble | Skrillex | Bajo de pulso con wobble 1/16 y S&H, estilo rave/warehouse. |
| 04 | Essa Ta Quente – Baile Sub Slide | Skrillex | 808 largo con slide y saturación tube. |
| 05 | Mirage – UKG Reese | Skrillex | Reese desafinado con wobble suave y chorus. |
| 06 | hit me where it hurts – Glass Pad | Skrillex | Pad ancho de 7 voces con delay y reverb. |
| 07 | Yo Yan – Ambient Drone | Skrillex | Drone vocal con filtro Vowel lento y cola larga. |
| 08 | Burning Out – Vowel Growl | Virtual Riot | Growl vocal: filtro Vowel + LFO 1/4T + OTT. |
| 09 | Yellow Lights – Neon Pluck | Virtual Riot | Pluck brillante melódico con delay con puntillo. |
| 10 | Stealing Fire – Neuro Reese | Virtual Riot | Neuro bass: FM + sync, filtro resonante y warp modulado. |

### Cómo crear tus propios presets
En `Source/FactoryPresets.h` cada preset es una lista `{ "id_parametro", valor }`.
Los parámetros omitidos vuelven a su valor por defecto. Las tablas de índices
(destinos de modulación, divisiones, formas de onda) están en el comentario del archivo.

## Ideas para ampliar
Matriz de modulación de más slots, mod wheel → profundidad del LFO, más wavetables
(importar WAV), filtro comb, soporte de presets de usuario en disco, sidechain interno.

## Estructura
```
Source/
  Parameters.h      parámetros (APVTS) y structs de datos
  WavetableBank.h   wavetables aditivas con mipmaps (anti-aliasing)
  Voice.h           oscilador, unison, filtros, LFOs, envolventes
  Effects.h         distorsión, OTT, delay, chorus, reverb, limiter
  FactoryPresets.h  los 10 presets
  PluginProcessor.* MIDI, gestión de voces, estado
  PluginEditor.*    interfaz
```
