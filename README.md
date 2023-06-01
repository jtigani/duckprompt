# duckprompt

This is a simple DuckDB extension that calls OpenAI's ChatGPT to do natural language queries. It is based on work from
Till Döhmen that he described in his blog post here: https://tdoehmen.github.io/blog/2023/03/07/quackingduck.html. 

To summarize, we first call ChatGPT to provide the schema, then we call again to get it generate a SQL query. We then check whether
the query is valid, and if not, we ask ChatGPT to fix it. 

We also don't
use samples of the data, only the schema, which makes it harder for chatGPT to answer some questions. But it works surprisingly 
well with the toy schema I've been working with, even coming up with JOINs and window functions.

There are three basic functions: 
* Ask a natural language query and get SQL back (`prompt_sql` table function)
* Ask a natual language query and run it (`prompt_query` pragma)
* Provide SQL that may or may not be valid and fix it (`prompt_fixup` table function)

---


## Running the extension

To run, you'll need an openai key. You can get one here:
https://platform.openai.com/account/api-keys

To run, first install and load the extension. If you build it yourself you can run the duckdb from `build/release/duckdb` and you can skip this step.
```
force install '<path to duckprompt>/duckprompt'
load duckprompt

```

To run a natual language query, use `pragma prompt_query`. For example:
```
D pragma prompt_query('Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.') ;
┌───────────┐
│ min(paid) │
│  double   │
├───────────┤
│     360.0 │
└───────────┘
```

If you want to see what queries actually get run, try the `prompt_sql` table function:
```
D select * from prompt_sql('Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.') ;
┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ prompt('Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.') │
│                                                       varchar                                                       │
├─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SELECT MIN(paid) FROM sales WHERE type_of_payment LIKE '%visa%';                                                    │
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

You can also "fix" a query. To fix a query, run `prompt_fixup' table function. This can detect errors with syntax,
usage, and even fix problems relating to the schame.
```
D select * from prompt_fixup("SEELECT * from customers");
┌─────────────────────────┐
│          query          │
│         varchar         │
├─────────────────────────┤
│ SELECT * FROM customers │
└─────────────────────────┘
```

Also note that if you want to set up sample data with a sales star schema you
can run the following script
```
build/release/duckdb -init ./scripts/build_sample_db.sql ./build/release/sales.db 
```
