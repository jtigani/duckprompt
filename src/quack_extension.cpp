#define DUCKDB_EXTENSION_MAIN

#include <vector>
#include <string>

#include "quack_extension.hpp"
#include "chat.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"


#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

namespace duckdb {

inline void SummarizeSchemaScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) { 
            QuackingDuck quacking_duck;
            std::string response = quacking_duck.ExplainSchema();
            return StringVector::AddString(result, "Schema " + response + " 🐥");;
        });
}
inline void PromptScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) { 
            QuackingDuck quacking_duck;
            std::string response = quacking_duck.Ask(name.GetString());
            return StringVector::AddString(result, response);;
        });
}

static void LoadInternal(DatabaseInstance &instance) {
	Connection con(instance);
    con.BeginTransaction();

    auto &catalog = Catalog::GetSystemCatalog(*con.context);

    CreateScalarFunctionInfo summarize_schema_fun_info(
            ScalarFunction("summarize_schema", {LogicalType::VARCHAR}, LogicalType::VARCHAR, SummarizeSchemaScalarFun));
    CreateScalarFunctionInfo prompt_fun_info(
            ScalarFunction("prompt", {LogicalType::VARCHAR}, LogicalType::VARCHAR, PromptScalarFun));
    prompt_fun_info.on_conflict = OnCreateConflict::ALTER_ON_CONFLICT;
    summarize_schema_fun_info.on_conflict = OnCreateConflict::ALTER_ON_CONFLICT;
    catalog.CreateFunction(*con.context, &prompt_fun_info);
    catalog.CreateFunction(*con.context, &summarize_schema_fun_info);
    con.Commit();
}

void QuackExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string QuackExtension::Name() {
	return "quack";
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
