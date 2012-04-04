#include "test-application.h"
#include "octree-application.h"
#include <cstdio>
#include <iostream>
using namespace std;

int main(int argc, char **argv) {
	freopen("log.txt","w", stdout);

	if (argc <= 1 || argv[1] == string("test"))
		rayt::RunTestApplication(argc, argv);
	else if (argv[1] == string("tree"))
		rayt::RunOctreeApplication(argc, argv);
	else {
		cerr << "usage:" << endl;
		cerr << "  " << argv[0] << endl;
		cerr << "  " << argv[0] << " tree ..." << endl;
		crash("invalid arguments");
	}

    return 0;
}
