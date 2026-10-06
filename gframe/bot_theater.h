#ifndef BOT_THEATER_H
#define BOT_THEATER_H

// Bot Theater: automatic bot-vs-bot rooms.
// When enabled, the game hosts a LAN room on startup using saved settings,
// moves itself to spectator, runs the random-deck bot script to fill the
// room, and starts the duel once every seat is ready.
// Settings live in bot_theater.conf, separate from system.conf, because the
// original game rewrites system.conf and would drop settings it doesn't know.

#include <string>
#include <vector>

namespace irr { namespace video { class ITexture; } namespace gui { class IGUICheckBox; } }

namespace ygo {

class ClientCard;

class BotTheater {
public:
	// Settings from bot_theater.conf
	bool enabled = true;
	bool skip_host_window = false;
	bool reveal_hands = true;
	bool info_follows_actions = true;
	bool save_replays = false;
	bool turn_highlight = true;
	bool holograms_on = true;
	int hologram_size = 100;		// percent
	double hologram_seconds = 1.5;
	int hologram_opacity = 90;		// percent
	bool hologram_stay = true;		// stay (smaller and dimmer) until the card leaves its zone
	int hologram_rest_size = 80;	// percent of full size while resting
	int hologram_rest_opacity = 50;	// percent of full opacity while resting
	bool hologram_backrow = true;		// Spell/Trap and Pendulum zones too
	int hologram_backrow_size = 65;	// percent of the monster hologram size
	int hologram_backrow_lift_bottom = 50;	// bottom player's back row: float height, percent of the monster row's
	int hologram_backrow_lift_top = 125;	// top player's back row (sits behind its monster row, so it floats higher)
	double top_hand_raise = 1.0;		// card heights to raise the top player's hand by
	bool split_zones = false;			// tag duels: each teammate has their own section of the field
	bool random_backgrounds = true;		// pick each duel's background from textures/backgrounds
	bool custom_background_field = false;	// still draw the field overlay (field2/field3.png) over custom backgrounds
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
	void ApplyToHostWindow();
	void ReadFromHostWindow();

	// Hooks called from the menu and network code
	void OnHostConfirm();
	void OnTypeChange(unsigned char selftype);
	void OnReadyChanged(bool all_ready, bool is_host);
	bool SkipPhaseBanner() const;
	bool RevealHands() const { return active && reveal_hands; }
	bool InfoFollowsActions() const;
	bool SkipEndPrompts() const;
	bool SplitZones() const { return active && split_zones; }
	void OnCardAction(unsigned int code, int local_player);
	void OnDuelStart();
	void SetTurnPlayer(int local_player);
	const wchar_t* TurnText() const;
	void DrawInfoBorder() const;
	int TurnSide() const;	// side whose turn it is: 0 = bottom, 1 = top, -1 = none
	void DrawHolograms();
	irr::video::ITexture* DuelBackground();	// background for the current duel (drawing code only)
	bool HideFieldOverlay() const;			// a custom background is showing, so skip field2/field3.png
	float TopHandShift() const;		// how far back the top hand moves on the board
	void MapTopHandPoint(int& x, int& y) const;	// mouse position -> where the hand would normally be
	void DrawTurnHighlight(int left, int top, int right, int bottom) const;
	void ShowWindowAfterRoom() const;

private:
	bool observer_requested = false;
	bool bots_launched = false;
	bool start_sent = false;
	unsigned short room_port = 0;
	bool room_is_tag = false;
	std::wstring room_password;
	int info_owner = -1;	// whose action the info window shows: 0 = bottom player, 1 = top
	wchar_t turn_text[64] = {};
	int turn_player = -1;	// turn player as the duel numbers them (not affected by swapping sides)

	struct Hologram {
		unsigned int code;
		int side;		// 0 = bottom player, 1 = top
		int sequence;	// zone number
		bool backrow;	// Spell/Trap or Pendulum zone (otherwise a Monster zone)
		int frame;
		ClientCard* card;	// the card the hologram belongs to
		int leave_frame;	// -1 while the card is still there, then counts the fade-out
	};
	std::vector<Hologram> holograms;
	irr::gui::IGUICheckBox* chkSplitZones = nullptr;	// added to the host window
	std::wstring pending_background;	// chosen at duel start, loaded by the drawing code
	std::wstring last_background;
	bool background_pending = false;
	irr::video::ITexture* tDuelBackground = nullptr;
	void PickBackground();
	irr::video::ITexture* tWhite = nullptr;
	irr::video::ITexture* tGlow = nullptr;
	bool SpectatingBotRoom() const;
	void MakeHologramTextures();
	void ScanForNewHolograms();

	bool LaunchBots(std::wstring& error);
	void SystemMessage(const wchar_t* msg) const;
};

extern BotTheater botTheater;

}

#endif // BOT_THEATER_H
