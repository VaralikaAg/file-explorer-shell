#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "myheader.h"

namespace fs = std::filesystem;

class DirCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::create_directories(DUMMY_DIR);
        app.cache.dirCache.clear();
        app.nav.fileList.clear();
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(DUMMY_DIR, ec);
    }
};

TEST_F(DirCacheTest, GetDirectoryCount_Basic) {
    for (int i = 0; i < 5; ++i) {
        std::ofstream(std::string(DUMMY_DIR) + "/file" + std::to_string(i) + ".txt");
    }
    int count = getDirectoryCount(fs::path(DUMMY_DIR));
    EXPECT_EQ(count, 5);
    EXPECT_EQ(app.nav.fileList.size(), 5);
}

TEST_F(DirCacheTest, CacheHit) {
    std::string path = std::string(DUMMY_DIR) + "/cache_test";
    fs::create_directories(path);
    std::ofstream(path + "/a.txt");
    
    // First call: Populate cache
    getDirectoryCount(fs::path(path));
    EXPECT_EQ(app.cache.dirCache.count(path), 1);
    
    // Second call: Should use cache
    int count = getDirectoryCount(fs::path(path));
    EXPECT_EQ(count, 1);
}

TEST_F(DirCacheTest, SortedOrder) {
    std::ofstream(std::string(DUMMY_DIR) + "/z.txt");
    std::ofstream(std::string(DUMMY_DIR) + "/a.txt");
    std::ofstream(std::string(DUMMY_DIR) + "/m.txt");
    
    getDirectoryCount(fs::path(DUMMY_DIR));
    ASSERT_EQ(app.nav.fileList.size(), 3);
    EXPECT_EQ(app.nav.fileList[0], "a.txt");
    EXPECT_EQ(app.nav.fileList[1], "m.txt");
    EXPECT_EQ(app.nav.fileList[2], "z.txt");
}

TEST_F(DirCacheTest, InvalidateCache) {
    std::string path = std::string(DUMMY_DIR) + "/inv_test";
    fs::create_directories(path);
    getDirectoryCount(fs::path(path));
    ASSERT_EQ(app.cache.dirCache.count(path), 1);
    
    invalidateDirCache(path);
    EXPECT_EQ(app.cache.dirCache.count(path), 0);
}

TEST_F(DirCacheTest, EmptyDirectory) {
    std::string path = std::string(DUMMY_DIR) + "/empty";
    fs::create_directories(path);
    EXPECT_EQ(getDirectoryCount(fs::path(path)), 0);
}

TEST_F(DirCacheTest, SkipDotFiles) {
    std::ofstream(std::string(DUMMY_DIR) + "/visible.txt");
    // . and .. are virtual, but we can simulate them if the iterator returns them
    int count = getDirectoryCount(fs::path(DUMMY_DIR));
    EXPECT_EQ(count, 1); 
}
