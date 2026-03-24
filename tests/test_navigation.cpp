#include <gtest/gtest.h>
#include <algorithm>
#include "myheader.h"

// --- normalizeRange Tests ---
TEST(NavigationTest, NormalizeRange_EmptyDir) {
    int up, down, x;
    normalizeRange(0, 10, up, down, x);
    EXPECT_EQ(up, 0);
    EXPECT_EQ(down, 0);
    EXPECT_EQ(x, 1);
}

TEST(NavigationTest, NormalizeRange_FewerThanRowSize) {
    int up, down, x;
    up = 0; x = 3;
    normalizeRange(5, 10, up, down, x);
    EXPECT_EQ(up, 0);
    EXPECT_EQ(down, 0);
    EXPECT_EQ(x, 3);
}

TEST(NavigationTest, NormalizeRange_ExactlyRowSize) {
    int up, down, x;
    up = 0; x = 1;
    normalizeRange(10, 10, up, down, x);
    EXPECT_EQ(up, 0);
    EXPECT_EQ(x, 1);
    EXPECT_EQ(down, 0);
}

TEST(NavigationTest, NormalizeRange_ScrolledDown) {
    int up, down, x;
    up = 5; x = 10;
    normalizeRange(20, 10, up, down, x);
    EXPECT_EQ(up, 5);      
    EXPECT_EQ(x, 10);      
    EXPECT_EQ(down, 5);    
}

TEST(NavigationTest, NormalizeRange_UpClamped) {
    int up, down, x;
    up = -5; x = 1;
    normalizeRange(10, 10, up, down, x);
    EXPECT_EQ(up, 0);      
    EXPECT_EQ(down, 0);
}

TEST(NavigationTest, NormalizeRange_UpOverflow) {
    int up, down, x;
    up = 15; x = 1;
    normalizeRange(10, 10, up, down, x);
    EXPECT_EQ(up, 0);      
    EXPECT_EQ(x, 1);
}

TEST(NavigationTest, NormalizeRange_VariousTotals) {
    int up, down, x;
    // Total < RowSize
    normalizeRange(5, 10, up=0, down, x=1);
    EXPECT_EQ(up, 0);
    EXPECT_EQ(down, 0);
    
    // Total > RowSize
    normalizeRange(20, 10, up=15, down, x=1);
    EXPECT_EQ(up, 10); // Clamped to 20-10=10
    EXPECT_EQ(down, 0);
}

TEST(NavigationTest, NormalizeRange_XBoundary) {
    int up, down, x;
    // X too high
    up = 0; x = 50;
    normalizeRange(20, 10, up, down, x);
    EXPECT_EQ(x, 10); // Clamped to rowSize
    
    // X too low
    x = -5;
    normalizeRange(20, 10, up, down, x);
    EXPECT_EQ(x, 1);
}

TEST(NavigationTest, ScrollToIndex_PageJumps) {
    int up, down, x;
    // Jump to middle
    scrollToIndex(15, 50, 10, up, down, x);
    EXPECT_EQ(up, 6); // 15-10+1
    EXPECT_EQ(x, 10);
    
    // Jump to very end
    scrollToIndex(49, 50, 10, up, down, x);
    EXPECT_EQ(up, 40); // 49-10+1
    EXPECT_EQ(x, 10);
}

TEST(NavigationTest, IsUnderCurrentDir_EdgeCases) {
    app.nav.currPath = "/a/b";
    EXPECT_TRUE(isUnderCurrentDir("/a/b/c"));
    EXPECT_TRUE(isUnderCurrentDir("/a/b/file.txt"));
    EXPECT_FALSE(isUnderCurrentDir("/a/c"));
    EXPECT_FALSE(isUnderCurrentDir("/a/bc")); // Prefix match but not directory match
}

TEST(NavigationTest, NormalizeRange_EmptyList) {
    int up=5, down=5, x=5;
    normalizeRange(0, 10, up, down, x);
    EXPECT_EQ(up, 0);
    EXPECT_EQ(down, 0);
    EXPECT_EQ(x, 1);
}

TEST(NavigationTest, ScrollToIndex_RapidSuccession) {
    int up=0, down=0, x=1;
    for(int i=0; i<30; ++i) {
        scrollToIndex(i, 100, 10, up, down, x);
    }
    // Final state of up should be 29-10+1 = 20
    EXPECT_EQ(up, 20);
    EXPECT_EQ(x, 10);
}

// --- isUnderCurrentDir Tests ---
TEST(NavigationTest, IsUnderCurrentDir) {
    app.nav.currPath = "/home/user/projects";
    EXPECT_TRUE(isUnderCurrentDir("/home/user/projects/foo.txt"));
    EXPECT_FALSE(isUnderCurrentDir("/home/user/other/foo.txt"));
}
