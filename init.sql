\echo Use "CREATE EXTENSION pg_execplan" to load this file. \quit

-- Store plan of a query into a text file.
-- query - query string which will be parsed and planned.
-- filename - path to the file on a disk.
-- name - name of prepared statement
CREATE OR REPLACE FUNCTION @extschema@.store_query_plan(
															filename	TEXT,
															query		TEXT,
														  )
RETURNS BOOL AS $$
LANGUAGE C;

CREATE OR REPLACE FUNCTION @extschema@.cache_stored_plan(filename	TEXT,
														 name		TEXT)
RETURNS BOOL AS $$
LANGUAGE C;
