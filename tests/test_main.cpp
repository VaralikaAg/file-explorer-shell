#include <gtest/gtest.h>
#include "myheader.hpp"

// Global app state definition for tests
AppState app;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
