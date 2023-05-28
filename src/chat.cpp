#include <iostream>
#include <string>

#include "chat.hpp"
#include "https.hpp"

const char* Chat::c_open_ai_host = "api.openai.com";
const char* Chat::c_chat_uri = "v1/chat/completions";

Chat::Chat(std::string initial_context) {
    if (initial_context.length() > 0) {
        context_.push_back(ChatContext("system", initial_context));
    }
}

Chat::~Chat() {}

std::string GetAuthorizationHeader() {
    char * key = std::getenv("OPENAI_API_KEY");
    if (key == nullptr) {
        return "";
    } else {
        return "Bearer " + std::string(key);
    }
}

std::string Chat::SendPrompt(std::string prompt) {
    HTTPS https(c_open_ai_host);    
    std::string question = prompt;
    std::vector<std::pair<std::string, std::string> > headers;
    std::string auth_header = GetAuthorizationHeader();
    if (auth_header.length() > 0) {
        headers.push_back(std::make_pair(std::string("Authorization"), auth_header));
    } else {
        std::cerr << "Missing information for OpenAI";
    }
    std::string body = 
        "{\"model\": \"gpt-3.5-turbo\", \"messages\": [{\"role\": \"user\", \"content\":\"" + prompt + "\"}]}";
    HTTPSResponse response = https.Post(c_chat_uri, headers, body);
    return response.response;
}

