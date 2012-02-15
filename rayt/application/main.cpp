#include "test-application.h"
#include <cstdio>

int main(int argc, char **argv) {
    freopen("log.txt","w", stdout);

    rayt::RunTestApplication(argc, argv);

    return 0;
}
