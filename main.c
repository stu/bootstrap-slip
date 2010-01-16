/*
	Bootstrap slip.

	Ideas from bootstrap scheme from Peter Michaux (http://peter.michaux.ca/)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#ifndef __MINGW_H
	#include <sys/stat.h>
#endif

#include "dlist.h"
#include "slip.h"
#include "slip_parser.h"
#include "slip_tokeniser.h"


static uint32_t get_filesize(FILE *fp)
{
	uint32_t n, o;

	o = ftell(fp);

	fseek(fp, 0x0L, SEEK_END);
	n = ftell(fp);

	fseek(fp, o, SEEK_SET);

	return n;
}

static int TokeniseBuffer(pSlip ctx, char *buff, uint32_t bufflen)
{
	char *z;
	pToken t;
	int rc = -1;

	z = buff;

	// cleans out token stream, resets info.
	slip_reset_parser(ctx);

	do
	{
		int flag;

		flag = 0;

		t = PP_IDToken(ctx, z);

		if (t != NULL)
		{

			t->line = ctx->parse_data.current_line;

			if (t->id == kNEWLINE)
			{
				// TODO: Test multiple newline tokens in one...
				ctx->parse_data.current_line += strlen(t->z);
			}

			/****************/
			if (t->id == kCOMMENT_START)
			{
				ctx->parse_data.comment_depth += 1;
				if (ctx->parse_data.comment_depth > 1)
				{
					printf("Nested comments unsupported\n");
					exit(0);
				}
				flag = 1;
				z = ctx->parse_data.buff_start + strlen(t->z);
				FreeToken(t);
				t = (void*) 1;
				flag = 1;
			}
			else if (t->id == kCOMMENT_END)
			{
				ctx->parse_data.comment_depth -= 1;
				if (ctx->parse_data.comment_depth < 0)
				{
					printf("Unmatched comments\n");
					exit(0);
				}
				flag = 1;
				z = ctx->parse_data.buff_start + strlen(t->z);
				FreeToken(t);
				t = (void*) 1;
				flag = 1;
			}


			// check line comment
			if (flag == 0 && ctx->parse_data.comment_depth == 1000)
			{
				z = ctx->parse_data.buff_start + strlen(t->z);
				if (t->id == kNEWLINE)
				{
					ctx->parse_data.comment_depth = 0;
					FreeToken(t);
					t = (void*) 1;
					flag = 1;
				}
				else
				{
					FreeToken(t);
					t = (void*) 1;
					flag = 1;
				}
			}
			else if (flag == 0 && ctx->parse_data.comment_depth >= 1)
			{
				z = ctx->parse_data.buff_start + strlen(t->z);
				// skip token!
				FreeToken(t);
				t = (void*) 1;
				flag = 1;
			}
			else if (flag == 0 && t->id == kCOMMENT_LINE)
			{
				z = ctx->parse_data.buff_start + strlen(t->z);
				// skip token!
				ctx->parse_data.comment_depth = 1000;
				FreeToken(t);
				t = (void*) 1;
				flag = 1;
			}

			else if (flag == 0)
			{
				if (strlen(t->z) > 0)
				{
					dlist_ins(ctx->parse_data.lstTokens, t);
					z = ctx->parse_data.buff_start + strlen(t->z);
				}
			}
		}
	} while (t != NULL);

	if (ctx->parse_data.comment_depth > 0)
	{
		printf("Unclosed comments\n");
	}

	rc = 0;

	ctx->parse_data.eCurrentToken = dlist_head(ctx->parse_data.lstTokens);

	return rc;
}

static int TokeniseFiles(pSlip ctx, char *fn)
{
	char *buff = NULL;
	uint32_t bufflen;
	FILE *fp = NULL;
	int rc = -1;
#ifndef __MINGW_H
	struct stat sbuff;
#endif

	fp = NULL;
#ifndef __MINGW_H
	stat(fn, &sbuff);
	if (S_ISREG(sbuff.st_mode) != 0)
		fp = fopen(fn, "rb");
#else
	fp = fopen(fn, "rb");
#endif
	if (fp != NULL)
	{
		bufflen = get_filesize(fp);
		buff = calloc(1, bufflen + 16);
		if (buff != NULL)
		{
			bufflen = fread(buff, 1, bufflen, fp);
			fclose(fp);
			fp = NULL;

			rc = TokeniseBuffer(ctx, buff, bufflen);
		}
		else
			printf("Not enough memory for read buffer\n");
	}
	else
		printf("Failed to open file : %s\n", fn);

	if (fp != NULL)
		fclose(fp);

	if (buff != NULL)
		free(buff);

	return rc;
}


int main(int argc, char *argv[])
{
	pSlip slip;
	char *buff;

	FILE *fp = stdin;
	buff = malloc(1024);

	if (argc == 2)
	{
		fp = fopen(argv[1], "rt");
		assert(fp != NULL);
	}

	slip = slip_init();
	assert(slip != NULL);

	printf("\nWelcome to bootstrap slip. Use ctrl-c to exit.\n");

	while (slip->running == SLIP_RUNNING)
	{
		char *p;

		pSlipObject obj;

		printf("> ");
		fflush(stdout);

		memset(buff, 0x0, 1024);
		fflush(stdout);
		fgets(buff, 1023, fp);

		// echo if script
		if (fp != stdin)
			printf("%s", buff);
		else
		{
			// trim off cr/lf.s
			p = strchr(buff, 0x0A); if (p != NULL) *p = 0;
			p = strchr(buff, 0x0D); if (p != NULL) *p = 0;
		}

		fflush(stdout);

		if (buff[0] != 0)
		{
			if ( TokeniseBuffer(slip, buff, strlen(buff)) == 0)
			{
				/*
				while (slip->parse_data.eCurrentToken != NULL)
				{
					pToken t;
					t = dlist_data(slip->parse_data.eCurrentToken);
					assert(t != NULL);
					slip_parser_(slip->parse_data.pParser, t->id, t, slip);
					slip->parse_data.eCurrentToken = dlist_next(slip->parse_data.eCurrentToken);
				}
				*/

				obj = slip_read(slip);
				if (slip->running == SLIP_RUNNING && obj != NULL)
				{
					obj = slip_evaluate(slip, obj);
					if (slip->running == SLIP_RUNNING && obj != NULL)
						slip_write(slip, obj);
				}
			}
			else
				printf("tokenise buffer failed\n");

			printf("\n");
		}
		else
			slip->running = SLIP_SHUTDOWN;
	};

	if (slip->running == SLIP_SHUTDOWN)
		printf("\nThank you.\n");

	slip_release(slip);

	if (fp != stdin)
		fclose(fp);

	return 0;
}

/*
  while (ctx->eCurrentToken != NULL)
	{
		pToken t;

		t = dlist_data(ctx->eCurrentToken);


		//LogInfo("pass in %s(%i)(%i)[%s]\n", t->file, t->line, t->id, t->z);
		xcc_parser_(ctx->pParser, t->id, t, ctx);

		ctx->eCurrentToken = dlist_next(ctx->eCurrentToken);
	}

	xcc_parser_(ctx->pParser, 0, NULL, ctx);
*/
