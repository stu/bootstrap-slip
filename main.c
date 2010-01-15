/*
 	Bootstrap slip.

	Ideas from bootstrap scheme from Peter Michaux (http://peter.michaux.ca/)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "dlist.h"
#include "slip.h"

int main(int argc, char *argv[])
{
	pSlip slip;
	char *buff;

	FILE *fp = stdin;
	buff = malloc(1024);

	if(argc == 2)
	{
		fp = fopen(argv[1], "rt");
		assert(fp != NULL);
	}

	slip = slip_init();
	assert(slip != NULL);

	printf("\nWelcome to bootstrap slip. Use ctrl-c to exit.\n");

	while (slip->running == SLIP_RUNNING)
	{
		pSlipObject obj;

		printf("> ");
		fflush(stdout);

		memset(buff, 0x0, 1024);
		fflush(stdout);
		fgets(buff, 1023, fp);

		// echo if script
		if(fp != stdin)
			printf("%s", buff);

		fflush(stdout);

		if(buff[0] != 0)
		{
			obj = slip_read(slip, buff, strlen(buff));
			if(slip->running == SLIP_RUNNING && obj != NULL)
			{
				obj = slip_evaluate(slip, obj);
				if(slip->running == SLIP_RUNNING && obj != NULL)
					slip_write(slip, obj);
			}

			printf("\n");
		}
		else
			slip->running = SLIP_SHUTDOWN;
	};

	if(slip->running == SLIP_SHUTDOWN)
		printf("\nThank you.\n");

	slip_release(slip);

	if(fp != stdin)
		fclose(fp);

	return 0;
}

