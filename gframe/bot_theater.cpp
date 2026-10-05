#include "config.h"
#include "bot_theater.h"
#include "game.h"
#include "duelclient.h"
#include "network.h"
#include "image_manager.h"
#include "materials.h"
#include "client_card.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <vector>

namespace ygo {

BotTheater botTheater;

static const char* CONFIG_FILE = "bot_theater.conf";
static const wchar_t* LOG_FILE = L"bot_theater.log";

static std::wstring FromUTF8(const char* s) {
	std::vector<wchar_t> buf(std::strlen(s) + 1);
	BufferIO::DecodeUTF8String(s, buf.data(), (int)buf.size());
	return std::wstring(buf.data());
}

static std::string ToUTF8(const std::wstring& s) {
	std::vector<char> buf(s.size() * 4 + 1);
	BufferIO::EncodeUTF8String(s.c_str(), buf.data(), (int)buf.size());
	return std::string(buf.data());
}

static std::string Trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	if(start == std::string::npos)
		return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

static int ToInt(const std::string& s, int fallback) {
	char* end = nullptr;
	long v = std::strtol(s.c_str(), &end, 10);
	return (end && end != s.c_str()) ? (int)v : fallback;
}

void BotTheater::Load() {
	FILE* fp = std::fopen(CONFIG_FILE, "r");
	if(!fp) {
		Save();	// first run: write a file with the defaults so it can be edited
		return;
	}
	char line[1024];
	bool first = true;
	while(std::fgets(line, sizeof line, fp)) {
		char* p = line;
		if(first && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
			p += 3;	// skip a UTF-8 marker added by Notepad
		first = false;
		std::string text = Trim(p);
		if(text.empty() || text[0] == '#')
			continue;
		size_t eq = text.find('=');
		if(eq == std::string::npos)
			continue;
		std::string key = Trim(text.substr(0, eq));
		std::string value = Trim(text.substr(eq + 1));
		if(key == "enabled") enabled = ToInt(value, 1) != 0;
		else if(key == "skip_host_window") skip_host_window = ToInt(value, 0) != 0;
		else if(key == "reveal_hands") reveal_hands = ToInt(value, 1) != 0;
		else if(key == "info_follows_actions") info_follows_actions = ToInt(value, 1) != 0;
		else if(key == "save_replays") save_replays = ToInt(value, 0) != 0;
		else if(key == "turn_highlight") turn_highlight = ToInt(value, 1) != 0;
		else if(key == "holograms") holograms_on = ToInt(value, 1) != 0;
		else if(key == "hologram_size") hologram_size = ToInt(value, hologram_size);
		else if(key == "hologram_seconds") hologram_seconds = std::strtod(value.c_str(), nullptr);
		else if(key == "hologram_opacity") hologram_opacity = ToInt(value, hologram_opacity);
		else if(key == "hologram_stay") hologram_stay = ToInt(value, 1) != 0;
		else if(key == "hologram_rest_size") hologram_rest_size = ToInt(value, hologram_rest_size);
		else if(key == "hologram_rest_opacity") hologram_rest_opacity = ToInt(value, hologram_rest_opacity);
		else if(key == "hologram_backrow") hologram_backrow = ToInt(value, 1) != 0;
		else if(key == "hologram_backrow_size") hologram_backrow_size = ToInt(value, hologram_backrow_size);
		else if(key == "top_hand_raise") top_hand_raise = std::strtod(value.c_str(), nullptr);
		else if(key == "python") python = FromUTF8(value.c_str());
		else if(key == "script") script = FromUTF8(value.c_str());
		else if(key == "script_args") script_args = FromUTF8(value.c_str());
		else if(key == "lflist") lflist_name = FromUTF8(value.c_str());
		else if(key == "allowed_cards") allowed_cards = ToInt(value, allowed_cards);
		else if(key == "mode") mode = ToInt(value, mode);
		else if(key == "time_limit") time_limit = ToInt(value, time_limit);
		else if(key == "duel_rule") duel_rule = ToInt(value, duel_rule);
		else if(key == "no_check_deck") no_check_deck = ToInt(value, 1) != 0;
		else if(key == "no_shuffle_deck") no_shuffle_deck = ToInt(value, 0) != 0;
		else if(key == "start_lp") start_lp = ToInt(value, start_lp);
		else if(key == "start_hand") start_hand = ToInt(value, start_hand);
		else if(key == "draw_count") draw_count = ToInt(value, draw_count);
		else if(key == "password") password = FromUTF8(value.c_str());
	}
	std::fclose(fp);
}

void BotTheater::Save() const {
	FILE* fp = std::fopen(CONFIG_FILE, "w");
	if(!fp)
		return;
	std::fprintf(fp, "# Bot Theater settings (used only by the modified game).\n");
	std::fprintf(fp, "# Lines starting with # are notes and are ignored.\n\n");
	std::fprintf(fp, "# 1 = open to the bot room's host window when the game opens, 0 = normal game\n");
	std::fprintf(fp, "enabled = %d\n\n", enabled ? 1 : 0);
	std::fprintf(fp, "# 1 = skip the host window and host straight away with the saved settings\n");
	std::fprintf(fp, "skip_host_window = %d\n\n", skip_host_window ? 1 : 0);
	std::fprintf(fp, "# 1 = show both players' hands to you while spectating a bot room\n");
	std::fprintf(fp, "reveal_hands = %d\n\n", reveal_hands ? 1 : 0);
	std::fprintf(fp, "# 1 = the card window at the top left shows the last card summoned or\n");
	std::fprintf(fp, "# activated (blue frame = bottom player, red = top) instead of the hovered card\n");
	std::fprintf(fp, "info_follows_actions = %d\n\n", info_follows_actions ? 1 : 0);
	std::fprintf(fp, "# Replays of bot duels are saved without asking. 0 = keep only the latest\n");
	std::fprintf(fp, "# (as _LastReplay), 1 = keep every one, named by date and time\n");
	std::fprintf(fp, "save_replays = %d\n\n", save_replays ? 1 : 0);
	std::fprintf(fp, "# 1 = gold frame around the avatar of the player whose turn it is\n");
	std::fprintf(fp, "turn_highlight = %d\n\n", turn_highlight ? 1 : 0);
	std::fprintf(fp, "# Holograms of face-up cards' art. 1 = on, 0 = off\n");
	std::fprintf(fp, "# hologram_size and hologram_opacity are percentages (100 = normal).\n");
	std::fprintf(fp, "# hologram_seconds is how long it shows at full size after the summon.\n");
	std::fprintf(fp, "# hologram_stay: 1 = afterwards it stays, at the rest size/opacity (percent\n");
	std::fprintf(fp, "# of full), until the card leaves its zone; 0 = it fades out instead.\n");
	std::fprintf(fp, "# Either way it fades out early if the card leaves the field.\n");
	std::fprintf(fp, "# hologram_backrow: 1 = Spell/Trap and Pendulum zones get holograms too,\n");
	std::fprintf(fp, "# floating lower, at hologram_backrow_size percent of the monster size.\n");
	std::fprintf(fp, "holograms = %d\n", holograms_on ? 1 : 0);
	std::fprintf(fp, "hologram_size = %d\n", hologram_size);
	std::fprintf(fp, "hologram_seconds = %.2f\n", hologram_seconds);
	std::fprintf(fp, "hologram_opacity = %d\n", hologram_opacity);
	std::fprintf(fp, "hologram_stay = %d\n", hologram_stay ? 1 : 0);
	std::fprintf(fp, "hologram_rest_size = %d\n", hologram_rest_size);
	std::fprintf(fp, "hologram_rest_opacity = %d\n", hologram_rest_opacity);
	std::fprintf(fp, "hologram_backrow = %d\n", hologram_backrow ? 1 : 0);
	std::fprintf(fp, "hologram_backrow_size = %d\n\n", hologram_backrow_size);
	std::fprintf(fp, "# How far to raise the top player's hand, in card heights, so holograms\n");
	std::fprintf(fp, "# don't cover it while spectating. 0 = normal position\n");
	std::fprintf(fp, "top_hand_raise = %.2f\n\n", top_hand_raise);
	std::fprintf(fp, "# Avatars: put images in textures/avatars named after the bots, e.g.\n");
	std::fprintf(fp, "# textures/avatars/Lady Luck.png (.png, .jpg or .jpeg). Characters that\n");
	std::fprintf(fp, "# Windows doesn't allow in file names (\\ / : * ? \" < > |) become _ instead.\n\n");
	std::fprintf(fp, "# How the bot script is started, and any extra options for it\n");
	std::fprintf(fp, "# (for example: script_args = --bot-deck Test)\n");
	std::fprintf(fp, "python = %s\n", ToUTF8(python).c_str());
	std::fprintf(fp, "script = %s\n", ToUTF8(script).c_str());
	std::fprintf(fp, "script_args = %s\n\n", ToUTF8(script_args).c_str());
	std::fprintf(fp, "# Room settings. These are saved automatically whenever you host,\n");
	std::fprintf(fp, "# so the easiest way to change them is in the game's host window.\n");
	std::fprintf(fp, "# mode: 0 = single duel, 1 = match, 2 = tag duel\n");
	std::fprintf(fp, "# allowed_cards: 0 = OCG, 1 = TCG, 2 = TCG/OCG, 3 = Custom, 4 = No Exclusive, 5 = All Cards\n");
	std::fprintf(fp, "# duel_rule: 1-3 = Master Rules 1-3, 4 = New Master Rules, 5 = Master Rules 2020\n");
	std::fprintf(fp, "# time_limit: seconds per player, 0 = no limit\n");
	std::fprintf(fp, "lflist = %s\n", ToUTF8(lflist_name).c_str());
	std::fprintf(fp, "allowed_cards = %d\n", allowed_cards);
	std::fprintf(fp, "mode = %d\n", mode);
	std::fprintf(fp, "time_limit = %d\n", time_limit);
	std::fprintf(fp, "duel_rule = %d\n", duel_rule);
	std::fprintf(fp, "no_check_deck = %d\n", no_check_deck ? 1 : 0);
	std::fprintf(fp, "no_shuffle_deck = %d\n", no_shuffle_deck ? 1 : 0);
	std::fprintf(fp, "start_lp = %d\n", start_lp);
	std::fprintf(fp, "start_hand = %d\n", start_hand);
	std::fprintf(fp, "draw_count = %d\n", draw_count);
	std::fprintf(fp, "password = %s\n", ToUTF8(password).c_str());
	std::fclose(fp);
}

void BotTheater::ApplyToHostWindow() const {
	wchar_t buf[32];
	auto* lf = mainGame->cbHostLFlist;
	for(irr::u32 i = 0; i < lf->getItemCount(); ++i) {
		if(lflist_name == lf->getItem(i)) {
			lf->setSelected(i);
			break;
		}
	}
	if(allowed_cards >= 0 && allowed_cards < (int)mainGame->cbRule->getItemCount())
		mainGame->cbRule->setSelected(allowed_cards);
	if(mode >= 0 && mode < (int)mainGame->cbMatchMode->getItemCount())
		mainGame->cbMatchMode->setSelected(mode);
	if(duel_rule >= 1 && duel_rule <= (int)mainGame->cbDuelRule->getItemCount())
		mainGame->cbDuelRule->setSelected(duel_rule - 1);
	myswprintf(buf, L"%d", time_limit);
	mainGame->ebTimeLimit->setText(buf);
	myswprintf(buf, L"%d", start_lp);
	mainGame->ebStartLP->setText(buf);
	myswprintf(buf, L"%d", start_hand);
	mainGame->ebStartHand->setText(buf);
	myswprintf(buf, L"%d", draw_count);
	mainGame->ebDrawCount->setText(buf);
	mainGame->chkNoCheckDeck->setChecked(no_check_deck);
	mainGame->chkNoShuffleDeck->setChecked(no_shuffle_deck);
	mainGame->ebServerPass->setText(password.c_str());
}

void BotTheater::ReadFromHostWindow() {
	auto* lf = mainGame->cbHostLFlist;
	if(lf->getSelected() >= 0)
		lflist_name = lf->getItem(lf->getSelected());
	allowed_cards = mainGame->cbRule->getSelected();
	mode = mainGame->cbMatchMode->getSelected();
	duel_rule = mainGame->cbDuelRule->getSelected() + 1;
	time_limit = std::wcstol(mainGame->ebTimeLimit->getText(), nullptr, 10);
	start_lp = std::wcstol(mainGame->ebStartLP->getText(), nullptr, 10);
	start_hand = std::wcstol(mainGame->ebStartHand->getText(), nullptr, 10);
	draw_count = std::wcstol(mainGame->ebDrawCount->getText(), nullptr, 10);
	no_check_deck = mainGame->chkNoCheckDeck->isChecked();
	no_shuffle_deck = mainGame->chkNoShuffleDeck->isChecked();
	password = mainGame->ebServerPass->getText();
}

void BotTheater::OnHostConfirm() {
	active = enabled;
	observer_requested = false;
	bots_launched = false;
	start_sent = false;
	if(!enabled)
		return;
	ReadFromHostWindow();
	Save();
	room_port = mainGame->gameConf.serverport;
	room_is_tag = (mode == 2);
	room_password = password;
}

void BotTheater::OnTypeChange(unsigned char selftype) {
	if(!active)
		return;
	if(selftype != NETPLAYER_TYPE_OBSERVER) {
		// We start in a duelist seat; move to spectator to leave the seats for the bots.
		if(!observer_requested) {
			observer_requested = true;
			DuelClient::SendPacketToServer(CTOS_HS_TOOBSERVER);
		}
		return;
	}
	if(bots_launched)
		return;
	bots_launched = true;
	std::wstring error;
	if(LaunchBots(error)) {
		SystemMessage(room_is_tag ? L"Bot Theater: sending in four bots..." : L"Bot Theater: sending in two bots...");
	} else {
		std::wstring msg = L"Bot Theater: couldn't start the bot script (" + error + L"). Check python and script in bot_theater.conf.";
		SystemMessage(msg.c_str());
	}
}

void BotTheater::OnReadyChanged(bool all_ready, bool is_host) {
	if(!active || !is_host)
		return;
	if(!all_ready) {
		start_sent = false;
		return;
	}
	if(start_sent)
		return;
	start_sent = true;
	DuelClient::SendPacketToServer(CTOS_HS_START);
}

bool BotTheater::SkipPhaseBanner() const {
	return enabled && mainGame->dInfo.player_type == NETPLAYER_TYPE_OBSERVER && !mainGame->dInfo.isReplay;
}

bool BotTheater::SkipEndPrompts() const {
	return active && mainGame->dInfo.player_type == NETPLAYER_TYPE_OBSERVER && !mainGame->dInfo.isReplay;
}

bool BotTheater::SpectatingBotRoom() const {
	return active && mainGame->dInfo.player_type == NETPLAYER_TYPE_OBSERVER && !mainGame->dInfo.isReplay;
}

bool BotTheater::InfoFollowsActions() const {
	return info_follows_actions && SpectatingBotRoom();
}

void BotTheater::OnCardAction(unsigned int code, int local_player) {
	if(!code || !InfoFollowsActions())
		return;
	mainGame->gMutex.lock();
	mainGame->ShowCardInfo(code);
	info_owner = local_player;
	mainGame->gMutex.unlock();
}

void BotTheater::SetTurnPlayer(int local_player) {
	turn_player = mainGame->LocalPlayer(local_player);	// back to the duel's own numbering
	const auto& d = mainGame->dInfo;
	const wchar_t* name;
	if(local_player == 0)
		name = (d.isTag && d.tag_player[0]) ? d.hostname_tag : d.hostname;
	else
		name = (d.isTag && d.tag_player[1]) ? d.clientname_tag : d.clientname;
	if(name && name[0])
		myswprintf(turn_text, L"%ls's Turn", name);
	else
		turn_text[0] = 0;
}

int BotTheater::TurnSide() const {
	if(!active || !turn_highlight || turn_player < 0 || !mainGame->dInfo.isStarted)
		return -1;
	return mainGame->LocalPlayer(turn_player);	// follows the spectator swap button
}

void BotTheater::DrawTurnHighlight(int left, int top, int right, int bottom) const {
	irr::video::SColor gold(255, 255, 200, 40);
	int thickness = std::max(2, (int)(3 * mainGame->xScale));
	for(int i = 1; i <= thickness; ++i) {	// drawn just outside the picture, so it isn't covered up
		irr::core::recti frame(left - i, top - i, right + i, bottom + i);
		mainGame->driver->draw2DRectangleOutline(frame, gold);
	}
}

const wchar_t* BotTheater::TurnText() const {
	if(!active || !turn_text[0] || mainGame->gameConf.hide_player_name)
		return nullptr;
	return turn_text;
}

void BotTheater::DrawInfoBorder() const {
	if(info_owner < 0 || !InfoFollowsActions() || !mainGame->dInfo.isStarted || !mainGame->wCardImg->isVisible())
		return;
	irr::video::SColor color = (info_owner == 0) ? irr::video::SColor(255, 40, 120, 255)
		: irr::video::SColor(255, 230, 40, 40);
	irr::core::recti r = mainGame->wCardImg->getAbsolutePosition();
	int thickness = std::max(2, (int)(3 * mainGame->xScale));
	for(int i = 0; i < thickness; ++i) {
		irr::core::recti frame(r.UpperLeftCorner.X + i, r.UpperLeftCorner.Y + i,
			r.LowerRightCorner.X - i, r.LowerRightCorner.Y - i);
		mainGame->driver->draw2DRectangleOutline(frame, color);
	}
}

static bool FileExists(const std::wstring& path) {
#ifdef _WIN32
	FILE* f = _wfopen(path.c_str(), L"rb");
#else
	FILE* f = std::fopen(ToUTF8(path).c_str(), "rb");
#endif
	if(!f)
		return false;
	std::fclose(f);
	return true;
}

static std::wstring FindAvatar(const wchar_t* name) {
	std::wstring safe;
	for(const wchar_t* p = name; *p; ++p)
		safe += std::wcschr(L"\\/:*?\"<>|", *p) ? L'_' : *p;
	if(safe.empty())
		return L"";
	static const wchar_t* exts[] = { L".png", L".jpg", L".jpeg" };
	for(auto ext : exts) {
		std::wstring path = L"textures/avatars/" + safe + ext;
		if(FileExists(path))
			return path;
	}
	return L"";
}

void BotTheater::OnDuelStart() {
	// Called when a duel starts, with the game's GUI lock already held.
	info_owner = -1;
	turn_text[0] = 0;
	turn_player = -1;
	holograms.clear();
	if(!active)
		return;
	int seats = mainGame->dInfo.isTag ? 4 : 2;
	for(int i = 0; i < seats; ++i) {
		std::wstring path = FindAvatar(mainGame->stHostPrepDuelist[i]->getToolTipText().c_str());
		imageManager.LoadLocalAvatar(i, path.empty() ? L"textures/avatar.png" : path.c_str());
	}
}

// ---------------------------------------------------------------------------
// Holograms: when a card turns up face-up in a Monster, Spell & Trap or Pendulum
// zone, its art rises out of the zone with a cyan glow. It then either rests
// (smaller and dimmer) or fades, and it fades out once the card leaves.
// Spell/Trap/Pendulum ("back row") holograms are smaller and float lower.

void BotTheater::MakeHologramTextures() {
	auto driver = mainGame->driver;
	if(!tWhite) {
		irr::video::IImage* img = driver->createImage(irr::video::ECF_A8R8G8B8, irr::core::dimension2du(4, 4));
		img->fill(irr::video::SColor(255, 255, 255, 255));
		tWhite = driver->addTexture("bot_theater_white", img);
		img->drop();
	}
	if(!tGlow) {
		// Soft-edged rectangle: solid in the middle, fading to nothing at the edges.
		const int size = 64;
		irr::video::IImage* img = driver->createImage(irr::video::ECF_A8R8G8B8, irr::core::dimension2du(size, size));
		for(int y = 0; y < size; ++y) {
			for(int x = 0; x < size; ++x) {
				float dx = std::fabs((x + 0.5f) / size * 2 - 1), dy = std::fabs((y + 0.5f) / size * 2 - 1);
				float edge = std::max(dx, dy);	// 0 in the middle, 1 at the border
				float a = edge < 0.6f ? 1.0f : 1.0f - (edge - 0.6f) / 0.4f;
				a = a * a;
				img->setPixel(x, y, irr::video::SColor((irr::u32)(a * 255), 255, 255, 255));
			}
		}
		tGlow = driver->addTexture("bot_theater_glow", img);
		img->drop();
	}
}

static void DrawQuad(irr::video::IVideoDriver* driver, const irr::core::vector3df& tl, const irr::core::vector3df& tr,
	const irr::core::vector3df& bl, const irr::core::vector3df& br, irr::video::SColor top, irr::video::SColor bottom,
	float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1) {
	irr::core::vector3df n(0, 0, 1);
	irr::video::S3DVertex v[4] = {
		irr::video::S3DVertex(tl, n, top, irr::core::vector2df(u1, v1)),
		irr::video::S3DVertex(tr, n, top, irr::core::vector2df(u2, v1)),
		irr::video::S3DVertex(bl, n, bottom, irr::core::vector2df(u1, v2)),
		irr::video::S3DVertex(br, n, bottom, irr::core::vector2df(u2, v2)),
	};
	irr::u16 idx[6] = { 0, 1, 2, 2, 1, 3 };
	driver->drawVertexPrimitiveList(v, 4, idx, 2);
}

void BotTheater::ScanForNewHolograms() {
	// Give every face-up card in a monster or back-row zone a hologram when it first
	// appears there. This covers summons, activations, Pendulum Scales, cards placed
	// face-up by effects, and cards flipped face-up.
	const unsigned char FACE_UP = 0x05;
	auto has_hologram = [this](ClientCard* c) {
		for(auto& h : holograms)
			if(h.card == c && h.leave_frame < 0)
				return true;
		return false;
	};
	for(int side = 0; side < 2; ++side) {
		for(int row = 0; row < 2; ++row) {
			bool backrow = (row == 1);
			if(backrow && !hologram_backrow)
				continue;
			auto& zones = backrow ? mainGame->dField.szone[side] : mainGame->dField.mzone[side];
			int count = std::min((int)zones.size(), backrow ? 8 : 7);
			for(int s = 0; s < count; ++s) {
				if(backrow && s == 5)
					continue;	// Field Zone: the field spell already shows as the background
				ClientCard* c = zones[s];
				if(c && c->code && (c->position & FACE_UP) && !has_hologram(c) && holograms.size() < 32)
					holograms.push_back(Hologram{ c->code, side, s, backrow, 0, c, -1 });
			}
		}
	}
}

void BotTheater::DrawHolograms() {
	// Called every frame from the field drawing, with the game's GUI lock held.
	if(!holograms_on || !SpectatingBotRoom() || !mainGame->dInfo.isStarted) {
		holograms.clear();
		return;
	}
	ScanForNewHolograms();
	if(holograms.empty())
		return;
	MakeHologramTextures();
	auto driver = mainGame->driver;
	driver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
	irr::video::SMaterial mat;
	// Same blending the game uses for cards, but taking transparency from both the
	// image (for the soft glow) and the vertex colours (for fading in and out).
	mat.MaterialType = irr::video::EMT_ONETEXTURE_BLEND;
	mat.MaterialTypeParam = irr::video::pack_textureBlendFunc(irr::video::EBF_SRC_ALPHA, irr::video::EBF_ONE_MINUS_SRC_ALPHA,
		irr::video::EMFN_MODULATE_1X, irr::video::EAS_VERTEX_COLOR | irr::video::EAS_TEXTURE);
	mat.setFlag(irr::video::EMF_LIGHTING, false);
	mat.setFlag(irr::video::EMF_ZBUFFER, false);	// always drawn on top of the field
	mat.setFlag(irr::video::EMF_ZWRITE_ENABLE, false);
	mat.setFlag(irr::video::EMF_BACK_FACE_CULLING, false);

	// Face the game's fixed camera (at (4.2, 8, 7.8), looking at (4.2, 0, 0)).
	irr::core::vector3df view(0, -8.0f, -7.8f);
	view.normalize();
	irr::core::vector3df up = irr::core::vector3df(0, 0, 1) - view * view.Z;
	up.normalize();
	irr::core::vector3df right(1, 0, 0);

	const int rise = 18, fade = 18, settle = 20;	// frames (the game runs at 60 a second)
	const unsigned char FACE_UP = 0x05;
	int full = std::max(rise, (int)(hologram_seconds * 60));
	float size_scale = std::max(10, hologram_size) / 100.0f;
	float backrow_scale = std::max(10, hologram_backrow_size) / 100.0f;
	float opacity = std::min(100, std::max(5, hologram_opacity)) / 100.0f;
	float rest_scale = std::max(10, hologram_rest_size) / 100.0f;
	float rest_alpha = std::min(100, std::max(0, hologram_rest_opacity)) / 100.0f;
	int rule = (mainGame->dInfo.duel_rule >= 4) ? 1 : 0;

	for(size_t i = 0; i < holograms.size();) {
		Hologram& h = holograms[i];
		auto& field = mainGame->dField;

		// Follow the card: start the fade-out once it leaves its zone or turns face-down.
		if(h.leave_frame < 0) {
			auto& zones = h.backrow ? field.szone[h.side] : field.mzone[h.side];
			auto& other_side = h.backrow ? field.szone[1 - h.side] : field.mzone[1 - h.side];
			ClientCard* here = (h.sequence < (int)zones.size()) ? zones[h.sequence] : nullptr;
			if(here != h.card) {
				ClientCard* there = (h.sequence < (int)other_side.size()) ? other_side[h.sequence] : nullptr;
				if(there == h.card)
					h.side = 1 - h.side;	// the spectator swapped sides
				else
					h.leave_frame = 0;	// destroyed, banished, bounced, sent to the Graveyard, moved...
			}
			if(h.leave_frame < 0 && (h.card->code != h.code || !(h.card->position & FACE_UP)))
				h.leave_frame = 0;	// flipped face-down, or became a different card (which gets its own hologram)
		}
		if(h.leave_frame >= fade) {
			holograms.erase(holograms.begin() + i);
			continue;
		}

		// Animation: rise and grow, hover at full size, then either settle down to the
		// resting size/opacity or fade away; fade out early if the card leaves.
		float alpha, scale, lift;
		if(h.frame < rise) {
			float p = (h.frame + 1) / (float)rise;
			p = 1 - (1 - p) * (1 - p);
			alpha = p;
			scale = 0.4f + 0.6f * p;
			lift = 0.15f + 0.35f * p;
		} else if(h.frame < full) {
			alpha = 1;
			scale = 1;
			lift = 0.5f;
		} else if(hologram_stay) {
			float k = std::min(1.0f, (h.frame - full) / (float)settle);
			alpha = 1 + (rest_alpha - 1) * k;
			scale = 1 + (rest_scale - 1) * k;
			lift = 0.5f - 0.1f * k;
		} else {
			float q = std::max(0.0f, 1 - (h.frame - full) / (float)fade);
			alpha = q;
			scale = 1.0f + 0.08f * (1 - q);
			lift = 0.5f + 0.15f * (1 - q);
		}
		if(h.leave_frame >= 0) {
			float q = 1 - h.leave_frame / (float)fade;
			alpha *= q;
			scale *= 1.0f + 0.08f * (1 - q);
			lift += 0.15f * (1 - q);
		}
		lift += 0.04f * std::sin(h.frame * 0.1f);
		alpha *= opacity;
		if(h.backrow) {
			scale *= backrow_scale;
			lift *= 0.5f;	// back-row holograms float lower, so they cover less of the monster row
		}
		if(h.frame < 1000000)
			++h.frame;
		if(h.leave_frame >= 0)
			++h.leave_frame;
		++i;
		if(alpha <= 0.002f)
			continue;	// faded away, but still following the card so it doesn't reappear

		const irr::video::S3DVertex* zone = h.backrow ? matManager.vFieldSzone[h.side][h.sequence][rule]
			: matManager.vFieldMzone[h.side][h.sequence];
		irr::core::vector3df center = (zone[0].Pos + zone[1].Pos + zone[2].Pos + zone[3].Pos) / 4.0f;

		// Crop to the art box (measured from standard card images); Pendulums stop above their text box.
		bool pendulum = false;
		auto cp = dataManager.GetCodePointer(h.code);
		if(cp != dataManager.datas_end() && (cp->second.type & 0x1000000))
			pendulum = true;
		float u1 = 50 / 421.0f, u2 = 371 / 421.0f, v1 = 113 / 614.0f, v2 = (pendulum ? 386 : 434) / 614.0f;
		float width = 1.2f * size_scale * scale;
		float height = width * ((v2 - v1) * 614.0f) / ((u2 - u1) * 421.0f);

		irr::core::vector3df base = center + irr::core::vector3df(0, 0, lift);
		irr::core::vector3df half = right * (width / 2);
		irr::core::vector3df tall = up * height;
		irr::core::vector3df tl = base - half + tall, tr = base + half + tall, bl = base - half, br = base + half;

		auto a = [alpha](float f) { return (irr::u32)std::min(255.0f, std::max(0.0f, 255 * alpha * f)); };
		irr::video::SColor cyanTop(a(0.15f), 80, 220, 255), cyanBottom(a(0.45f), 80, 220, 255);

		// Projector beam from the card to the bottom of the art.
		mat.setTexture(0, tWhite);
		driver->setMaterial(mat);
		irr::core::vector3df floorHalf = right * (h.backrow ? 0.22f : 0.3f);
		irr::core::vector3df floor = center + irr::core::vector3df(0, 0, 0.02f);
		DrawQuad(driver, bl, br, floor - floorHalf, floor + floorHalf, cyanTop, cyanBottom);

		// Soft glow behind the art.
		irr::core::vector3df gHalf = half * 1.25f, gExtra = up * (height * 0.125f);
		mat.setTexture(0, tGlow);
		driver->setMaterial(mat);
		irr::video::SColor glow(a(0.55f), 80, 220, 255);
		DrawQuad(driver, base - gHalf + tall + gExtra, base + gHalf + tall + gExtra, base - gHalf - gExtra, base + gHalf - gExtra, glow, glow);

		// The art itself, slightly cyan-tinted.
		irr::video::ITexture* art = imageManager.GetTexture(h.code);
		if(art) {
			mat.setTexture(0, art);
			driver->setMaterial(mat);
			irr::video::SColor tint(a(1.0f), 215, 245, 255);
			DrawQuad(driver, tl, tr, bl, br, tint, tint, u1, v1, u2, v2);
		}

		// Thin bright frame.
		mat.setTexture(0, tWhite);
		driver->setMaterial(mat);
		irr::video::SColor edge(a(0.9f), 140, 235, 255);
		float t = width * 0.015f;
		irr::core::vector3df tx = right * t, ty = up * t;
		DrawQuad(driver, tl, tr, tl - ty, tr - ty, edge, edge);			// top
		DrawQuad(driver, bl + ty, br + ty, bl, br, edge, edge);			// bottom
		DrawQuad(driver, tl, tl + tx, bl, bl + tx, edge, edge);			// left
		DrawQuad(driver, tr - tx, tr, br - tx, br, edge, edge);			// right
	}
}

// ---------------------------------------------------------------------------
// Raising the top player's hand. The game places hand cards on the board, seen
// through its fixed camera at (4.2, 8, 7.8) looking at (4.2, 0, 0). Moving the hand
// further back by about 1.955 board units raises it on screen by one card height.

static const float TOP_HAND_Y = -3.4f, HAND_Z = 0.5f;
static const float SHIFT_PER_CARD_HEIGHT = 1.955f;

// Distance from the camera, and how far above the camera's centre line, a point appears.
static void CameraDepth(float y, float z, float& depth, float& height) {
	const float fy = -0.71601f, fz = -0.69811f;	// viewing direction
	const float uy = -0.69811f, uz = 0.71601f;	// camera's "up"
	depth = (y - 8.0f) * fy + (z - 7.8f) * fz;
	height = (y - 8.0f) * uy + (z - 7.8f) * uz;
}

float BotTheater::TopHandShift() const {
	if(top_hand_raise <= 0 || !SpectatingBotRoom())
		return 0;
	return (float)std::min(5.0, top_hand_raise) * SHIFT_PER_CARD_HEIGHT;
}

void BotTheater::MapTopHandPoint(int& x, int& y) const {
	float shift = TopHandShift();
	if(shift <= 0)
		return;
	// Screen positions here are in the game's base 1024x640 layout.
	float d0, h0, d1, h1;
	CameraDepth(TOP_HAND_Y, HAND_Z, d0, h0);
	CameraDepth(TOP_HAND_Y - shift, HAND_Z, d1, h1);
	auto screen_y = [](float d, float h) { return (0.42f - h / d) / 0.84f * 640.0f; };
	const float axis_x = 0.9f / 1.35f * 1024.0f;	// where the camera's centre line meets the screen
	float shrink = d1 / d0;	// the raised hand is further away, so it looks smaller
	x = (int)(axis_x + (x - axis_x) * shrink);
	y = (int)(screen_y(d0, h0) + (y - screen_y(d1, h1)) * shrink);
}

void BotTheater::ShowWindowAfterRoom() const {
	// Go back to the host window (settings already filled in) so another
	// bot duel is one click away.
	mainGame->btnHostConfirm->setEnabled(true);
	mainGame->btnHostCancel->setEnabled(true);
	mainGame->ShowElement(mainGame->wCreateHost);
}

void BotTheater::SystemMessage(const wchar_t* msg) const {
	mainGame->gMutex.lock();
	mainGame->AddChatMsg(msg, 8);	// shown as [System], not as a script error
	mainGame->gMutex.unlock();
}

bool BotTheater::LaunchBots(std::wstring& error) {
#ifdef _WIN32
	wchar_t num[16];
	myswprintf(num, L"%d", (int)room_port);
	std::wstring cmd = L"\"" + python + L"\" \"" + script + L"\" --launch --port " + num;
	if(room_is_tag)
		cmd += L" --tag";
	if(!room_password.empty())
		cmd += L" --password \"" + room_password + L"\"";
	if(!script_args.empty())
		cmd += L" " + script_args;

	// The script's output goes to bot_theater.log, so problems can be checked.
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof sa;
	sa.lpSecurityDescriptor = nullptr;
	sa.bInheritHandle = TRUE;
	HANDLE log = CreateFileW(LOG_FILE, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	STARTUPINFOEXW si;
	std::memset(&si, 0, sizeof si);
	si.StartupInfo.cb = sizeof si;
	DWORD flags = CREATE_NO_WINDOW;
	BOOL inherit = FALSE;
	std::vector<char> attr_buf;
	if(log != INVALID_HANDLE_VALUE) {
		// Pass on only the log file, not the game's own network connections.
		SIZE_T attr_size = 0;
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_size);
		attr_buf.resize(attr_size);
		auto attrs = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attr_buf.data());
		if(InitializeProcThreadAttributeList(attrs, 1, 0, &attr_size)
			&& UpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &log, sizeof log, nullptr, nullptr)) {
			si.lpAttributeList = attrs;
			si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
			si.StartupInfo.hStdOutput = log;
			si.StartupInfo.hStdError = log;
			si.StartupInfo.hStdInput = nullptr;
			flags |= EXTENDED_STARTUPINFO_PRESENT;
			inherit = TRUE;
		}
	}
	std::vector<wchar_t> cmdline(cmd.begin(), cmd.end());
	cmdline.push_back(0);
	PROCESS_INFORMATION pi;
	std::memset(&pi, 0, sizeof pi);
	BOOL ok = CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, inherit, flags, nullptr, nullptr,
		&si.StartupInfo, &pi);
	DWORD err = GetLastError();
	if(si.lpAttributeList)
		DeleteProcThreadAttributeList(si.lpAttributeList);
	if(log != INVALID_HANDLE_VALUE)
		CloseHandle(log);
	if(!ok) {
		wchar_t ebuf[32];
		myswprintf(ebuf, L"Windows error %u", (unsigned)err);
		error = ebuf;
		return false;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return true;
#else
	error = L"only supported on Windows";
	return false;
#endif
}

}
