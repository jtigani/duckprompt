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

void QuackingDuck::StoreSchema(duckdb::ClientContext& context) {
    auto &catalog = duckdb::Catalog::GetCatalog(context, INVALID_CATALOG);
    auto callback = [&](
            duckdb::SchemaCatalogEntry& schema_entry) {
        StoreSchema(schema_entry);
    };
    catalog.ScanSchemas(context, callback);
}

void QuackingDuck::StoreSchema(duckdb::SchemaCatalogEntry& schema_entry) {
    auto callback = [this](duckdb::CatalogEntry& entry) {
        auto &table = (duckdb::TableCatalogEntry &)entry;
        std::string name = table.name;
        std::string sql = table.ToSQL();
        if (sql.substr(0, 6) == "SELECT") {
            // this is a system view that for some reason shows up
            // as a table.
            return;
        }
        table_ddl_.push_back(sql);
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

std::string QuackingDuck::FixupQuery(std::string error_message) {
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
