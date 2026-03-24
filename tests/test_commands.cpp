#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "myheader.h"

namespace fs = std::filesystem;

class CommandsTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::create_directories(DUMMY_DIR);
        app.nav.currPath = DUMMY_DIR;
        app.nav.fileList.clear();
        app.selection.selectedFiles.clear();
        app.selection.clipboard.clear();
        app.config.indexingEnabled = false; // Disable indexing for core command tests
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(DUMMY_DIR, ec);
    }
};

TEST_F(CommandsTest, ProcessCommand_Parsing) {
    EXPECT_TRUE(processCommand("").success);
    EXPECT_EQ(processCommand("q").message, "EXIT_COMMAND_MODE");
    
    // Create Dir success
    EXPECT_TRUE(processCommand("create_dir cmd_dir").success);
    EXPECT_TRUE(fs::is_directory(std::string(DUMMY_DIR) + "/cmd_dir"));
    
    // Duplicate
    EXPECT_FALSE(processCommand("create_dir cmd_dir").success);
    
    // CD
    EXPECT_TRUE(processCommand("cd " + std::string(DUMMY_DIR)).success);
}

TEST_F(CommandsTest, ProcessCommand_CdSpaces) {
    std::string spaceDir = std::string(DUMMY_DIR) + "/path with spaces";
    fs::create_directories(spaceDir);
    EXPECT_TRUE(processCommand("cd " + spaceDir).success);
    EXPECT_EQ(app.nav.currPath, spaceDir);
}

TEST_F(CommandsTest, CreateFile_Success) {
    bool res = createFile("test_file.txt");
    EXPECT_TRUE(res);
    EXPECT_TRUE(fs::exists(std::string(DUMMY_DIR) + "/test_file.txt"));
}

TEST_F(CommandsTest, CreateDir_Success) {
    bool res = createDirectory("test_dir");
    EXPECT_TRUE(res);
    EXPECT_TRUE(fs::is_directory(std::string(DUMMY_DIR) + "/test_dir"));
}

TEST_F(CommandsTest, Rename_Success) {
    std::string oldName = "old.txt";
    std::string newName = "new.txt";
    std::ofstream(std::string(DUMMY_DIR) + "/" + oldName).close();
    
    bool res = renameItem(oldName, newName);
    EXPECT_TRUE(res);
    EXPECT_TRUE(fs::exists(std::string(DUMMY_DIR) + "/" + newName));
}

TEST_F(CommandsTest, CopyPaste_RecursiveDir) {
    std::string srcDir = std::string(DUMMY_DIR) + "/src_dir";
    fs::create_directories(srcDir);
    std::ofstream(srcDir + "/inner.txt") << "content";
    
    app.nav.fileList = {"src_dir"};
    app.nav.xcurr = 1;
    app.nav.up_screen = 0;
    
    copy();
    
    std::string targetDir = std::string(DUMMY_DIR) + "/target";
    fs::create_directories(targetDir);
    app.nav.currPath = targetDir;
    
    paste();
    EXPECT_TRUE(fs::is_directory(targetDir + "/src_dir"));
    EXPECT_TRUE(fs::exists(targetDir + "/src_dir/inner.txt"));
}

TEST_F(CommandsTest, Paste_EmptyClipboard) {
    app.selection.clipboard.clear();
    std::string res = paste();
    EXPECT_EQ(res, "");
}

TEST_F(CommandsTest, Delete_WithoutSelection) {
    std::string fileName = "direct_delete.txt";
    std::string fullPath = std::string(DUMMY_DIR) + "/" + fileName;
    std::ofstream(fullPath).close();
    
    app.nav.fileList = {fileName};
    app.nav.xcurr = 1;
    app.selection.selectedFiles.clear();
    
    deleteSelectedItems();
    EXPECT_FALSE(fs::exists(fullPath));
}

TEST_F(CommandsTest, ProcessCommand_InvalidArgs) {
    EXPECT_FALSE(processCommand("create_file").success);
    EXPECT_FALSE(processCommand("rename").success);
    EXPECT_FALSE(processCommand("cd").success);
}

TEST_F(CommandsTest, CreateFile_WithSpaces) {
    // App handles spaces in 'cd' but maybe not in 'create_file' because it uses args[1]
    // Let's verify and test the current behavior
    processCommand("create_file file with spaces.txt");
    EXPECT_TRUE(fs::exists(std::string(DUMMY_DIR) + "/file"));
}

TEST_F(CommandsTest, Cd_InvalidPath) {
    CommandResult res = processCommand("cd /non/existent/path/at/all");
    EXPECT_TRUE(res.success); // Navigate might return success but not change path
    EXPECT_EQ(app.nav.currPath, DUMMY_DIR); // Should stay at dummy_dir
}

TEST_F(CommandsTest, Selection_DeletePriority) {
    std::string f1 = std::string(DUMMY_DIR) + "/f1.txt";
    std::string f2 = std::string(DUMMY_DIR) + "/f2.txt";
    std::ofstream(f1).close();
    std::ofstream(f2).close();
    
    app.nav.fileList = {"f1.txt", "f2.txt"};
    app.nav.xcurr = 1; // Points to f1.txt
    
    // Select f2.txt
    app.selection.selectedFiles.insert(f2);
    
    // deleteSelectedItems should delete f2.txt (selection), not f1.txt (cursor)
    deleteSelectedItems();
    EXPECT_FALSE(fs::exists(f2));
    EXPECT_TRUE(fs::exists(f1));
}

TEST_F(CommandsTest, Paste_CollisionOverwrite) {
    std::string src = std::string(DUMMY_DIR) + "/src.txt";
    std::ofstream(src) << "source content";
    
    app.selection.clipboard = {src};
    
    std::string sub = std::string(DUMMY_DIR) + "/sub";
    fs::create_directories(sub);
    std::ofstream(sub + "/src.txt") << "old content";
    
    app.nav.currPath = sub;
    paste();
    
    // Should overwrite
    std::ifstream ifs(sub + "/src.txt");
    std::string content;
    std::getline(ifs, content);
    EXPECT_EQ(content, "source content");
}

TEST_F(CommandsTest, Rename_IntoExisting) {
    std::ofstream(std::string(DUMMY_DIR) + "/a.txt").close();
    std::ofstream(std::string(DUMMY_DIR) + "/b.txt").close();
    
    // Rename a.txt to b.txt should fail (rename syscall behavior)
    bool res = renameItem("a.txt", "b.txt");
    // On many unix systems, rename(a, b) actually overwrites b if b is a file.
    // Let's see what the app does.
    EXPECT_TRUE(res); 
}

TEST_F(CommandsTest, Rename_InvalidChars) {
    std::ofstream(std::string(DUMMY_DIR) + "/valid.txt").close();
    // Path segments with / are invalid for rename
    EXPECT_FALSE(renameItem("valid.txt", "invalid/name.txt"));
}
