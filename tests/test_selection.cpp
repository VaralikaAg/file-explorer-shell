#include "myheader.h"
#include <gtest/gtest.h>

class SelectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        app.selection.selectedFiles.clear();
        app.selection.clipboard.clear();
    }
};

TEST_F(SelectionTest, EmptySelection) {
    EXPECT_TRUE(app.selection.selectedFiles.empty());
}

TEST_F(SelectionTest, SingleSelect) {
    app.selection.selectedFiles.insert("/path/to/f1");
    EXPECT_EQ(app.selection.selectedFiles.size(), 1);
    EXPECT_TRUE(app.selection.selectedFiles.count("/path/to/f1"));
}

TEST_F(SelectionTest, MultiSelect) {
    app.selection.selectedFiles.insert("/a");
    app.selection.selectedFiles.insert("/b");
    EXPECT_EQ(app.selection.selectedFiles.size(), 2);
}

TEST_F(SelectionTest, ClearSelection) {
    app.selection.selectedFiles.insert("/a");
    app.selection.selectedFiles.clear();
    EXPECT_TRUE(app.selection.selectedFiles.empty());
}

TEST_F(SelectionTest, ClipboardEmpty) {
    EXPECT_TRUE(app.selection.clipboard.empty());
}

TEST_F(SelectionTest, ClipboardUpdate) {
    app.selection.clipboard = {"/a", "/b"};
    EXPECT_EQ(app.selection.clipboard.size(), 2);
}
