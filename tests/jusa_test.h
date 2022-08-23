#ifndef JUSA_TEST_H
#define JUSA_TEST_H

#include <stdio.h>
#include <string.h>

#define TEST_EQUAL(exp, value) {PrintResult("TEST_EQUAL("#exp", "#value")", (exp) == value);}

void PrintResult(char* name, bool ok, int align = 60)
{
	char* testResult = ok ? "OK" : "FAIL";
	size_t nameLength = strlen(name);
	size_t remain = align - nameLength;

	if (remain > 0)
	{
		printf("%s", name);
		for(size_t i = 0; i < remain; ++i)
		{
			putc('.', stdout);
		}
		printf("%s\n", testResult);
	}
	else
	{
		printf("Not enough space!");
	}
}

#endif