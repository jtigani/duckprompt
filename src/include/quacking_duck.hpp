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

    void StoreSchema(duckdb::SchemaCatalogEntry& schema_entry);

private:
    Chat chat_;
    std::vector<std::string> table_ddl_;
    std::string schema_representation_;
};
