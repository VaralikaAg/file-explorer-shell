#include "myheader.hpp"
#include <gtest/gtest.h>

class SelectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        app.selection.selected_files.clear();
        app.selection.clipboard.clear();
    }
};

TEST_F(SelectionTest, EmptySelection) {
    EXPECT_TRUE(app.selection.selected_files.empty());
}

TEST_F(SelectionTest, SingleSelect) {
    app.selection.selected_files.insert("/path/to/f1");
    EXPECT_EQ(app.selection.selected_files.size(), 1);
    EXPECT_TRUE(app.selection.selected_files.count("/path/to/f1"));
}

TEST_F(SelectionTest, MultiSelect) {
    app.selection.selected_files.insert("/a");
    app.selection.selected_files.insert("/b");
    EXPECT_EQ(app.selection.selected_files.size(), 2);
}

TEST_F(SelectionTest, ClearSelection) {
    app.selection.selected_files.insert("/a");
    app.selection.selected_files.clear();
    EXPECT_TRUE(app.selection.selected_files.empty());
}

TEST_F(SelectionTest, ClipboardEmpty) {
    EXPECT_TRUE(app.selection.clipboard.empty());
}

TEST_F(SelectionTest, ClipboardUpdate) {
    app.selection.clipboard = {"/a", "/b"};
    EXPECT_EQ(app.selection.clipboard.size(), 2);
}
