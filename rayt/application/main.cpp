#include "test-application.h"
#include "octree-application.h"
#include "binary-util.h"
#include <cstdio>
#include <iostream>
using namespace std;

int main(int argc, char **argv) {
	freopen("log.txt","w", stdout);

	if (!rayt::BinaryUtil::CheckEndianness())
        crash("wrong endianness");

	if (argc > 1 && argv[1] == string("tree"))
		rayt::RunOctreeApplication(argc, argv);
	else {
		rayt::RunTestApplication(argc, argv);
	}

    return 0;
}
