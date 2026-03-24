#include "myheader.h"

// Stubs for TUI functions to satisfy the linker in tests
void renderUI() {}
void displaySearchResults() {}
void print_details() {}
void showHelp() {}
void showStatusMessage(const std::string&, const std::string&) {}
void cleanupAndExit() { exit(0); }
void hideCursor() {}
void showCursor() {}
void showTempMessage(const std::string&, int) {}
void renderMiddlePanel() {}
void renderRightPanel() {}
void renderStatusBar() {}
void renderFilePreview(const std::string&, bool) {}
void renderDirectoryPreview(const std::string&) {}

// Other globals needed if not defined elsewhere
// AppState app; // Already defined in test_main.cpp
