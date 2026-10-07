// PageTurnSkillBooks - skill books grant their bonus only after the player has
// flipped through a few pages, instead of the moment the book is opened.
//
// Jak to dziala:
//  1. TESObjectBOOK::Read (funkcja nadajaca bonus do umiejetnosci) jest hakowana
//     MinHookiem. Gdy gracz otwiera ksiazke uczaca umiejetnosci, wywolujemy
//     oryginal z TYMCZASOWO zdjeta flaga kAdvancesActorValue - ksiazka jest
//     "otwarta", ale bonus nie jest nadany. Flagi zaraz wracaja do stanu
//     poczatkowego, wiec stan gry (ksiazka nieprzeczytana) nie zmienia sie.
//  2. Dopoki otwarte jest "Book Menu", zliczamy przewrocone strony na podstawie
//     zdarzen wejscia (klawiatura / pad; konfigurowalne w INI).
//  3. Po osiagnieciu progu wywolujemy oryginalne Read z flaga bypass - gra nadaje
//     bonus, pokazuje komunikat i sama zapisuje stan (flaga kAdvancesActorValue
//     znika, wiec zapis gry dziala jak w vanilli, bez wlasnego cosave).
//  4. Zamkniecie ksiazki przed progiem = brak bonusu, licznik startuje od nowa.

#include "PCH.h"

#include <MinHook.h>

namespace
{
	// ------------------------------------------------------------------ config
	struct Config
	{
		bool                     enabled{ true };
		int                      pageTurns{ 3 };
		bool                     logInput{ false };
		std::vector<std::string> forward{ "Right", "NextPage", "Click" };
		std::vector<std::string> back{ "Left", "PrevPage" };
	};

	Config g_cfg;

	std::vector<std::string> SplitList(const std::string& a_text)
	{
		std::vector<std::string> out;
		std::string              cur;
		auto                     flush = [&]() {
            const auto b = cur.find_first_not_of(" \t");
            const auto e = cur.find_last_not_of(" \t");
            if (b != std::string::npos) {
                out.emplace_back(cur.substr(b, e - b + 1));
            }
            cur.clear();
		};
		for (const char c : a_text) {
			if (c == ',') {
				flush();
			} else {
				cur.push_back(c);
			}
		}
		flush();
		return out;
	}

	void LoadConfig()
	{
		constexpr auto path = ".\\Data\\SKSE\\Plugins\\PageTurnSkillBooks.ini";

		g_cfg.enabled = GetPrivateProfileIntA("General", "bEnabled", 1, path) != 0;
		g_cfg.pageTurns = std::max(1, static_cast<int>(GetPrivateProfileIntA("General", "iPageTurns", 3, path)));
		g_cfg.logInput = GetPrivateProfileIntA("Debug", "bLogInput", 0, path) != 0;

		char buf[256]{};
		GetPrivateProfileStringA("Input", "sForwardEvents", "Right,NextPage,Click", buf, sizeof(buf), path);
		if (auto v = SplitList(buf); !v.empty()) {
			g_cfg.forward = std::move(v);
		}
		GetPrivateProfileStringA("Input", "sBackEvents", "Left,PrevPage", buf, sizeof(buf), path);
		g_cfg.back = SplitList(buf);  // moze byc puste

		logger::info("Config: enabled={} pageTurns={} logInput={}", g_cfg.enabled, g_cfg.pageTurns, g_cfg.logInput);
	}

	bool Matches(const std::vector<std::string>& a_list, std::string_view a_name)
	{
		return std::any_of(a_list.begin(), a_list.end(), [&](const std::string& s) {
			return _stricmp(s.c_str(), std::string(a_name).c_str()) == 0;
		});
	}

	// -------------------------------------------------------------------- hook
	namespace Hook
	{
		using Read_t = bool (*)(RE::TESObjectBOOK*, RE::TESObjectREFR*);

		inline Read_t origRead{ nullptr };
		inline bool   bypass{ false };

		bool Read(RE::TESObjectBOOK* a_book, RE::TESObjectREFR* a_reader)
		{
			const auto* player = RE::PlayerCharacter::GetSingleton();

			if (g_cfg.enabled && !bypass && a_book && player && a_reader == player &&
				a_book->TeachesSkill() && !a_book->TeachesSpell()) {
				// "Otworz" ksiazke bez nadania bonusu, potem przywroc flagi.
				const auto saved = a_book->data.flags;
				a_book->data.flags.reset(RE::OBJ_BOOK::Flag::kAdvancesActorValue);
				const bool result = origRead(a_book, a_reader);
				a_book->data.flags = saved;
				return result;
			}
			return origRead(a_book, a_reader);
		}

