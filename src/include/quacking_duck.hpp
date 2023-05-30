#pragma once

#include <vector>
#include <string>

#include "chat.hpp"
#include "duckdb/catalog/catalog.hpp"

class QuackingDuck {
public:
    QuackingDuck();
    std::string Ask(std::string question);
    std::string ExplainSchema(std::string detail = "one sentence");
    
    
    std::string AnalyzeQuery(std::string query);

    // You should call Ask or AnalyzeQuery before calling FixupQuery.
    std::string FixupQuery(std::string error_message);
    void StoreSchema(duckdb::SchemaCatalogEntry& schema_entry);

    bool HasSeenQuery(std::string query) {return query == query_;}

private:
    Chat chat_;
    std::vector<std::string> table_ddl_;
    std::string schema_representation_;
    std::string query_;
};
