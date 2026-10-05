/*
 * Compiles the miniaudio sound library into the game, together with the
 * Ogg Vorbis decoder it uses for .ogg music (MP3 and WAV are built in).
 * miniaudio replaces irrKlang, which is no longer publicly available.
 * The build adds this file and the YGOPRO_USE_MINIAUDIO setting.
 */
#ifdef YGOPRO_USE_MINIAUDIO
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
#endif
