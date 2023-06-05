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

    std::string GenerateMessage();
    std::string role;
    std::string content;
};

class Chat {
public:
    Chat(std::string model) : model_(model.length() > 0 ? model : c_model) { }

    void Reset(std::string context);

    void SetSystemContext(std::string  context);

    std::string SendPrompt(std::string prompt);

private:
    std::string GenerateMessages();
    std::string GenerateRequest();

private:
    static const char* c_open_ai_host;
    static const char* c_chat_uri;
    static const char* c_model;

    std::string model_;
    std::vector<ChatContext> context_;
};
