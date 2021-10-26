//mc_error.c - MCalc Error handling
//Copyright (C) 2021  Ayman Wagih Mohsen
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
	printf("%s\t == false\n", expr);
	_getch();
	exit(1);
	return 0;
}