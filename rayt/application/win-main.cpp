#ifdef WIN32

int main(int argc, char **argv);

#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	return main(1, &lpCmdLine);
}

#endif
