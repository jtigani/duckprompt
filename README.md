# duckprompt

This is a simple DuckDB extension that calls OpenAI's ChatGPT to do natural language queries. It is based on work from
Till Döhmen that he described in his blog post here: https://tdoehmen.github.io/blog/2023/03/07/quackingduck.html. 

To summarize, we first call ChatGPT to provide the schema, then we call again to get it generate a SQL query. We then check whether
the query is valid, and if not, we ask ChatGPT to fix it. 

The context only includes the schema, not data, which makes it harder for chatGPT to answer some questions. But it works surprisingly 
well with the toy schema I've been working with, even coming up with JOINs and window functions.

There are three basic functions: 
* Ask a natural language query and get SQL back (`prompt_sql` table function)
* Ask a natual language query and run it (`prompt_query` pragma)
* Provide SQL that may or may not be valid and fix it (`prompt_fixup` table function)

---

## Installing the extension
The binaries for OSX amd64 are built and staged for duckdb 0.8.0 under the s3 location `s3://motherduck-duckdb-extensions/jordan/duckprompt`. 
They're not signed so you need to run duckdb with the `-unsigned` flag.
```
 % duckdb -unsigned
v0.8.0 e8e4cea5ec
Enter ".help" for usage hints.
D SET custom_extension_repository='motherduck-duckdb-extensions.s3.amazonaws.com/jordan/duckprompt/duckprompt/0.0.1';
D install duckprompt;
D load duckprompt;
```

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

## Examples
Also note that if you want to set up sample data with a sales star schema you
can run the following script
```
build/release/duckdb -init ./scripts/build_sample_db.sql ./build/release/sales.db 
```

Here are some example questions to ask:
```
pragma duck_prompt("Who bought the most PCs, print also the users name?");
pragma duck_prompt("List only the model number of all products made by maker B.");
pragma duck_prompt("Return the minimum amount paid by customers who used a visa card (debit or credit) to purchase a product.");
pragma duck_prompt("Find the customer_id of customers who have the letter 'e' either in their first name or in their last name.");
pragma duck_prompt("
    Assume all prices in the Laptops table are in Euro. List the model numbers of all laptops with ram at least 1024. For each model,
    list also its price in USD. Assume that 1 USD = 0.85 EURO (you need to divide the price by 0.85). Name the price column 'price (USD)'.");
pragma duck_prompt("Return a list of makers that make more than four different models.");
pragma duck_prompt("List all first names of customers in an ascending order based on the number of purchases made by customers with that first name.");
pragma duck_prompt("Show a list of sales per customer, with a running sum of their total spending across all of their sales");

select * from prompt_fixup("SELEECT * from customers");
select * from prompt_fixup("SELECT * from customer");
