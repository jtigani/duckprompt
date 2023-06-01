#define DUCKDB_EXTENSION_MAIN

#include <string>
#include <vector>

#include "duckprompt_extension.hpp"
#include "chat.hpp"
#include "quacking_duck.hpp"
#include "duckdb.hpp"
#include "duckdb/catalog/catalog.hpp"
#include "duckdb/catalog/catalog_entry.hpp"
#include "duckdb/catalog/catalog_entry/schema_catalog_entry.hpp"
#include "duckdb/catalog/catalog_entry/table_catalog_entry.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/function_binder.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/parser/parsed_data/create_pragma_function_info.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "duckdb/parser/parsed_data/create_view_info.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/parser/statement/select_statement.hpp"
#include "duckdb/planner/binder.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"

namespace duckdb {

// Encapsulates database operations that are needed by the
// QuackingDuck prompt engine.
class DuckDatabaseInterface : public DatabaseInterface {
  public:
    DuckDatabaseInterface(ClientContext& context) : context_(context) { }
    virtual void ExtractSchema(ExtractedSchema& extracted_schema) {
        auto &catalog = duckdb::Catalog::GetCatalog(context_, INVALID_CATALOG);
        auto callback = [&](
                duckdb::SchemaCatalogEntry& schema_entry) {
            ExtractSchema(schema_entry, extracted_schema);
        };
        catalog.ScanSchemas(context_, callback);
    }

    virtual std::string ValidateParse(std::string query) {
        return ValidateQuery(query, false);
    }
    virtual std::string ValidateSemantics(std::string query) {
        return ValidateQuery(query, true);
    }

  private:
    ClientContext& context_;

  private:
    void ExtractSchema(
        duckdb::SchemaCatalogEntry& schema_entry,
        ExtractedSchema& extracted_schema) {

        auto callback = [&](duckdb::CatalogEntry& entry) {
            auto &table = (duckdb::TableCatalogEntry &)entry;
            std::string name = table.name;
            std::string sql = table.ToSQL();
            if (sql.substr(0, 6) == "SELECT") {
                // this is a system view that for some reason shows up
                // as a table (system views).
                return;
            }
            extracted_schema.table_ddl.emplace_back(sql);
        };
        schema_entry.Scan(duckdb::CatalogType::TABLE_ENTRY, callback);
    }

    // Validates a query and returns an error message if the query fails.
    std::string ValidateQuery(std::string query, bool bind) {
        std::string error_message;
        duckdb::Parser parser;
        try {
            parser.ParseQuery(query);
            if (parser.statements.size() == 0) {
                // This is not a good query. (how do we find out the error?)
                return "Unable to parse query";
            }
            if (bind) {
                shared_ptr<duckdb::Binder> binder = Binder::CreateBinder(context_);
                binder->Bind(*parser.statements[0]);
            }

            // Passed the parse and the bind test!
        } catch (duckdb::ParserException parser_exception) {
            error_message = parser_exception.RawMessage();
        } catch (duckdb::BinderException binder_exception) {
            error_message = binder_exception.RawMessage();
        } catch (duckdb::Exception other_exception) {
            // This isn't an error we can likely correct.
            error_message = "Unexpected Error Validating Query.";
        }
        return error_message;
    }
};

struct PromptFunctionData : public TableFunctionData {
    PromptFunctionData() : finished(false) { }
    std::string prompt;
    bool finished;
};

static void SummarizeSchemaFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
    auto &data = (PromptFunctionData &)*data_p.bind_data;
    if (data.finished) {
        return;
    }
    data.finished = true;

    DuckDatabaseInterface dbInterface(context);
    QuackingDuck quacking_duck(dbInterface);
    std::string response = quacking_duck.ExplainSchema();
    output.SetCardinality(1);
    output.SetValue(0, 0, Value(response));
}

static void PromptSqlFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
    auto &data = (PromptFunctionData &)*data_p.bind_data;
    if (data.finished) {
        return;
    }
    data.finished = true;

    DuckDatabaseInterface dbInterface(context);
    QuackingDuck quacking_duck(dbInterface);
    std::string response = quacking_duck.Ask(data.prompt);
    output.SetCardinality(1);
    output.SetValue(0, 0, Value(response));
}


static unique_ptr<FunctionData> SummarizeBind(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
    auto result = make_uniq<PromptFunctionData>();
    return_types.emplace_back(LogicalType::VARCHAR);
    names.emplace_back("summary");
    return std::move(result);
}

static unique_ptr<FunctionData> PromptBind(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
    auto result = make_uniq<PromptFunctionData>();
    if (input.inputs.size() > 0) {
      result->prompt = input.inputs[0].template GetValue<std::string>();
    }
    return_types.emplace_back(LogicalType::VARCHAR);
    names.emplace_back("query");
    return std::move(result);
}

static void FixupFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
    auto &data = (PromptFunctionData &)*data_p.bind_data;
    if (data.finished) {
        return;
    }
    data.finished = true;

    DuckDatabaseInterface dbInterface(context);
    QuackingDuck quacking_duck(dbInterface);
    std::string response = quacking_duck.FixupQuery(data.prompt);
    output.SetCardinality(1);
    output.SetValue(0, 0, Value(response));
}

static string PragmaPromptQuery(ClientContext &context, const FunctionParameters &parameters) {
    auto prompt = StringValue::Get(parameters.values[0]);
    DuckDatabaseInterface dbInterface(context);
    QuackingDuck quacking_duck(dbInterface);
    std::string query_result = quacking_duck.Ask(prompt);
    return query_result;
}

 void LoadInternal(DatabaseInstance &db_instance) {
    // create the TPCH pragma that allows us to run the query
    auto prompt_query_func = PragmaFunction::PragmaCall("prompt_query", PragmaPromptQuery, 
            {LogicalType::VARCHAR});
    ExtensionUtil::RegisterFunction(db_instance, prompt_query_func);

    TableFunction summarize_func("prompt_schema", {},
        SummarizeSchemaFunction, SummarizeBind);
    ExtensionUtil::RegisterFunction(db_instance, summarize_func);

    TableFunction prompt_func("prompt_sql", {LogicalType::VARCHAR}, PromptSqlFunction,
            PromptBind);
    ExtensionUtil::RegisterFunction(db_instance, prompt_func);

    TableFunction fixup_func("prompt_fixup", {LogicalType::VARCHAR}, FixupFunction, PromptBind);
    ExtensionUtil::RegisterFunction(db_instance, fixup_func);
}

void DuckpromptExtension::Load(DuckDB &db) {
    LoadInternal(*db.instance);
}
std::string DuckpromptExtension::Name() {
    return "duckprompt";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void duckprompt_init(duckdb::DatabaseInstance &db) {
    LoadInternal(db);
}

DUCKDB_EXTENSION_API const char *duckprompt_version() {
    return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
