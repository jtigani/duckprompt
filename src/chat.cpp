#include <iostream>
#include <string>

#include "chat.hpp"
#include "https.hpp"

#include "yyjson.hpp"

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

std::string JsonEncode(std::string unencoded) {
    
}

std::string GenerateMessage(ChatContext context) {
    std::string msg;
    msg.append("{\"role\": \"");
    msg.append(context.role);
    msg.append("\", \"content\":\"");
    msg.append(JsonEncode(context.content));
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

/* Exmaple
{"id":"chatcmpl-7LLgyWTjgbnz6d46npSn2iehhvLDg",
 "object":"chat.completion","created":1685322628,
 "model":"gpt-3.5-turbo-0301","usage":{"prompt_tokens":103,"completion_tokens":94,"total_tokens":197},
 "choices":[{"message":{"role":"assistant",
    "content":"SELECT \n  sum(l_extendedprice * (1 - l_discount)) as revenue \nFROM \n  lineitem \nWHERE \n
          l_shipdate >= date '1994-01-01'\n  AND l_shipdate < date '1994-01-01' + interval '1' year \n 
          AND l_discount between 0.06 - 0.01 AND 0.06 + 0.01 \n  AND l_quantity < 24;"},
    "finish_reason":"stop","index":0}]}
*/

std::string ParseResponse(std::string response) {
    yyjson_doc *doc = yyjson_read(response.c_str(), response.length(), 0);
    std::string content;
    do {
        std::cerr << "parsed the response doc";
        yyjson_val *root_json = yyjson_doc_get_root(doc);
        if (root_json == nullptr) {
            std::cerr << "Unable to read the root of the response";
            break;
        }
        yyjson_val *choices_json = yyjson_obj_get(root_json, "choices");
        if (choices_json == nullptr) {
            std::cerr << "Unable to choices object from the root";
            break;
        }
        size_t idx, max;
        yyjson_val *choice_json;
        
        yyjson_arr_foreach(choices_json, idx, max, choice_json) {
            yyjson_val* message_json = yyjson_obj_get(choice_json, "message");
            if (message_json == nullptr) {
                std::cerr << "Unable to read the message object from the choice";
                break;
            }
            yyjson_val* content_json = yyjson_obj_get(message_json, "content");
            if (message_json == nullptr) {
                std::cerr << "Unable to read the content object from the message";
                break;
            }
            content = yyjson_get_str(content_json); 
            break;
        }
    } while(false);
    yyjson_doc_free(doc);
    return content;
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
    return (response.code == 200) ? ParseResponse(response.response) : "";
}