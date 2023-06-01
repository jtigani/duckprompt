#include <iostream>
#include <string>
#include <vector>

#include "quacking_duck.hpp"
#include "chat.hpp"
#include "yyjson.hpp"


std::string QuackingDuck::ExplainSchema() {
    ExtractedSchema extracted_schema;
    db_.ExtractSchema(extracted_schema);
    return ExplainSchemaPrompt(extracted_schema);
}

std::string QuackingDuck::Ask(std::string prompt) {
    ExplainSchema();
    std::string query = AskPrompt(prompt);
    std::string fixed = FixupQuery(query);
    return (query == fixed)? query : AskPrompt(prompt);
}

std::string QuackingDuck::FixupQuery(std::string query) {
    std::string error = db_.ValidateParse(query);
    if (error.size() > 0) {
        AnalyzeQueryPrompt(query);
        return FixupQueryPrompt(error);
    }
    ExplainSchema();
    error = db_.ValidateSemantics(query);
    if (error.size() > 0) {
        AnalyzeQueryPrompt(query);
        return FixupQueryPrompt(error);
    }
    return query;
}

std::string QuackingDuck::AskPrompt(std::string question) {
    chat_.SetSystemContext("You are a helpful assistant that can generate Postgresql code based on "
        "the user input. You do not respond with any human readable text, only SQL code.");    
    std::string whole_prompt = "Output a single SQL query without any explanation and do "
        "not add anything to the query that was not part of the question. "
        "Make sure to only use tables and columns from the schema above and write a query "
        "to answer the following question: '" 
        + question 
        + "'";
    
    query_ = chat_.SendPrompt(whole_prompt);
    return query_;
}

std::string QuackingDuck::ExplainSchemaPrompt(const ExtractedSchema& extracted_schema) {
    if (schema_summary_.length() > 0) {
        return schema_summary_;
    }
    chat_.Reset("You are a helpful assistant that can generate an human redable summary "
            " of database content based on the schema.");

    std::string whole_prompt = "SQL schema of my database:\n" 
        + extracted_schema.SchemaToString()
        + "\nExplain in one sentence what the data is about";
    schema_summary_ = chat_.SendPrompt(whole_prompt);
    return schema_summary_;
}

std::string ExtractedSchema::SchemaToString() const {
    std::string schema = "";
    for (std::string current_table_ddl : table_ddl) {
        schema.append(current_table_ddl);
        schema.append("\n");
    }

    return schema;
}

std::string QuackingDuck::AnalyzeQueryPrompt(std::string query) {
    if (query_.length() > 0) {
        return query_;
    }
    chat_.SetSystemContext("You are a helpful assistant that is an expert in SQL code who can output "
           "a human readable summary of a SQL query.");
    std::string whole_prompt = "Here is my SQL query:\n"
        + query
        + "\nPlease respond with the SQL query and an explaiation about what the query will do.";
    std::string result = chat_.SendPrompt(whole_prompt);
    query_ = query;
    return result;
}

std::string ExtractSelect(std::string message) {
    size_t pos = message.find(" SELECT");
    if (pos == std::string::npos ) {
        pos = message.find("\nSELECT");
    }

    std::string result = (pos == std::string::npos) 
        ? message
        : message.substr(pos + 1); // go beyond the ' '.

    pos = result.find(";\n");
    result = (pos == std::string::npos)
        ? result
        : result.substr(0, pos);

    return result; 
}

std::string QuackingDuck::FixupQueryPrompt(std::string error_message) {
    if (query_.length() == 0) {
        // Should have called Ask or AnalyzeQuery to set the query.
        return "";
    }
    
    chat_.SetSystemContext("You are a helpful assistant that can generate Postgresql code based on "
        "the user input. You do not respond with any human readable text, only SQL code.");    

    std::string whole_prompt = "When I ran the previous query, I got the following exception:\n"
        + error_message
        + "\nPlease correct and output only the resulting SQL code.";
    
    std::string response = chat_.SendPrompt(whole_prompt);
    // This is a hack .... if the select statement is at the end, return it.
    query_ = ExtractSelect(response);
    return query_;
}
