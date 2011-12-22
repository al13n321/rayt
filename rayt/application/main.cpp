#include "test-application.h"
#include <cstdio>

#ifdef __WIN32__
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	freopen("log.txt","w", stdout);
    
	rayt::RunTestApplication(1, &lpCmdLine);
    
	return 0;
}
#else
int main(int argc, char **argv) {
    //freopen("log.txt","w", stdout);

    rayt::RunTestApplication(argc, argv);

    return 0;
}
#endif