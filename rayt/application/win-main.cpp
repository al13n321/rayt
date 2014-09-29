#ifdef WIN32

int main(int argc, char **argv);

#include <windows.h>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <iostream>
using namespace std;

void InitializeDebugConsole()
{	
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
		return;

	//Redirect unbuffered STDOUT to the console
	HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
    FILE *COutputHandle = _fdopen(SystemOutput, "w" );
    *stdout = *COutputHandle;
    setvbuf(stdout, NULL, _IONBF, 0);

    //Redirect unbuffered STDERR to the console
	HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
    int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
    FILE *CErrorHandle = _fdopen(SystemError, "w" );
    *stderr = *CErrorHandle;
    setvbuf(stderr, NULL, _IONBF, 0);

    //Redirect unbuffered STDIN to the console
	HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
    FILE *CInputHandle = _fdopen(SystemInput, "r" );
    *stdin = *CInputHandle;
    setvbuf(stdin, NULL, _IONBF, 0);
    
    //make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
	ios::sync_with_stdio(true);

	cout << endl;
	cout << "console issues on windows:" << endl;
	cout << "if the application is run not from bat file, it works with console incorrectly" << endl;
	cout << "console prompt may appear when it's not terminated" << endl;
	cout << "console prompt may not appear when it's terminated (hitting return key helps)" << endl;
	cout << "if you happen to run me not from bat file, use task manager to check if I terminated" << endl;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	InitializeDebugConsole();

	return main(__argc, __argv);
}

#endif
