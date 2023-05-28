#pragma once

#include <string>
#include <vector>


class ChatContext {
    public:
    ChatContext() : role(""), content("") {};
    ChatContext(std::string r, std::string c) : role(r), content(c) {};
    ChatContext(const ChatContext& other) : role(other.role), content(other.content) {};
    ChatContext& operator=(const ChatContext& other) {
        if (this != &other) {
            this->role = other.role;
            this->content = other.content;
        }
        return *this;
    }
    std::string role;
    std::string content;
};

class Chat {
public:
    Chat(std::string context);
    ~Chat();

    std::string SendPrompt(std::string prompt);

private:
    static const char* c_open_ai_host;
    static const char* c_chat_uri;

    std::vector<ChatContext> context_;
};
