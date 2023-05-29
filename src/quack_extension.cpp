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
#include "duckdb/parser/parsed_data/create_view_info.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/parser/statement/select_statement.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"

namespace duckdb {

struct QuackingDuckBindData : public FunctionData {
    QuackingDuckBindData()  {
    }

    QuackingDuckBindData(const QuackingDuckBindData &other) {
        throw NotImplementedException("FIXME: serialize");
    }

    bool Equals(const FunctionData &other_p) const override {
        auto &other = (QuackingDuckBindData &)other_p;
        return true;
    }
    unique_ptr<FunctionData> Copy() const override {
        return make_unique<QuackingDuckBindData>(*this);
    }

    static void Serialize(FieldWriter &writer, const FunctionData *bind_data_p, const ScalarFunction &function) {
        throw NotImplementedException("FIXME: serialize");
    }

    static unique_ptr<FunctionData> Deserialize(ClientContext &context, FieldReader &reader,
                                                ScalarFunction &bound_function) {
        throw NotImplementedException("FIXME: serialize");
    }

    QuackingDuck quacking_duck;
};

inline void SummarizeSchemaScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
    
    auto &func_expr = (BoundFunctionExpression &)state.expr;
    auto &info = (QuackingDuckBindData &)*func_expr.bind_info;
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) { 
            std::string response = info.quacking_duck.ExplainSchema();
            return StringVector::AddString(result, response);;
        });
}



unique_ptr<FunctionData> SummarizeSchemaBind(ClientContext &context, ScalarFunction &bound_function,
    vector<unique_ptr<Expression> > &arguments) {
    auto bind_data =  make_unique<QuackingDuckBindData>();
    
    auto &catalog = Catalog::GetCatalog(context, INVALID_CATALOG);
    auto schema = catalog.GetSchema(context); // Get the default schema.
    bind_data->quacking_duck.StoreSchema(*schema);
    return bind_data;
}

unique_ptr<FunctionData> PromptBind(ClientContext &context, ScalarFunction &bound_function,
    vector<unique_ptr<Expression> > &arguments) {
    auto bind_data = make_unique<QuackingDuckBindData>();
    auto &catalog = Catalog::GetCatalog(context, INVALID_CATALOG);
    auto schema = catalog.GetSchema(context); // Get the default schema.
    bind_data->quacking_duck.StoreSchema(*schema);
    return bind_data;
}

inline void PromptScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    auto &func_expr = (BoundFunctionExpression &)state.expr;
    auto &info = (QuackingDuckBindData &)*func_expr.bind_info;
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) { 
            std::string response = info.quacking_duck.Ask(name.GetString());
            return StringVector::AddString(result, response);;
        });
}

static string PragmaPromptQuery(ClientContext &context, const FunctionParameters &parameters) {
	auto prompt = StringValue::Get(parameters.values[0]);

    auto &catalog = Catalog::GetCatalog(context, INVALID_CATALOG);
    auto schema = catalog.GetSchema(context); // Get the default schema.
    QuackingDuck quacking_duck;
    quacking_duck.StoreSchema(*schema);
    return quacking_duck.Ask(prompt);
}


 void LoadInternal(DatabaseInstance &instance) {
    Connection con(instance);
	con.BeginTransaction();
	auto &catalog = Catalog::GetSystemCatalog(*con.context);

    // create the TPCH pragma that allows us to run the query
	auto prompt_query_func = PragmaFunction::PragmaCall("prompt_query", PragmaPromptQuery, {LogicalType::VARCHAR});
	CreatePragmaFunctionInfo info(prompt_query_func);
	catalog.CreatePragmaFunction(*con.context, &info);


    auto summarize_func = ScalarFunction("summarize_schema", {LogicalType::VARCHAR}, LogicalType::VARCHAR,
        SummarizeSchemaScalarFun, SummarizeSchemaBind);
    CreateScalarFunctionInfo summarize_info(summarize_func);
	catalog.CreateFunction(*con.context, &summarize_info);

    auto prompt_func = ScalarFunction("prompt", {LogicalType::VARCHAR}, LogicalType::VARCHAR, PromptScalarFun,
        PromptBind);
    CreateScalarFunctionInfo prompt_info(prompt_func);
    catalog.CreateFunction(*con.context, &prompt_info);
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
