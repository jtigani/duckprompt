#define DUCKDB_EXTENSION_MAIN

#include <vector>
#include <string>

#include "quack_extension.hpp"
#include "chat.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/function_binder.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/parser/parsed_data/create_pragma_function_info.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "duckdb/parser/parsed_data/create_view_info.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/parser/statement/select_statement.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"

namespace duckdb {

struct PromptFunctionData : public TableFunctionData {
	PromptFunctionData() {
	}

    std::string prompt;
	bool finished = false;
};

static void SummarizeSchemaFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = (PromptFunctionData &)*data_p.bind_data;
	if (data.finished) {
		return;
	}
	data.finished = true;

    QuackingDuck quacking_duck;
    auto &catalog = Catalog::GetCatalog(context, INVALID_CATALOG);
    auto schema = catalog.GetSchema(context); // Get the default schema.
    quacking_duck.StoreSchema(*schema);

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

    QuackingDuck quacking_duck;
    auto &catalog = Catalog::GetCatalog(context, INVALID_CATALOG);
    auto schema = catalog.GetSchema(context); // Get the default schema.
    quacking_duck.StoreSchema(*schema);
    std::string response = quacking_duck.Ask(data.prompt);
    output.SetCardinality(1);
    output.SetValue(0, 0, Value(response));
}

// Validates a query and returns an error message if the query fails.
std::string ValidateQuery(std::string query, duckdb::Parser& parser, duckdb::Binder& binder) {
    std::string error_message;
    try {
        parser.ParseQuery(query);
        if (parser.statements.size() == 0) {
            // This is not a good query. (how do we find out the error?)
            return "Unable to parse query";
        }
        // TODO: Validate the query is a SELECT query, we shouldn't be automagically doing mutations.

        binder.Bind(*parser.statements[0]);

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

std::string ValidateAndFixup(ClientContext &context, QuackingDuck quacking_duck, std::string query) {
    duckdb::Parser parser;
    shared_ptr<duckdb::Binder> binder = Binder::CreateBinder(context);
    std::string error_message = ValidateQuery(query, parser, *binder);
    if (error_message.size() == 0) {
        return query;
    } else {
        // First analyze the query which tells the AI about the query and adds it to the 
        // chat context.
        if (!quacking_duck.HasSeenQuery(query)) {
            quacking_duck.AnalyzeQuery(query);
        }
        return quacking_duck.FixupQuery(error_message);
    }
}

static unique_ptr<FunctionData> SummarizeBind(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_unique<PromptFunctionData>();
	return_types.emplace_back(LogicalType::VARCHAR);
	names.emplace_back("summary");
	return std::move(result);
}

static unique_ptr<FunctionData> PromptBind(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
	auto result = make_unique<PromptFunctionData>();
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

    QuackingDuck quacking_duck;
    auto &catalog = Catalog::GetCatalog(context, INVALID_CATALOG);
    auto schema = catalog.GetSchema(context); // Get the default schema.
    quacking_duck.StoreSchema(*schema);
    std::string response = ValidateAndFixup(context, quacking_duck, data.prompt);
    output.SetCardinality(1);
    output.SetValue(0, 0, Value(response));
}

static string PragmaPromptQuery(ClientContext &context, const FunctionParameters &parameters) {
	auto prompt = StringValue::Get(parameters.values[0]);
    auto &catalog = Catalog::GetCatalog(context, INVALID_CATALOG);
    auto schema = catalog.GetSchema(context); // Get the default schema.
    QuackingDuck quacking_duck;
    quacking_duck.StoreSchema(*schema);
    std::string query_result = quacking_duck.Ask(prompt);
    return ValidateAndFixup(context, quacking_duck, query_result);
}

 void LoadInternal(DatabaseInstance &instance) {
    Connection con(instance);
	con.BeginTransaction();
	auto &catalog = Catalog::GetSystemCatalog(*con.context);

    // create the TPCH pragma that allows us to run the query
	auto prompt_query_func = PragmaFunction::PragmaCall("prompt_query", PragmaPromptQuery, {LogicalType::VARCHAR});
	CreatePragmaFunctionInfo info(prompt_query_func);
	catalog.CreatePragmaFunction(*con.context, &info);

    TableFunction summarize_func("prompt_schema", {},
        SummarizeSchemaFunction, SummarizeBind);
    CreateTableFunctionInfo summarize_info(summarize_func);
	catalog.CreateTableFunction(*con.context, &summarize_info);

    TableFunction prompt_func("prompt_sql", {LogicalType::VARCHAR}, PromptSqlFunction, PromptBind);
    CreateTableFunctionInfo prompt_info(prompt_func);
    catalog.CreateTableFunction(*con.context, &prompt_info);

	TableFunction fixup_func("prompt_fixup", {LogicalType::VARCHAR}, FixupFunction, PromptBind);
	CreateTableFunctionInfo fixup_info(fixup_func);
	catalog.CreateTableFunction(*con.context, &fixup_info);
    con.Commit();
}

void QuackExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string QuackExtension::Name() {
	return "duckprompt";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void quack_init(duckdb::DatabaseInstance &db) {
	LoadInternal(db);
}

DUCKDB_EXTENSION_API const char *quack_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
