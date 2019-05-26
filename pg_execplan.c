/*
 * pg_execplan.c
 *
 */

#include "postgres.h"

#include "access/printtup.h"
#include "commands/extension.h"
#include "commands/prepare.h"
#include "executor/executor.h"
#include "nodes/nodes.h"
#include "nodes/plannodes.h"
#include "tcop/pquery.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/plancache.h"
#include "utils/snapmgr.h"
#include "plan_utils.h"


#define EXPLAN_DEBUG_LEVEL 0

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(store_query_plan);
PG_FUNCTION_INFO_V1(pg_exec_plan);
PG_FUNCTION_INFO_V1(cache_stored_plan);

void _PG_init(void);
void _PG_fini(void);

void
_PG_init(void)
{
	return;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	return;
}

Datum
store_query_plan(PG_FUNCTION_ARGS)
{
	char			*query_string = TextDatumGetCString(PG_GETARG_DATUM(1)),
					*filename = TextDatumGetCString(PG_GETARG_DATUM(0)),
					*plan_string;
	int				nstmts;
	FILE			*fout;
	MemoryContext	oldcontext;
	List	  		*parsetree_list;
	RawStmt			*parsetree;
	List			*querytree_list,
					*plantree_list;

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(LOG, "Store into %s plan of the query %s.", filename, query_string);

	oldcontext = MemoryContextSwitchTo(MessageContext);

	parsetree_list = pg_parse_query(query_string);
	nstmts = list_length(parsetree_list);
	if (nstmts != 1)
		elog(ERROR, "Query contains %d elements, but must contain only one.", nstmts);

	parsetree = (RawStmt *) linitial(parsetree_list);
	querytree_list = pg_analyze_and_rewrite(parsetree, query_string, NULL, 0, NULL);
	plantree_list = pg_plan_queries(querytree_list, CURSOR_OPT_PARALLEL_OK, NULL);

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "BEFORE writing %s ...", filename);

	fout = fopen(filename, "wt");
	if (fout == NULL)
	{
		elog(WARNING, "Can't create file %s.", filename);
		PG_RETURN_BOOL(false);
	}

	set_portable_output(true);
	plan_string = nodeToString((PlannedStmt *) linitial(plantree_list));
	set_portable_output(false);

	fprintf(fout, "%s\n", query_string);
	fprintf(fout, "%s\n", plan_string);

	fclose(fout);
	MemoryContextSwitchTo(oldcontext);
	PG_RETURN_BOOL(true);
}

static void
cache_plan(const char *stmt_name, const char *query_string, const char *plan_string)
{
	PlannedStmt			*pstmt;
	CachedPlanSource	*psrc;

	PG_TRY();
	{
		set_portable_input(true);
		pstmt = (PlannedStmt *) stringToNode(plan_string);
		set_portable_input(false);
	}
	PG_CATCH();
	{
		elog(INFO, "BAD PLAN: %s. Query: %s", plan_string, query_string);
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (EXPLAN_DEBUG_LEVEL > 0)
		elog(INFO, "query: %s\n", query_string);
	if (EXPLAN_DEBUG_LEVEL > 1)
		elog(INFO, "\nplan: %s\n", plan_string);

	psrc = CreateCachedPlan(NULL, query_string, NULL);
	CompleteCachedPlan(psrc, NIL, NULL, NULL, 0, NULL, NULL,
							   CURSOR_OPT_GENERIC_PLAN, false);
	SetRemoteSubplan(psrc, pstmt);
	StorePreparedStatement(stmt_name, psrc, true);
}

static bool
LoadPlanFromFile(const char *filename, char **query_string, char **plan_string)
{
	FILE	*fin;
	size_t	string_len;

	fin = fopen(filename, "rt");
	{
		elog(WARNING, "Can't create file %s.", filename);
		return false;
	}

	fscanf(fin, "%lu", &string_len);
	*query_string = palloc0(string_len + 1);
	fscanf(fin, "%s", *query_string);
	fscanf(fin, "%lu", &string_len);
	*plan_string = palloc0(string_len + 1);
	fscanf(fin, "%s", *plan_string);
	fclose(fin);

}

Datum
cache_stored_plan(PG_FUNCTION_ARGS)
{
	char				*filename = TextDatumGetCString(PG_GETARG_DATUM(0)),
						*stmt_name = TextDatumGetCString(PG_GETARG_DATUM(1)),
						*query_string = NULL,
						*plan_string = NULL;

	if (!LoadPlanFromFile(filename, &query_string, &plan_string))
		PG_RETURN_BOOL(false);

	cache_plan(stmt_name, query_string, plan_string);
	pfree(query_string);
	pfree(plan_string);
	PG_RETURN_BOOL(true);
}
