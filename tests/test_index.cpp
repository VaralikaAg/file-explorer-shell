#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include "myheader.h"

namespace fs = std::filesystem;

class IndexTest : public ::testing::Test {
protected:
    std::string dbPath = std::string(DUMMY_DIR) + "/lmdb_test";
    std::string dummyFile = std::string(DUMMY_DIR) + "/index_test.txt";

    void SetUp() override {
        fs::remove_all(dbPath);
        fs::create_directories(dbPath);
        fs::remove_all(dummyFile);
        
        // Initialize app config for indexing
        app.config.indexingRoot = DUMMY_DIR;
        app.config.indexingEnabled = true;
        
        app.indexing.index.open(dbPath);
    }

    void TearDown() override {
        app.indexing.index.close();
        fs::remove_all(dbPath);
        fs::remove_all(dummyFile);
    }
};

TEST_F(IndexTest, OpenClose) {
    // Already opened in SetUp, closed in TearDown
    // Testing that multiple opens don't crash
    EXPECT_NE(app.indexing.index.getEnv(), nullptr);
}

TEST_F(IndexTest, IndexAndSearch_SingleWord) {
    std::ofstream(dummyFile) << "rocket";
    app.indexing.index.indexPath(dummyFile);
    
    app.indexing.index.search("rocket");
    // Since we use volfs and inode resolution, let's verify if results are found
    // If running on a system where /.vol/ is restricted, this might be empty
    // But logically, it should find it.
    EXPECT_FALSE(app.search.foundPaths.empty());
}

TEST_F(IndexTest, IndexAndSearch_MultiWord_AND) {
    std::ofstream(dummyFile) << "hello world";
    app.indexing.index.indexPath(dummyFile);
    
    // Search for both words
    app.indexing.index.search("hello world");
    EXPECT_FALSE(app.search.foundPaths.empty());
    
    // Search for non-existent pair
    app.indexing.index.search("hello nonexistent");
    EXPECT_TRUE(app.search.foundPaths.empty());
}

TEST_F(IndexTest, StopwordNotIndexed) {
    std::ofstream(dummyFile) << "the and simple";
    app.indexing.index.indexPath(dummyFile);
    
    app.indexing.index.search("the");
    EXPECT_TRUE(app.search.foundPaths.empty());
    
    app.indexing.index.search("simple");
    EXPECT_FALSE(app.search.foundPaths.empty());
}

TEST_F(IndexTest, SpecialSymbolWord) {
    std::ofstream(dummyFile) << "user@corp #tag_name";
    app.indexing.index.indexPath(dummyFile);
    
    app.indexing.index.search("user@corp");
    EXPECT_FALSE(app.search.foundPaths.empty());
    
    app.indexing.index.search("#tag_name");
    EXPECT_FALSE(app.search.foundPaths.empty());
}

TEST_F(IndexTest, BinaryFileSkipped) {
    std::ofstream out(dummyFile, std::ios::binary);
    out << "text\0null\x01\x02" << std::string(500, '\0');
    out.close();
    
    app.indexing.index.indexPath(dummyFile);
    app.indexing.index.search("text");
    EXPECT_TRUE(app.search.foundPaths.empty());
}

TEST_F(IndexTest, RemovePath_Cleans) {
    std::ofstream(dummyFile) << "apple banana";
    app.indexing.index.indexPath(dummyFile);
    app.indexing.index.search("apple");
    ASSERT_FALSE(app.search.foundPaths.empty());
    
    app.indexing.index.removePath(dummyFile);
    app.indexing.index.search("apple");
    EXPECT_TRUE(app.search.foundPaths.empty());
}

TEST_F(IndexTest, UpdatePath) {
    std::ofstream(dummyFile) << "old_content";
    app.indexing.index.indexPath(dummyFile);
    
    // Change file content
    std::ofstream(dummyFile, std::ios::trunc) << "new_content";
    app.indexing.index.removePath(dummyFile);
    app.indexing.index.indexPath(dummyFile);
    
    app.indexing.index.search("old_content");
    EXPECT_TRUE(app.search.foundPaths.empty());
    
    app.indexing.index.search("new_content");
    EXPECT_FALSE(app.search.foundPaths.empty());
}

TEST_F(IndexTest, MultiFile_AND) {
    std::string f1 = std::string(DUMMY_DIR) + "/f1.txt";
    std::string f2 = std::string(DUMMY_DIR) + "/f2.txt";
    std::ofstream(f1) << "quick fox";
    std::ofstream(f2) << "lazy fox dog";
    
    app.indexing.index.indexPath(f1);
    app.indexing.index.indexPath(f2);
    
    app.indexing.index.search("fox");
    EXPECT_EQ(app.search.foundPaths.size(), 2);
    
    app.indexing.index.search("fox dog");
    ASSERT_EQ(app.search.foundPaths.size(), 1);
    EXPECT_EQ(app.search.foundPaths[0], f2);
}

TEST_F(IndexTest, LastSyncTime) {
    app.indexing.index.setLastSyncTime(987654321);
    EXPECT_EQ(app.indexing.index.getLastSyncTime(), 987654321);
}

TEST_F(IndexTest, WordLength_TooLong) {
    std::string longWord(130, 'z');
    std::ofstream(dummyFile) << longWord;
    app.indexing.index.indexPath(dummyFile);
    
    app.indexing.index.search(longWord);
    EXPECT_TRUE(app.search.foundPaths.empty());
}

TEST_F(IndexTest, PersistenceAcrossOpen) {
    std::ofstream(dummyFile) << "persistent";
    app.indexing.index.indexPath(dummyFile);
    app.indexing.index.close();
    
    // Reopen
    app.indexing.index.open(dbPath);
    app.indexing.index.search("persistent");
    EXPECT_FALSE(app.search.foundPaths.empty());
}
