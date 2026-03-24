#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "myheader.h"

namespace fs = std::filesystem;

class SearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::create_directories(std::string(DUMMY_DIR) + "/subdir/hidden_dir");
        std::ofstream(std::string(DUMMY_DIR) + "/alpha.txt").close();
        std::ofstream(std::string(DUMMY_DIR) + "/beta.cpp").close();
        std::ofstream(std::string(DUMMY_DIR) + "/subdir/alpha_copy.txt").close();
        std::ofstream(std::string(DUMMY_DIR) + "/subdir/hidden_dir/deep.txt").close();
        app.search.foundPaths.clear();
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(DUMMY_DIR, ec);
    }
};

TEST_F(SearchTest, FindByExactName) {
    app.search.foundPaths.clear();
    searchAnything(fs::path(DUMMY_DIR), "alpha.txt", true, true);
    ASSERT_EQ(app.search.foundPaths.size(), 1);
    EXPECT_NE(app.search.foundPaths[0].find("alpha.txt"), std::string::npos);
}

TEST_F(SearchTest, FindByPartialName) {
    app.search.foundPaths.clear();
    searchAnything(fs::path(DUMMY_DIR), "alp", true, true);
    // Should find alpha.txt and alpha_copy.txt
    EXPECT_EQ(app.search.foundPaths.size(), 2);
    
    bool foundAlpha = false;
    for (const auto& p : app.search.foundPaths) {
        if (p.find("alpha.txt") != std::string::npos) foundAlpha = true;
    }
    EXPECT_TRUE(foundAlpha);
}

TEST_F(SearchTest, CaseInsensitiveSearch) {
    // Note: searchAnything converts name to lowercase (search_engine.cpp:11)
    searchAnything(fs::path(DUMMY_DIR), "BETA", true, true);
    EXPECT_EQ(app.search.foundPaths.size(), 1);
    EXPECT_NE(app.search.foundPaths[0].find("beta.cpp"), std::string::npos);
}

TEST_F(SearchTest, DeepNestedSearch) {
    searchAnything(fs::path(DUMMY_DIR), "deep", true, true);
    ASSERT_EQ(app.search.foundPaths.size(), 1);
    EXPECT_NE(app.search.foundPaths[0].find("deep.txt"), std::string::npos);
}

TEST_F(SearchTest, FilesOnlySearch) {
    searchAnything(fs::path(DUMMY_DIR), "subdir", true, false);
    // Should not find the directory "subdir" as a search result
    EXPECT_EQ(app.search.foundPaths.size(), 0);
}

TEST_F(SearchTest, NoMatches) {
    searchAnything(fs::path(DUMMY_DIR), "nonexistent", true, true);
    EXPECT_EQ(app.search.foundPaths.size(), 0);
}
