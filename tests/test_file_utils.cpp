#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "myheader.hpp"

namespace fs = std::filesystem;

class FileUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure dummy dir exists
        fs::create_directories(DUMMY_DIR);
    }

    void TearDown() override {
        // Clean up dummy dir
        std::error_code ec;
        fs::remove_all(DUMMY_DIR, ec);
    }
};

TEST_F(FileUtilsTest, IsRegularFile_ExtensionVariants) {
    std::string f1 = std::string(DUMMY_DIR) + "/test.txt";
    std::string f2 = std::string(DUMMY_DIR) + "/test.cpp";
    std::string f3 = std::string(DUMMY_DIR) + "/test.md";
    std::ofstream(f1).close();
    std::ofstream(f2).close();
    std::ofstream(f3).close();
    
    EXPECT_TRUE(isRegularFile(fs::path(f1)));
    EXPECT_TRUE(isRegularFile(fs::path(f2)));
    EXPECT_TRUE(isRegularFile(fs::path(f3)));
}

TEST_F(FileUtilsTest, IsBinaryFile_ContentVariants) {
    std::string txt = std::string(DUMMY_DIR) + "/plain.txt";
    std::string bin = std::string(DUMMY_DIR) + "/raw.bin";
    
    std::ofstream(txt) << "this is just text";
    std::ofstream(bin, std::ios::binary) << "text\0binary\x01\x02" << std::string(500, '\0');
    
    EXPECT_FALSE(isBinaryFile(fs::path(txt)));
    EXPECT_TRUE(isBinaryFile(fs::path(bin)));
}

TEST_F(FileUtilsTest, IsReadable_Boundary) {
    std::string f = std::string(DUMMY_DIR) + "/secret.txt";
    std::ofstream(f).close();
    
    EXPECT_TRUE(isReadable(fs::path(f)));
    
    // Non-existent
    EXPECT_FALSE(isReadable(fs::path(std::string(DUMMY_DIR) + "/missing")));
}

TEST_F(FileUtilsTest, IsDirectory_False) {
    std::string f = std::string(DUMMY_DIR) + "/not_a_dir";
    std::ofstream(f).close();
    EXPECT_FALSE(fs::is_directory(fs::path(f)));
}

TEST_F(FileUtilsTest, PermissionRestricted) {
    std::string f = std::string(DUMMY_DIR) + "/no_access";
    std::ofstream(f).close();
    chmod(f.c_str(), 0000);
    
    // Depending on root/user, is_readable might still be true, but usually false for 000
    // We just ensure it doesn't crash
    isReadable(fs::path(f));
    
    chmod(f.c_str(), 0644); // Cleanup
}

TEST_F(FileUtilsTest, IsDirectory_True) {
    fs::create_directories(std::string(DUMMY_DIR) + "/subdir");
    EXPECT_TRUE(isDirectory(std::string(DUMMY_DIR) + "/subdir"));
}

TEST_F(FileUtilsTest, IsDirectory_False_Original) {
    std::string file_path = std::string(DUMMY_DIR) + "/file.txt";
    std::ofstream out(file_path);
    out << "test content";
    out.close();
    EXPECT_FALSE(isDirectory(file_path));
}

TEST_F(FileUtilsTest, IsRegularFile_True) {
    std::string file_path = std::string(DUMMY_DIR) + "/file.txt";
    std::ofstream out(file_path);
    out << "test content";
    out.close();
    EXPECT_TRUE(isRegularFile(file_path));
}

TEST_F(FileUtilsTest, IsBinaryFile_TextFile) {
    std::string file_path = std::string(DUMMY_DIR) + "/text.txt";
    std::ofstream out(file_path);
    out << "hello world\n";
    out.close();
    EXPECT_FALSE(isBinaryFile(file_path));
}

TEST_F(FileUtilsTest, IsBinaryFile_BinaryFile) {
    std::string file_path = std::string(DUMMY_DIR) + "/binary.bin";
    std::ofstream out(file_path, std::ios::binary);
    out.put('\0'); // Null byte should trigger isBinaryFile
    out.close();
    EXPECT_TRUE(isBinaryFile(file_path));
}

TEST_F(FileUtilsTest, IsReadable_Readable) {
    std::string file_path = std::string(DUMMY_DIR) + "/r.txt";
    std::ofstream out(file_path);
    out << "readable content";
    out.close();
    EXPECT_TRUE(isReadable(file_path));
}

TEST_F(FileUtilsTest, IsReadable_NoPermission) {
    std::string file_path = std::string(DUMMY_DIR) + "/noread.txt";
    std::ofstream out(file_path);
    out << "secret";
    out.close();
    
    // Make unreadable
    chmod(file_path.c_str(), 0000);
    EXPECT_FALSE(isReadable(file_path));
    
    // Restore for cleanup (remove_all might fail otherwise)
    chmod(file_path.c_str(), 0644);
}
