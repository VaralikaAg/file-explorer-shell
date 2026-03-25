#include "myheader.hpp"

// Stubs for TUI functions to satisfy the linker in tests
void renderUI() {}
void displaySearchResults() {}
void renderLeftPanel() {}
void showHelp() {}
void showStatusMessage(const std::string &, const std::string &, int) {}
void cleanupAndExit() { exit(0); }
void hideCursor() {}
void showCursor() {}
void renderMiddlePanel() {}
void renderRightPanel() {}
void renderFilePreview(const std::string &, bool) {}
void renderDirectoryPreview(const std::string &) {}

// Other globals needed if not defined elsewhere
// AppState app; // Already defined in test_main.cpp
