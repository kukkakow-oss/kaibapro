#ifndef BOT_THEATER_H
#define BOT_THEATER_H

// Bot Theater: automatic bot-vs-bot rooms.
// When enabled, the game hosts a LAN room on startup using saved settings,
// moves itself to spectator, runs the random-deck bot script to fill the
// room, and starts the duel once every seat is ready.
// Settings live in bot_theater.conf, separate from system.conf, because the
// original game rewrites system.conf and would drop settings it doesn't know.

#include <string>

namespace ygo {

class BotTheater {
public:
	// Settings from bot_theater.conf
	bool enabled = true;
	bool skip_host_window = false;
	std::wstring python = L"python";
	std::wstring script = L"random_duel.py";
	std::wstring script_args;
	std::wstring lflist_name = L"No Banlist";
	int allowed_cards = 5;
	int mode = 0;
	int time_limit = 0;
	int duel_rule = 5;
	bool no_check_deck = true;
	bool no_shuffle_deck = false;
	int start_lp = 8000;
	int start_hand = 5;
	int draw_count = 1;
	std::wstring password;

	// State for the current room
	bool active = false;

	void Load();
	void Save() const;
	void ApplyToHostWindow() const;
	void ReadFromHostWindow();

	// Hooks called from the menu and network code
	void OnHostConfirm();
	void OnTypeChange(unsigned char selftype);
	void OnReadyChanged(bool all_ready, bool is_host);
	bool SkipPhaseBanner() const;
	void ShowWindowAfterRoom() const;

private:
	bool observer_requested = false;
	bool bots_launched = false;
	bool start_sent = false;
	unsigned short room_port = 0;
	bool room_is_tag = false;
	std::wstring room_password;

	bool LaunchBots(std::wstring& error);
	void SystemMessage(const wchar_t* msg) const;
};

extern BotTheater botTheater;

}

#endif // BOT_THEATER_H
