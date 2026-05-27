#ifndef PROMPT_CACHE_UTILS_HPP
#define PROMPT_CACHE_UTILS_HPP

#include <string>

namespace MNN {
namespace Transformer {

inline bool isThinkTagWhitespace(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

inline size_t skipLeadingThinkTagWhitespace(const std::string& text, size_t pos = 0) {
    while (pos < text.size() && isThinkTagWhitespace(text[pos])) {
        pos++;
    }
    return pos;
}

inline bool hasThinkTagAt(const std::string& text, size_t pos, const std::string& tag) {
    return text.compare(pos, tag.size(), tag) == 0;
}

inline bool hasThinkTagPrefixAt(const std::string& text, size_t pos, const std::string& tag) {
    if (pos >= text.size()) {
        return false;
    }
    size_t remain = text.size() - pos;
    return remain <= tag.size() && text.compare(pos, remain, tag, 0, remain) == 0;
}

inline void stripDelimitedBlocks(std::string& text, const std::string& startTag, const std::string& endTag) {
    while (true) {
        auto start = text.find(startTag);
        if (start == std::string::npos)
            break;
        auto end = text.find(endTag, start);
        if (end == std::string::npos)
            break;
        end += endTag.size();
        while (end < text.size() && (text[end] == '\n' || text[end] == '\r'))
            end++;
        text.erase(start, end - start);
    }
}

// Strip <think>...</think> blocks from cached prompt text.
// When enable_thinking is active, templates may append reasoning blocks to the
// last assistant message only. Since that message won't be the last one on the
// next turn, stripping keeps the cached text aligned with the next rendering.
inline void stripThinkBlocks(std::string& text) {
    stripDelimitedBlocks(text, "<think>", "</think>");
    stripDelimitedBlocks(text, "<|thought_begin|>", "<|thought_end|>");
}

inline void stripLeadingThinkClosers(std::string& text) {
    static const char* kClosingTags[] = {"</think>", "<|thought_end|>"};
    while (true) {
        size_t pos = skipLeadingThinkTagWhitespace(text);
        bool stripped = false;
        for (const auto* tag : kClosingTags) {
            std::string closingTag(tag);
            if (hasThinkTagAt(text, pos, closingTag)) {
                pos += closingTag.size();
                pos = skipLeadingThinkTagWhitespace(text, pos);
                text.erase(0, pos);
                stripped = true;
                break;
            }
        }
        if (!stripped) {
            break;
        }
    }
}

inline void stripThinkCloserTokens(std::string& text) {
    static const char* kClosingTags[] = {"</think>", "<|thought_end|>"};
    for (const auto* tag : kClosingTags) {
        std::string closingTag(tag);
        size_t pos = 0;
        while ((pos = text.find(closingTag, pos)) != std::string::npos) {
            text.erase(pos, closingTag.size());
        }
    }
}

inline size_t trailingThinkCloserPrefixLength(const std::string& text) {
    static const char* kClosingTags[] = {"</think>", "<|thought_end|>"};
    size_t maxPrefix = 0;
    for (const auto* tag : kClosingTags) {
        std::string closingTag(tag);
        size_t upper = std::min(text.size(), closingTag.size() - 1);
        for (size_t len = 1; len <= upper; len++) {
            if (text.compare(text.size() - len, len, closingTag, 0, len) == 0) {
                maxPrefix = std::max(maxPrefix, len);
            }
        }
    }
    return maxPrefix;
}

inline bool stripLeadingEmptyThinkBlocks(std::string& text) {
    struct ThinkTagPair {
        const char* start;
        const char* end;
    };
    static const ThinkTagPair kThinkPairs[] = {
        {"<think>", "</think>"},
        {"<|thought_begin|>", "<|thought_end|>"},
    };
    bool strippedAny = false;
    while (true) {
        size_t pos = skipLeadingThinkTagWhitespace(text);
        bool stripped = false;
        for (const auto& pair : kThinkPairs) {
            std::string startTag(pair.start);
            std::string endTag(pair.end);
            if (!hasThinkTagAt(text, pos, startTag)) {
                continue;
            }
            size_t endPos = skipLeadingThinkTagWhitespace(text, pos + startTag.size());
            if (!hasThinkTagAt(text, endPos, endTag)) {
                return strippedAny;
            }
            endPos = skipLeadingThinkTagWhitespace(text, endPos + endTag.size());
            text.erase(0, endPos);
            stripped = true;
            strippedAny = true;
            break;
        }
        if (!stripped) {
            return strippedAny;
        }
    }
}

inline void stripLeadingReasoningBlocks(std::string& text) {
    struct ThinkTagPair {
        const char* start;
        const char* end;
    };
    static const ThinkTagPair kThinkPairs[] = {
        {"<think>", "</think>"},
        {"<|thought_begin|>", "<|thought_end|>"},
    };
    while (true) {
        size_t pos = skipLeadingThinkTagWhitespace(text);
        bool stripped = false;
        for (const auto& pair : kThinkPairs) {
            std::string startTag(pair.start);
            std::string endTag(pair.end);
            if (!hasThinkTagAt(text, pos, startTag)) {
                continue;
            }
            size_t endPos = text.find(endTag, pos + startTag.size());
            if (endPos == std::string::npos) {
                return;
            }
            endPos = skipLeadingThinkTagWhitespace(text, endPos + endTag.size());
            text.erase(0, endPos);
            stripped = true;
            break;
        }
        if (!stripped) {
            stripLeadingThinkClosers(text);
            return;
        }
        stripLeadingThinkClosers(text);
    }
}

inline bool mayContinueLeadingEmptyThinkBlock(const std::string& text) {
    struct ThinkTagPair {
        const char* start;
        const char* end;
    };
    static const char* kClosingTags[] = {"</think>", "<|thought_end|>"};
    static const ThinkTagPair kThinkPairs[] = {
        {"<think>", "</think>"},
        {"<|thought_begin|>", "<|thought_end|>"},
    };
    size_t pos = skipLeadingThinkTagWhitespace(text);
    if (pos >= text.size()) {
        return true;
    }
    for (const auto* tag : kClosingTags) {
        std::string closingTag(tag);
        if (hasThinkTagPrefixAt(text, pos, closingTag)) {
            return true;
        }
    }
    for (const auto& pair : kThinkPairs) {
        std::string startTag(pair.start);
        std::string endTag(pair.end);
        if (hasThinkTagPrefixAt(text, pos, startTag)) {
            return true;
        }
        if (!hasThinkTagAt(text, pos, startTag)) {
            continue;
        }
        size_t endPos = skipLeadingThinkTagWhitespace(text, pos + startTag.size());
        if (endPos >= text.size()) {
            return true;
        }
        if (hasThinkTagPrefixAt(text, endPos, endTag)) {
            return true;
        }
    }
    return false;
}

}  // namespace Transformer
}  // namespace MNN

#endif  // PROMPT_CACHE_UTILS_HPP
