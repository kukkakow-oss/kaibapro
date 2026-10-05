#include "sound_manager.h"
#include "myfilesystem.h"
#ifdef IRRKLANG_STATIC
#include "../ikpmp3/ikpMP3.h"
#endif
#ifdef YGOPRO_USE_MINIAUDIO
#include <miniaudio.h>
#endif

namespace ygo {

SoundManager soundManager;

#ifdef YGOPRO_USE_MINIAUDIO
// miniaudio replaces irrKlang, which is no longer publicly available.
struct MiniaudioState {
	ma_engine engine;
	ma_sound_group sfx;
	ma_sound_group music;
	ma_sound* bgm{ nullptr };
	std::wstring song;
};
#endif

static const char* SoundFile(int sound) {
	switch(sound) {
	case SOUND_SUMMON: return "./sound/summon.wav";
	case SOUND_SPECIAL_SUMMON: return "./sound/specialsummon.wav";
	case SOUND_ACTIVATE: return "./sound/activate.wav";
	case SOUND_SET: return "./sound/set.wav";
	case SOUND_FILP: return "./sound/flip.wav";
	case SOUND_REVEAL: return "./sound/reveal.wav";
	case SOUND_EQUIP: return "./sound/equip.wav";
	case SOUND_DESTROYED: return "./sound/destroyed.wav";
	case SOUND_BANISHED: return "./sound/banished.wav";
	case SOUND_TOKEN: return "./sound/token.wav";
	case SOUND_ATTACK: return "./sound/attack.wav";
	case SOUND_DIRECT_ATTACK: return "./sound/directattack.wav";
	case SOUND_DRAW: return "./sound/draw.wav";
	case SOUND_SHUFFLE: return "./sound/shuffle.wav";
	case SOUND_DAMAGE: return "./sound/damage.wav";
	case SOUND_RECOVER: return "./sound/gainlp.wav";
	case SOUND_COUNTER_ADD: return "./sound/addcounter.wav";
	case SOUND_COUNTER_REMOVE: return "./sound/removecounter.wav";
	case SOUND_COIN: return "./sound/coinflip.wav";
	case SOUND_DICE: return "./sound/diceroll.wav";
	case SOUND_NEXT_TURN: return "./sound/nextturn.wav";
	case SOUND_PHASE: return "./sound/phase.wav";
	case SOUND_MENU: return "./sound/menu.wav";
	case SOUND_BUTTON: return "./sound/button.wav";
	case SOUND_INFO: return "./sound/info.wav";
	case SOUND_QUESTION: return "./sound/question.wav";
	case SOUND_CARD_PICK: return "./sound/cardpick.wav";
	case SOUND_CARD_DROP: return "./sound/carddrop.wav";
	case SOUND_PLAYER_ENTER: return "./sound/playerenter.wav";
	case SOUND_CHAT: return "./sound/chatmessage.wav";
	default: return nullptr;
	}
}

bool SoundManager::Init() {
#ifdef YGOPRO_USE_IRRKLANG
	bgm_scene = -1;
	RefreshBGMList();
	rnd.reset((unsigned int)std::time(nullptr));
	engineSound = irrklang::createIrrKlangDevice();
	engineMusic = irrklang::createIrrKlangDevice();
	if(!engineSound || !engineMusic) {
		return false;
	} else {
#ifdef IRRKLANG_STATIC
		irrklang::ikpMP3Init(engineMusic);
#endif
		return true;
	}
#elif defined(YGOPRO_USE_MINIAUDIO)
	bgm_scene = -1;
	RefreshBGMList();
	rnd.reset((unsigned int)std::time(nullptr));
	ma = new MiniaudioState();
	if(ma_engine_init(nullptr, &ma->engine) != MA_SUCCESS) {
		delete ma;
		ma = nullptr;
		return false;
	}
	ma_sound_group_init(&ma->engine, 0, nullptr, &ma->sfx);
	ma_sound_group_init(&ma->engine, 0, nullptr, &ma->music);
	return true;
#else
	return false;
#endif
}
void SoundManager::RefreshBGMList() {
	RefershBGMDir(L"", BGM_DUEL);
	RefershBGMDir(L"duel", BGM_DUEL);
	RefershBGMDir(L"menu", BGM_MENU);
	RefershBGMDir(L"deck", BGM_DECK);
	RefershBGMDir(L"advantage", BGM_ADVANTAGE);
	RefershBGMDir(L"disadvantage", BGM_DISADVANTAGE);
	RefershBGMDir(L"win", BGM_WIN);
	RefershBGMDir(L"lose", BGM_LOSE);
}
void SoundManager::RefershBGMDir(std::wstring path, int scene) {
	std::wstring search = L"./sound/BGM/" + path;
	FileSystem::TraversalDir(search.c_str(), [this, &path, scene](const wchar_t* name, bool isdir) {
		if(!isdir && (IsExtension(name, L".mp3") || IsExtension(name, L".ogg"))) {
			std::wstring filename = path + L"/" + name;
			BGMList[BGM_ALL].push_back(filename);
			BGMList[scene].push_back(filename);
		}
	});
}
void SoundManager::PlaySoundEffect(int sound) {
	const char* file = SoundFile(sound);
	if(!file)
		return;
#ifdef YGOPRO_USE_IRRKLANG
	if(!mainGame->chkEnableSound->isChecked())
		return;
	engineSound->setSoundVolume(mainGame->gameConf.sound_volume);
	engineSound->play2D(file);
#elif defined(YGOPRO_USE_MINIAUDIO)
	if(!ma || !mainGame->chkEnableSound->isChecked())
		return;
	ma_sound_group_set_volume(&ma->sfx, (float)mainGame->gameConf.sound_volume);
	ma_engine_play_sound(&ma->engine, file, &ma->sfx);
#endif
}
void SoundManager::PlayDialogSound(irr::gui::IGUIElement * element) {
	if(element == mainGame->wMessage) {
		PlaySoundEffect(SOUND_INFO);
	} else if(element == mainGame->wQuery) {
		PlaySoundEffect(SOUND_QUESTION);
	} else if(element == mainGame->wSurrender) {
		PlaySoundEffect(SOUND_QUESTION);
	} else if(element == mainGame->wOptions) {
		PlaySoundEffect(SOUND_QUESTION);
	} else if(element == mainGame->wANAttribute) {
		PlaySoundEffect(SOUND_QUESTION);
	} else if(element == mainGame->wANCard) {
		PlaySoundEffect(SOUND_QUESTION);
	} else if(element == mainGame->wANNumber) {
		PlaySoundEffect(SOUND_QUESTION);
	} else if(element == mainGame->wANRace) {
		PlaySoundEffect(SOUND_QUESTION);
	} else if(element == mainGame->wReplaySave) {
		PlaySoundEffect(SOUND_QUESTION);
	} else if(element == mainGame->wFTSelect) {
		PlaySoundEffect(SOUND_QUESTION);
	}
}
#ifdef YGOPRO_USE_MINIAUDIO
void SoundManager::StopMusicSound() {
	if(ma && ma->bgm) {
		ma_sound_uninit(ma->bgm);
		delete ma->bgm;
		ma->bgm = nullptr;
		ma->song.clear();
	}
}
#endif
void SoundManager::PlayMusic(char* song, bool loop) {
#ifdef YGOPRO_USE_IRRKLANG
	if(!mainGame->chkEnableMusic->isChecked())
		return;
	if(!engineMusic->isCurrentlyPlaying(song)) {
		engineMusic->stopAllSounds();
		engineMusic->setSoundVolume(mainGame->gameConf.music_volume);
		soundBGM = engineMusic->play2D(song, loop, false, true);
	}
#elif defined(YGOPRO_USE_MINIAUDIO)
	if(!ma || !mainGame->chkEnableMusic->isChecked())
		return;
	wchar_t wsong[1024];
	BufferIO::DecodeUTF8(song, wsong);
	if(ma->bgm && ma->song == wsong && !ma_sound_at_end(ma->bgm))
		return;	// already playing this song
	StopMusicSound();
	ma->bgm = new ma_sound;
	// Wide-character path so music files with non-English names still load.
	if(ma_sound_init_from_file_w(&ma->engine, wsong, MA_SOUND_FLAG_STREAM, &ma->music, nullptr, ma->bgm) != MA_SUCCESS) {
		delete ma->bgm;
		ma->bgm = nullptr;
		return;
	}
	ma->song = wsong;
	ma_sound_set_looping(ma->bgm, loop ? MA_TRUE : MA_FALSE);
	ma_sound_group_set_volume(&ma->music, (float)mainGame->gameConf.music_volume);
	ma_sound_start(ma->bgm);
#endif
}
void SoundManager::PlayBGM(int scene) {
#if defined(YGOPRO_USE_IRRKLANG) || defined(YGOPRO_USE_MINIAUDIO)
	if(!mainGame->chkEnableMusic->isChecked())
		return;
	if(!mainGame->chkMusicMode->isChecked())
		scene = BGM_ALL;
#ifdef YGOPRO_USE_IRRKLANG
	bool finished = soundBGM && soundBGM->isFinished();
#else
	if(!ma)
		return;
	bool finished = ma->bgm && ma_sound_at_end(ma->bgm);
#endif
	char BGMName[1024];
	if(scene != bgm_scene || finished) {
		int count = BGMList[scene].size();
		if(count <= 0)
			return;
		bgm_scene = scene;
		int bgm = rnd.get_random_integer(0, count -1);
		auto name = BGMList[scene][bgm].c_str();
		wchar_t fname[1024];
		myswprintf(fname, L"./sound/BGM/%ls", name);
		BufferIO::EncodeUTF8(fname, BGMName);
		PlayMusic(BGMName, false);
	}
#endif
}
void SoundManager::StopBGM() {
#ifdef YGOPRO_USE_IRRKLANG
	engineMusic->stopAllSounds();
#elif defined(YGOPRO_USE_MINIAUDIO)
	StopMusicSound();
	bgm_scene = -1;	// so music starts again if it's switched back on
#endif
}
void SoundManager::SetSoundVolume(double volume) {
#ifdef YGOPRO_USE_IRRKLANG
	engineSound->setSoundVolume(volume);
#elif defined(YGOPRO_USE_MINIAUDIO)
	if(ma)
		ma_sound_group_set_volume(&ma->sfx, (float)volume);
#endif
}
void SoundManager::SetMusicVolume(double volume) {
#ifdef YGOPRO_USE_IRRKLANG
	engineMusic->setSoundVolume(volume);
#elif defined(YGOPRO_USE_MINIAUDIO)
	if(ma)
		ma_sound_group_set_volume(&ma->music, (float)volume);
#endif
}
}