		bool Install()
		{
			if (MH_Initialize() != MH_OK) {
				logger::error("MH_Initialize failed");
				return false;
			}
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(17439, 17842) };
			auto*                           addr = reinterpret_cast<void*>(target.address());

			if (MH_CreateHook(addr, reinterpret_cast<void*>(&Read), reinterpret_cast<void**>(&origRead)) != MH_OK) {
				logger::error("MH_CreateHook failed for TESObjectBOOK::Read @ {:X}", target.address());
				return false;
			}
			if (MH_EnableHook(addr) != MH_OK) {
				logger::error("MH_EnableHook failed");
				return false;
			}
			logger::info("Hooked TESObjectBOOK::Read @ {:X}", target.address());
			return true;
		}
	}

	// ----------------------------------------------------------------- tracker
	class Tracker :
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static Tracker* GetSingleton()
		{
			static Tracker singleton;
			return std::addressof(singleton);
		}

		static void Register()
		{
			auto* self = GetSingleton();
			RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(
				static_cast<RE::BSTEventSink<RE::MenuOpenCloseEvent>*>(self));
			RE::BSInputDeviceManager::GetSingleton()->AddEventSink(
				static_cast<RE::BSTEventSink<RE::InputEvent*>*>(self));
			logger::info("Event sinks registered");
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
			RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
		{
			if (a_event && a_event->menuName == RE::BookMenu::MENU_NAME.data()) {
				if (a_event->opening) {
					Begin();
				} else {
					End();
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
			RE::BSTEventSource<RE::InputEvent*>*) override
		{
			if (!_book || _granted || !a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}

			for (auto* e = *a_event; e; e = e->next) {
				if (e->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
					continue;
				}
				auto* button = e->AsButtonEvent();
				if (!button || !button->IsDown()) {
					continue;
				}
				const char* raw = button->QUserEvent().c_str();
				if (!raw) {
					continue;
				}
				const std::string_view name{ raw };

				if (g_cfg.logInput) {
					logger::info("BookMenu input: '{}'", name);
				}

				if (Matches(g_cfg.forward, name)) {
					++_progress;
					_best = std::max(_best, _progress);
				} else if (Matches(g_cfg.back, name)) {
					_progress = std::max(0, _progress - 1);
				} else {
					continue;
				}

				if (_best >= g_cfg.pageTurns) {
					Grant();
					break;
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		void Begin()
		{
			_book = nullptr;
			_progress = 0;
			_best = 0;
			_granted = false;

			if (!g_cfg.enabled) {
				return;
			}
			auto* book = RE::BookMenu::GetTargetForm();
			if (book && book->TeachesSkill() && !book->TeachesSpell()) {
				_book = book;
				logger::info("Skill book opened: {} ({:08X}), need {} page turns",
					book->GetName(), book->GetFormID(), g_cfg.pageTurns);
			}
		}

		void End()
		{
			if (_book && !_granted) {
				logger::info("Book closed early ({} / {} turns) - no bonus", _best, g_cfg.pageTurns);
			}
			_book = nullptr;
		}

		void Grant()
		{
			_granted = true;
			auto* book = _book;
			logger::info("Page threshold reached - granting skill bonus");

			SKSE::GetTaskInterface()->AddTask([book]() {
				auto* player = RE::PlayerCharacter::GetSingleton();
				if (player && book && book->TeachesSkill()) {
					Hook::bypass = true;
					book->Read(player);  // przechodzi przez hook -> oryginal z bypass
					Hook::bypass = false;
				}
			});
		}

		RE::TESObjectBOOK* _book{ nullptr };
		int                _progress{ 0 };
		int                _best{ 0 };
		bool               _granted{ false };
	};

	// --------------------------------------------------------------------- log
	void SetupLog()
	{
		auto path = SKSE::log::log_directory();
		if (!path) {
			return;
		}
		*path /= "PageTurnSkillBooks.log";

		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S] [%l] %v"s);
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();
	logger::info("PageTurnSkillBooks loading");

	LoadConfig();
	if (!g_cfg.enabled) {
		logger::info("Disabled in INI - nothing hooked");
		return true;
	}
	if (!Hook::Install()) {
		return false;
	}

	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
		if (a_msg && a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
			Tracker::Register();
		}
	});

	logger::info("PageTurnSkillBooks loaded");
	return true;
}
