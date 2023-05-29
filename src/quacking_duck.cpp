#include <iostream>
#include <vector>
#include <string>

#include "quacking_duck.hpp"
#include "chat.hpp"
#include "yyjson.hpp"

QuackingDuck::QuackingDuck() : chat_("") {}

std::string QuackingDuck::Ask(std::string question) {
    chat_.Reset("You are a helpful assistant that can generate Postgresql code based on the user input. You do not respond with any human readable text, only SQL code.");    
    std::string whole_prompt = "Output a single SQL query without any explanation and do "
        "not add anything to the query that was not part of the question. "
        "Make sure to only use tables and columns from the schema above and write a query "
        "to answer the following question: '" 
        + question 
        + "'";
    return chat_.SendPrompt(whole_prompt);
}

std::string QuackingDuck::ExplainSchema(std::string detail) {
    chat_.Reset("You are a helpful assistant that can generate an human redable summary of database content based on the schema.");
    return chat_.SendPrompt("Get Schema");
}

