#include <gtest/gtest.h>
#include "myheader.h"

// --- normalizeWord Tests ---
TEST(TextUtilsTest, NormalizeWord_Basic) {
    EXPECT_EQ(normalizeWord("Hello"), "hello");
    EXPECT_EQ(normalizeWord("the"), "");      // Stopword
    EXPECT_EQ(normalizeWord("user@host"), "user@host"); // @ is allowed
    EXPECT_EQ(normalizeWord("tag#1"), "tag#1");       // # is allowed
    EXPECT_EQ(normalizeWord("env_var"), "env_var");   // _ is allowed
    EXPECT_EQ(normalizeWord("hi-world"), "hi-world"); // - is allowed
    EXPECT_EQ(normalizeWord("$price"), "$price");     // $ is allowed
    EXPECT_EQ(normalizeWord("r&d"), "r&d");           // & is allowed
}

TEST(TextUtilsTest, NormalizeWord_InvalidChars) {
    EXPECT_EQ(normalizeWord("hi{there}"), ""); // { } invalidates whole word
    EXPECT_EQ(normalizeWord("path/file"), ""); // / invalidates whole word
    EXPECT_EQ(normalizeWord("file.cpp"), "");  // . is not whitelisted
}

// --- Normalizer ---

TEST(TextUtilsTest, Normalize_Alpha) { EXPECT_EQ(normalizeWord("apple"), "apple"); }
TEST(TextUtilsTest, Normalize_Numeric) { EXPECT_EQ(normalizeWord("123"), "123"); }
TEST(TextUtilsTest, Normalize_Mixed) { EXPECT_EQ(normalizeWord("a1b2"), "a1b2"); }

TEST(TextUtilsTest, Normalize_At) { EXPECT_EQ(normalizeWord("a@b"), "a@b"); }
TEST(TextUtilsTest, Normalize_Hash) { EXPECT_EQ(normalizeWord("#tag"), "#tag"); }
TEST(TextUtilsTest, Normalize_Underscore) { EXPECT_EQ(normalizeWord("v_n"), "v_n"); }
TEST(TextUtilsTest, Normalize_Dash) { EXPECT_EQ(normalizeWord("a-b"), "a-b"); }
TEST(TextUtilsTest, Normalize_Dollar) { EXPECT_EQ(normalizeWord("$c"), "$c"); }
TEST(TextUtilsTest, Normalize_Amp) { EXPECT_EQ(normalizeWord("a&b"), "a&b"); }

TEST(TextUtilsTest, Normalize_Invalid_Bang) { EXPECT_EQ(normalizeWord("a!"), ""); }
TEST(TextUtilsTest, Normalize_Invalid_Ques) { EXPECT_EQ(normalizeWord("a?"), ""); }
TEST(TextUtilsTest, Normalize_Invalid_Star) { EXPECT_EQ(normalizeWord("a*"), ""); }

TEST(TextUtilsTest, Normalize_Empty) { EXPECT_EQ(normalizeWord(""), ""); }
TEST(TextUtilsTest, Normalize_Space) { EXPECT_EQ(normalizeWord(" "), ""); }
TEST(TextUtilsTest, Normalize_Long_127) { std::string w(127, 'a'); EXPECT_EQ(normalizeWord(w), w); }
TEST(TextUtilsTest, Normalize_Long_128) { std::string w(128, 'a'); EXPECT_EQ(normalizeWord(w), ""); }

TEST(TextUtilsTest, Normalize_Upper) { EXPECT_EQ(normalizeWord("APPLE"), "apple"); }
TEST(TextUtilsTest, Normalize_MixedCase) { EXPECT_EQ(normalizeWord("MiXeD"), "mixed"); }

TEST(TextUtilsTest, Stopword_The) { EXPECT_EQ(normalizeWord("the"), ""); }
TEST(TextUtilsTest, Stopword_And) { EXPECT_EQ(normalizeWord("and"), ""); }
TEST(TextUtilsTest, Stopword_While) { EXPECT_EQ(normalizeWord("while"), ""); }
TEST(TextUtilsTest, Stopword_Between) { EXPECT_EQ(normalizeWord("between"), ""); }

// --- Formatter ---

TEST(FormatUtilsTest, Size_0) { EXPECT_EQ(humanReadableSize(0), "0 B"); }
TEST(FormatUtilsTest, Size_B) { EXPECT_EQ(humanReadableSize(1023), "1023 B"); }
TEST(FormatUtilsTest, Size_KB) { EXPECT_EQ(humanReadableSize(1024), "1.0 KB"); }
TEST(FormatUtilsTest, Size_MB) { EXPECT_EQ(humanReadableSize(1024*1024), "1.0 MB"); }
TEST(FormatUtilsTest, Size_GB) { EXPECT_EQ(humanReadableSize(1024LL*1024*1024), "1.0 GB"); }
TEST(FormatUtilsTest, Size_TB) { EXPECT_EQ(humanReadableSize(1024LL*1024*1024*1024), "1.0 TB"); }
TEST(FormatUtilsTest, Size_Precision) { EXPECT_EQ(humanReadableSize(1536), "1.5 KB"); }

TEST(FormatUtilsTest, Truncate_Long) { EXPECT_EQ(truncateStr("hello world", 5), "he..."); }
TEST(FormatUtilsTest, Truncate_Exact) { EXPECT_EQ(truncateStr("hello", 5), "hello"); }
TEST(FormatUtilsTest, Truncate_Short) { EXPECT_EQ(truncateStr("abc", 2), "..."); }
TEST(FormatUtilsTest, Truncate_VeryShort) { EXPECT_EQ(truncateStr("a", 1), "a"); }
TEST(FormatUtilsTest, Truncate_Empty) { EXPECT_EQ(truncateStr("", 5), ""); }
TEST(FormatUtilsTest, Truncate_Padding_None) { EXPECT_EQ(truncateStr("hi", 10), "hi"); }
