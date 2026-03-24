#include "myheader.h"
#include <gtest/gtest.h>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        app.config.indexingEnabled = true;
        app.config.workers = 4;
        app.layout.rowSize = 10;
    }
};

TEST_F(ConfigTest, DefaultValues) {
    EXPECT_TRUE(app.config.indexingEnabled);
    EXPECT_EQ(app.config.workers, 4);
}

TEST_F(ConfigTest, ToggleIndexing) {
    app.config.indexingEnabled = false;
    EXPECT_FALSE(app.config.indexingEnabled);
    app.config.indexingEnabled = true;
    EXPECT_TRUE(app.config.indexingEnabled);
}

TEST_F(ConfigTest, UpdateWorkers) {
    app.config.workers = 8;
    EXPECT_EQ(app.config.workers, 8);
    app.config.workers = 4; // Restore
}
