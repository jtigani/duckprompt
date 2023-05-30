#include <iostream>
#include <vector>
#include <string>

#include "quacking_duck.hpp"
#include "chat.hpp"
#include "yyjson.hpp"

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/catalog/catalog_entry.hpp"
#include "duckdb/catalog/catalog_entry/schema_catalog_entry.hpp"
#include "duckdb/catalog/catalog_entry/table_catalog_entry.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/planner/binder.hpp"

QuackingDuck::QuackingDuck() : chat_("") {}

std::string QuackingDuck::Ask(std::string question) {
    if (schema_representation_.length() > 0) {
        ExplainSchema();
    }
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

std::string QuackingDuck::ExplainSchema(std::string detail) {
    chat_.Reset("You are a helpful assistant that can generate an human redable summary of database content based on the schema.");

    std::string whole_prompt = "SQL schema of my database:\n" 
        + schema_representation_
        + "\nExplain in " + detail + " what the data is about";
    return chat_.SendPrompt(whole_prompt);
}

void QuackingDuck::StoreSchema(duckdb::SchemaCatalogEntry& schema_entry) {
    std::function<void(duckdb::CatalogEntry*)> callback = [this](duckdb::CatalogEntry* entry) {
        auto &table = (duckdb::TableCatalogEntry &)*entry;
        table_ddl_.push_back(table.ToSQL());
    };
    schema_entry.Scan(duckdb::CatalogType::TABLE_ENTRY, callback);

    std::string schema = "";
    for (std::string table_ddl : table_ddl_) {
        schema.append(table_ddl);
        schema.append("\n");
    }

    schema_representation_ = schema;
}

std::string QuackingDuck::AnalyzeQuery(std::string query) {
    if (schema_representation_.length() > 0) {
        ExplainSchema();
    }
    chat_.SetSystemContext("You are a helpful assistant that is an expert in SQL code who can a human readable summary of a SQL query.");
    std::string whole_prompt = "Here is my SQL query:\n"
        + query
        + "\nExplain in one sentance what this query will do.";
    std::string result = chat_.SendPrompt(whole_prompt);
    query_ = query;
    return result;
}

std::string QuackingDuck::FixupQuery(std::string error_message) {
    if (query_.length() == 0) {
        // Should have called Ask or AnalyzeQuery to set the query.
        return "";
    }
    
    chat_.SetSystemContext("You are a helpful assistant that can generate Postgresql code based on "
        "the user input. You do not respond with any human readable text, only SQL code.");    

    std::string whole_prompt = "I got the following error: "
        + error_message
        + "Please correct the query and only print SQL code, without an apology or describing the query.";
    query_ = chat_.SendPrompt(whole_prompt);
    return query_;
}
