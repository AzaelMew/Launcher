#pragma once

namespace Launcher::WineDarkMode {
	/* Wine/Proton do not implement the Windows dark mode APIs, so wxWidgets'
	 * SetAppearance() has no effect there and the launcher stays light.
	 *
	 * When running under Wine, Enable() overrides the process' system colour
	 * table with a dark palette and disables visual styles on the windows
	 * the launcher creates, so that every control draws from that table.
	 * Setting the environment variable REPENTOGON_LAUNCHER_THEME=light opts out.
	 *
	 * It does nothing on real Windows. Call it before any window is created,
	 * and call Disable() on exit.
	 */
	void Enable();
	void Disable();
}
