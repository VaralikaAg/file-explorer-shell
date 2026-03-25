#ifndef ANSI_CODES_HPP
#define ANSI_CODES_HPP

#include <string>

namespace ANSI {

    // --- Reset ---
    const std::string RESET = "\033[0m"; // Reset all terminal attributes (color, style, etc.) to default.

    // --- Text Styles ---
    const std::string BOLD = "\033[1m"; // Enable bold text.
    const std::string INVERSE = "\033[7m"; // Swap foreground and background colors.

    // --- Bold Colors ---
    const std::string BOLD_RED = "\033[1;31m"; // Bold red text for errors or important warnings.
    const std::string BOLD_GREEN = "\033[1;32m"; // Bold green text for success messages.
    const std::string BOLD_YELLOW = "\033[1;33m"; // Bold yellow text for highlights or warnings.
    const std::string BOLD_BLUE = "\033[1;34m"; // Bold blue text for paths or informational headers.
    const std::string BOLD_MAGENTA = "\033[1;35m"; // Bold magenta text.
    const std::string BOLD_CYAN = "\033[1;36m"; // Bold cyan text for help commands or prompts.

    // --- Screen Control ---
    const std::string CLEAR_SCREEN = "\033[2J"; // Clear the entire terminal screen.
    const std::string CLEAR_LINE = "\033[K"; // Clear the current line from the cursor to the end of the line.
    const std::string HOME = "\033[H"; // Move the cursor to the top-left corner (1, 1).

    // --- Cursor Control ---
    const std::string HIDE_CURSOR = "\033[?25l"; // Make the terminal cursor invisible.
    const std::string SHOW_CURSOR = "\033[?25h"; // Make the terminal cursor visible.

    // --- Alternative Screen Buffer ---
    const std::string ALT_SCREEN_ON = "\033[?1049h"; // Switch to the alternative screen buffer (often used by full-screen TUI apps).
    const std::string ALT_SCREEN_OFF = "\033[?1049l"; // Switch back to the main screen buffer.
    const std::string MOUSE_TRACK_OFF = "\033[?1000l"; // Disable mouse tracking.

    // --- Custom Cursor Colors (Platform Specific / TUI Extensions) ---
    const std::string CURSOR_RED = "\033]12;#ff0000\007"; // Set the cursor color to red (supported by some terminals like iTerm2/Kitty).
    const std::string CURSOR_RESET = "\033]112;\007"; // Reset the cursor color to its default value.

    inline std::string MOVE_TO(int row, int col) {
        return "\033[" + std::to_string(row) + ";" + std::to_string(col) + "H";
    }

} // namespace ANSI

#endif // ANSI_CODES_H
