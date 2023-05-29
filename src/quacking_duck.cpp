#include <iostream>
#include <vector>
#include <string>

#include "quacking_duck.hpp"
#include "chat.hpp"
#include "yyjson.hpp"

QuackingDuck::QuackingDuck() : chat_("") {}

std::string QuackingDuck::Ask(std::string question) {
    chat_.Reset("You are a helpful assistant that can generate Postgresql code based on the user input. You do not respond with any human readable text, only SQL code.");    
    std::string json_result = chat_.SendPrompt(question);

    return json_result;
}

std::string QuackingDuck::ExplainSchema(std::string detail) {
    chat_.Reset("You are a helpful assistant that can generate an human redable summary of database content based on the schema.");
    return chat_.SendPrompt("Get Schema");
}

