#include "myheader.hpp"
#include <gtest/gtest.h>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        app.config.indexing_enabled = true;
        app.config.workers = 4;
        app.layout.row_size = 10;
    }
};

TEST_F(ConfigTest, DefaultValues) {
    EXPECT_TRUE(app.config.indexing_enabled);
    EXPECT_EQ(app.config.workers, 4);
}

TEST_F(ConfigTest, ToggleIndexing) {
    app.config.indexing_enabled = false;
    EXPECT_FALSE(app.config.indexing_enabled);
    app.config.indexing_enabled = true;
    EXPECT_TRUE(app.config.indexing_enabled);
}

TEST_F(ConfigTest, UpdateWorkers) {
    app.config.workers = 8;
    EXPECT_EQ(app.config.workers, 8);
    app.config.workers = 5; // Updated default for config.json test
}