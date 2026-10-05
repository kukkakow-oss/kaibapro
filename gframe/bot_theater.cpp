#include "config.h"
#include "bot_theater.h"
#include "game.h"
#include "duelclient.h"
#include "network.h"
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

void BotTheater::ShowWindowAfterRoom() const {
	// Go back to the host window (settings already filled in) so another
	// bot duel is one click away.
	mainGame->btnHostConfirm->setEnabled(true);
	mainGame->btnHostCancel->setEnabled(true);
	mainGame->ShowElement(mainGame->wCreateHost);
}

void BotTheater::SystemMessage(const wchar_t* msg) const {
	mainGame->gMutex.lock();
	mainGame->AddChatMsg(msg, 9);
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
