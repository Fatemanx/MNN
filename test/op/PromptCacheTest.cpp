//
//  PromptCacheTest.cpp
//  MNN
//
//  Tests for prompt cache utilities.
//

#include <MNN/MNNDefine.h>
#include "MNNTestSuite.h"
#include "prompt_cache_utils.hpp"

using MNN::Transformer::stripThinkBlocks;
using MNN::Transformer::stripLeadingThinkClosers;
using MNN::Transformer::stripLeadingEmptyThinkBlocks;
using MNN::Transformer::stripLeadingReasoningBlocks;
using MNN::Transformer::mayContinueLeadingEmptyThinkBlock;

class PromptCacheStripThinkTest : public MNNTestCase {
public:
    virtual ~PromptCacheStripThinkTest() = default;
    virtual bool run(int precision) {
        // Test 1: basic strip
        {
            std::string text = "Hello <think>reasoning here</think> world";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "Hello  world");
        }
        // Test 2: multiple blocks
        {
            std::string text = "<think>a</think>mid<think>b</think>end";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "midend");
        }
        // Test 3: no tags
        {
            std::string text = "no tags here";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "no tags here");
        }
        // Test 4: trailing newlines after </think>
        {
            std::string text = "before<think>x</think>\n\nafter";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "beforeafter");
        }
        // Test 5: unclosed <think> (no </think>)
        {
            std::string text = "before<think>unclosed";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "before<think>unclosed");
        }
        // Test 6: empty string
        {
            std::string text = "";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "");
        }
        // Test 7: trailing \r\n mix
        {
            std::string text = "a<think>b</think>\r\nc";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "ac");
        }
        // Test 8: MiniCPM5 thought tags
        {
            std::string text = "Hello <|thought_begin|>reasoning here<|thought_end|> world";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "Hello  world");
        }
        // Test 9: multiple MiniCPM5 thought blocks
        {
            std::string text = "<|thought_begin|>a<|thought_end|>mid<|thought_begin|>b<|thought_end|>end";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "midend");
        }
        // Test 10: MiniCPM5 thought tag with trailing newlines
        {
            std::string text = "before<|thought_begin|>x<|thought_end|>\n\nafter";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "beforeafter");
        }
        // Test 11: mixed think styles
        {
            std::string text = "a<think>b</think><|thought_begin|>c<|thought_end|>d";
            stripThinkBlocks(text);
            MNNTEST_ASSERT(text == "ad");
        }
        // Test 12: orphan </think> at assistant prefix
        {
            std::string text = "\n</think>\n\nanswer";
            stripLeadingThinkClosers(text);
            MNNTEST_ASSERT(text == "answer");
        }
        // Test 13: orphan MiniCPM5 thought_end at assistant prefix
        {
            std::string text = "  <|thought_end|>\r\nfinal";
            stripLeadingThinkClosers(text);
            MNNTEST_ASSERT(text == "final");
        }
        // Test 14: keep normal assistant content unchanged
        {
            std::string text = "normal reply";
            stripLeadingThinkClosers(text);
            MNNTEST_ASSERT(text == "normal reply");
        }
        // Test 15: strip leading empty think block only
        {
            std::string text = "<think>\n</think>\nanswer";
            MNNTEST_ASSERT(stripLeadingEmptyThinkBlocks(text));
            MNNTEST_ASSERT(text == "answer");
        }
        // Test 16: strip leading empty MiniCPM5 thought block only
        {
            std::string text = "<|thought_begin|>\r\n<|thought_end|>\r\nanswer";
            MNNTEST_ASSERT(stripLeadingEmptyThinkBlocks(text));
            MNNTEST_ASSERT(text == "answer");
        }
        // Test 17: keep non-empty think block unchanged for streaming cleanup
        {
            std::string text = "<think>reasoning</think>answer";
            MNNTEST_ASSERT(!stripLeadingEmptyThinkBlocks(text));
            MNNTEST_ASSERT(text == "<think>reasoning</think>answer");
        }
        // Test 18: keep inline think tags unchanged for streaming cleanup
        {
            std::string text = "answer <think>literal</think> text";
            MNNTEST_ASSERT(!stripLeadingEmptyThinkBlocks(text));
            MNNTEST_ASSERT(text == "answer <think>literal</think> text");
        }
        // Test 19: prefix reasoning cleanup removes only leading reasoning block
        {
            std::string text = "<|thought_begin|>reasoning<|thought_end|>\nanswer<think>literal</think>";
            stripLeadingReasoningBlocks(text);
            MNNTEST_ASSERT(text == "answer<think>literal</think>");
        }
        // Test 20: partial closer at prefix should keep buffering
        {
            MNNTEST_ASSERT(mayContinueLeadingEmptyThinkBlock("</thi"));
            MNNTEST_ASSERT(mayContinueLeadingEmptyThinkBlock("<|thought_en"));
        }
        // Test 21: partial empty think block should keep buffering
        {
            MNNTEST_ASSERT(mayContinueLeadingEmptyThinkBlock("<think>\n"));
            MNNTEST_ASSERT(mayContinueLeadingEmptyThinkBlock("<|thought_begin|>\n  "));
        }
        // Test 22: normal visible text should stop prefix buffering
        {
            MNNTEST_ASSERT(!mayContinueLeadingEmptyThinkBlock("answer"));
            MNNTEST_ASSERT(!mayContinueLeadingEmptyThinkBlock("answer </think> literal"));
        }
        return true;
    }
};
MNNTestSuiteRegister(PromptCacheStripThinkTest, "op/prompt_cache");
