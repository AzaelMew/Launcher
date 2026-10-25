#include <WinSock2.h>
#include <Windows.h>
#include <uxtheme.h>

#include <cstdlib>
#include <cstring>

#include <wx/wx.h>
#include <wx/app.h>
#include <wx/eventfilter.h>

#include "launcher/wine_dark_mode.h"
#include "shared/logger.h"

#pragma comment(lib, "uxtheme.lib")

namespace {
	struct Colour {
		int index;
		COLORREF dark;
	};

	const Colour colours[] = {
		{ COLOR_WINDOW,        RGB(23, 26, 33) },
		{ COLOR_WINDOWTEXT,    RGB(235, 237, 242) },
		{ COLOR_BTNFACE,       RGB(28, 31, 39) },
		{ COLOR_BTNTEXT,       RGB(235, 237, 242) },
		{ COLOR_CAPTIONTEXT,   RGB(235, 237, 242) },
		{ COLOR_GRAYTEXT,      RGB(110, 116, 130) },
		{ COLOR_BTNSHADOW,     RGB(60, 64, 75) },
		{ COLOR_WINDOWFRAME,   RGB(60, 64, 75) },
		{ COLOR_3DDKSHADOW,    RGB(10, 10, 14) },
		{ COLOR_3DLIGHT,       RGB(60, 64, 75) },
		{ COLOR_BTNHIGHLIGHT,  RGB(90, 95, 108) },
		{ COLOR_MENU,          RGB(28, 31, 39) },
		{ COLOR_MENUTEXT,      RGB(235, 237, 242) },
	};
	constexpr int count = sizeof(colours) / sizeof(colours[0]);

	COLORREF original[count];
	bool overridden = false;

	bool IsWine() {
		HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
		return ntdll && GetProcAddress(ntdll, "wine_get_version");
	}

	// Without visual styles, most controls are drawn using the system colours above.
	void DisableVisualStyles(wxWindow* window) {
		SetWindowTheme(static_cast<HWND>(window->GetHandle()), L"", L"");
		// Wine paints push buttons and group box titles from its light visual
		// style, so give them explicit colours; wxWidgets then draws them itself.
		// Push buttons are made borderless: the frame wxWidgets draws is the
		// bright theme one, which does not follow the colour table.
		if (wxDynamicCast(window, wxButton)) {
			window->SetWindowStyleFlag(window->GetWindowStyleFlag() | wxBORDER_NONE);
			window->SetBackgroundColour(wxColour(78, 83, 97));
			window->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
		} else if (wxDynamicCast(window, wxStaticBox)) {
			window->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
			window->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
		}
		for (wxWindow* child : window->GetChildren()) {
			DisableVisualStyles(child);
		}
	}

	class ShowFilter final : public wxEventFilter {
	public:
		int FilterEvent(wxEvent& event) override {
			if (event.GetEventType() == wxEVT_SHOW && static_cast<wxShowEvent&>(event).IsShown()) {
				wxWindow* window = wxDynamicCast(event.GetEventObject(), wxWindow);
				if (window && window->IsTopLevel()) {
					DisableVisualStyles(window);
				}
			}
			return Event_Skip;
		}
	};

	ShowFilter filter;

	void RestoreColours() {
		if (!overridden) {
			return;
		}

		int indices[count];
		for (int i = 0; i < count; ++i) {
			indices[i] = colours[i].index;
		}
		SetSysColors(count, indices, original);
		overridden = false;
	}

	// OnExit() is not reached on every path out of OnInit().
	struct RestoreAtExit {
		~RestoreAtExit() { RestoreColours(); }
	} restoreAtExit;
}

namespace Launcher::WineDarkMode {
	void Enable() {
		if (overridden || !IsWine()) {
			return;
		}

		const char* theme = std::getenv("REPENTOGON_LAUNCHER_THEME");
		if (theme && !_stricmp(theme, "light")) {
			return;
		}

		int indices[count];
		COLORREF dark[count];
		for (int i = 0; i < count; ++i) {
			indices[i] = colours[i].index;
			dark[i] = colours[i].dark;
			original[i] = GetSysColor(colours[i].index);
		}

		// Wine only keeps this in memory for the lifetime of the session.
		if (!SetSysColors(count, indices, dark)) {
			return;
		}
		overridden = true;
		wxEvtHandler::AddFilter(&filter);

		// App::OnInit() requested the light appearance because Wine reports
		// light apps. Without wxWidgets' dark mode, text box borders are drawn
		// with Wine's light visual style regardless of the colours above.
		wxTheApp->SetAppearance(wxApp::Appearance::Dark);
		Logger::Info("Running under Wine: dark system colours enabled "
			"(set REPENTOGON_LAUNCHER_THEME=light to disable)\n");
	}

	void Disable() {
		if (overridden) {
			wxEvtHandler::RemoveFilter(&filter);
			RestoreColours();
		}
	}
}
