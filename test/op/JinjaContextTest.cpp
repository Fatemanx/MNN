#include <MNN/MNNDefine.h>
#include "MNNTestSuite.h"
#include "tokenizer/jinja.hpp"

class JinjaContextTest : public MNNTestCase {
public:
    virtual ~JinjaContextTest() = default;
    virtual bool run(int precision) {
        const std::string thoughtTpl =
            "{% if enable_thinking is not defined %}{% set enable_thinking = true %}{% endif %}"
            "{% for message in messages %}"
            "{% if message.role == \"user\" %}<|user|>{{ message.content }}{% endif %}"
            "{% if message.role == \"assistant\" %}"
            "<|assistant|>"
            "{% if message.reasoning_content is defined and message.reasoning_content %}"
            "<|thought_begin|>{{ message.reasoning_content }}<|thought_end|>"
            "{% endif %}"
            "{{ message.content }}"
            "{% endif %}"
            "{% endfor %}"
            "{% if add_generation_prompt %}<|assistant|>{% if enable_thinking %}<|thought_begin|>{% endif %}{% endif %}";

        jinja::Template thoughtTemplate(thoughtTpl, jinja::json::object());

        jinja::json messages = jinja::json::array();
        jinja::json user = jinja::json::object();
        user["role"] = "user";
        user["content"] = "Explain";
        messages.push_back(user);
        jinja::json assistant = jinja::json::object();
        assistant["role"] = "assistant";
        assistant["content"] = "Answer";
        assistant["reasoning_content"] = "Reason";
        messages.push_back(assistant);

        {
            auto rendered = thoughtTemplate.apply_chat_template(messages, true, jinja::json::array(), jinja::json::object());
            MNNTEST_ASSERT(rendered.find("<|user|>Explain") != std::string::npos);
            MNNTEST_ASSERT(rendered.find("<|thought_begin|>Reason<|thought_end|>") != std::string::npos);
            MNNTEST_ASSERT(rendered.rfind("<|assistant|><|thought_begin|>") != std::string::npos);
        }
        {
            jinja::json extra = jinja::json::object();
            extra["enable_thinking"] = false;
            auto rendered = thoughtTemplate.apply_chat_template(messages, true, jinja::json::array(), extra);
            MNNTEST_ASSERT(rendered.find("<|user|>Explain") != std::string::npos);
            MNNTEST_ASSERT(rendered.find("<|thought_begin|>Reason<|thought_end|>") != std::string::npos);
            MNNTEST_ASSERT(rendered.size() >= std::string("<|assistant|>").size());
            MNNTEST_ASSERT(rendered.substr(rendered.size() - std::string("<|assistant|>").size()) == "<|assistant|>");
        }

        const std::string thinkTpl =
            "{% if enable_thinking is not defined %}{% set enable_thinking = true %}{% endif %}"
            "{% for message in messages %}<|{{ message.role }}|>{{ message.content }}{% endfor %}"
            "{% if add_generation_prompt %}<|assistant|>{% if enable_thinking %}<think>\n\n</think>\n\n{% endif %}{% endif %}";
        jinja::Template thinkTemplate(thinkTpl, jinja::json::object());

        {
            auto rendered = thinkTemplate.apply_chat_template(messages, true, jinja::json::array(), jinja::json::object());
            MNNTEST_ASSERT(rendered.find("<|user|>Explain") != std::string::npos);
            MNNTEST_ASSERT(rendered.rfind("<|assistant|><think>\n\n</think>\n\n") != std::string::npos);
        }
        {
            jinja::json extra = jinja::json::object();
            extra["enable_thinking"] = false;
            auto rendered = thinkTemplate.apply_chat_template(messages, true, jinja::json::array(), extra);
            MNNTEST_ASSERT(rendered.find("<|assistant|><think>\n\n</think>\n\n") == std::string::npos);
            MNNTEST_ASSERT(rendered.size() >= std::string("<|assistant|>").size());
            MNNTEST_ASSERT(rendered.substr(rendered.size() - std::string("<|assistant|>").size()) == "<|assistant|>");
        }
        return true;
    }
};

MNNTestSuiteRegister(JinjaContextTest, "op/jinja_context");
