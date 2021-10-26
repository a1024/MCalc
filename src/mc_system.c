//static const char file[]=__FILE__;

//_WIN32
#ifdef __linux__//linux stuff
#include <termios.h>//https://stackoverflow.com/questions/7469139/what-is-the-equivalent-to-getch-getche-in-linux
static struct termios	old, current;
void	initTermios(int echo)//Initialize new terminal i/o settings
{
	tcgetattr(0, &old);//grab old terminal i/o settings
	current=old;//make new settings same as old settings
	current.c_lflag&=~ICANON;//disable buffered i/o
	if(echo)
		current.c_lflag|=ECHO;//set echo mode
	else
		current.c_lflag&=~ECHO;//set no echo mode
	tcsetattr(0, TCSANOW, &current);//use these new terminal i/o settings now
}
void	resetTermios()//Restore old terminal i/o settings
{
	tcsetattr(0, TCSANOW, &old);
}
char	getch_sel(int echo)//Read 1 character - echo defines echo mode
{
	char ch;
	initTermios(echo);
	ch=getchar();
	resetTermios();
	return ch;
}
char	_getch()//Read 1 character without echo
{
	return getch_sel(0);
}
char	_getche()//Read 1 character with echo
{
	return getch_sel(1);
}
#else
#include	<Windows.h>
CONSOLE_SCREEN_BUFFER_INFO csbi;
void		get_console_size(short *w, short *h)
{
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	*w=csbi.dwSize.X;
	*h=csbi.dwSize.Y;
}
int			set_console_buffer_size(short w, short h)
{
	COORD coords={w, h};
	HANDLE handle=GetStdHandle(STD_OUTPUT_HANDLE);
	int success=SetConsoleScreenBufferSize(handle, coords);
	if(!success)
		printf("Failed to resize console buffer: %d\n\n", GetLastError());
	return success;
}
#endif