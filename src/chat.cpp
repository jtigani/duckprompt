#include <iostream>
#include <string>

#include "chat.hpp"
#include "https.hpp"

const char* Chat::c_open_ai_host = "api.openai.com";
const char* Chat::c_chat_uri = "v1/chat/completions";

Chat::Chat(std::string initial_context) {
    Reset(initial_context);
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

void Chat::Reset(std::string initial_context) {
    context_.clear();
    if (initial_context.length() > 0) {
        context_.push_back(ChatContext("system", initial_context));
    }

}
std::string GenerateMessage(ChatContext context) {
    std::string msg;
    msg.append("{\"role\": \"");
    msg.append(context.role);
    msg.append("\", \"content\":\"");
    msg.append(context.content);
    msg.append("\"}");
    return msg;
}

std::string join(const std::vector<std::string>& v, char c) {
   std::string s;
   for (std::vector<std::string>::const_iterator p = v.begin();
        p != v.end(); ++p) {
      s += *p;
      if (p != v.end() - 1)
        s += c;
   }
   return s;
}

std::string Chat::GenerateMessages() {
    std::string inner;

    std::vector<std::string> messages;
    for (auto single_context : context_) {
        messages.push_back(GenerateMessage(single_context));
    }
    return "[" + join(messages, ',') + "]";

}

std::string Chat::SendPrompt(std::string prompt) {
    HTTPS https(c_open_ai_host);    
    std::string question = prompt;
    std::vector<std::pair<std::string, std::string> > headers;
    std::string auth_header = GetAuthorizationHeader();
    if (auth_header.length() > 0) {
        headers.push_back(std::make_pair(std::string("Authorization"), auth_header));
    } else {
        std::cerr << "Missing authorization key for OpenAI";

    }

    std::string whole_prompt = "Output a single SQL query without any explanation and do "
        "not add anything to the query that was not part of the question. "
        "Make sure to only use tables and columns from the schema above and write a query "
        "to answer the following question: '" 
        + prompt 
        + "'";
    context_.push_back(ChatContext("user", whole_prompt));
    std::string body = 
        "{\"model\": \"gpt-3.5-turbo\", \"messages\":" + GenerateMessages() + "}";
    
    HTTPSResponse response = https.Post(c_chat_uri, headers, body);
    if (response.code == 200) {
        // TODO: Parse the JSON of the result and add it to the context.
    }
    return response.response;  
}

