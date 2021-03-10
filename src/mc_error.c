#include"mc.h"
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
//#include<assert.h>
int debugmode=DE_INFO;
const char* get_filename(const char *filename)
{
	int k;
	for(k=strlen(filename)-1;k>=0&&filename[k]!='/'&&filename[k]!='\\';--k);
	return filename+k+1;
}
void debugevent(int level, const char *format, ...)
{
	va_list args;
	if(level>=debugmode)
	{
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
	}
}
int crash(const char *file, int line, const char *expr, const char *msg, ...)
{
	printf("RUNTIME ERROR\n%s(%d)\n", get_filename(file), line);
	if(msg)
	{
		vprintf(msg, (char*)(&msg+1));
		printf("\n");
	}
	printf("%s\n", expr);
	_getch();
	exit(1);
	return 0;
}