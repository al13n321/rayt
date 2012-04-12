#include "octree-application.h"
#include <iostream>
#include <sstream>
#include "import-obj-scene.h"
using namespace std;

namespace rayt {

	static void CrashShowingUsage(string name) {
		name += " tree ";

		cerr << "usage:" << endl;
		cerr << "  " << name << "obj2tree <obj file> <tree file> <depth> <nodes in block>" << endl;
		cerr << "  " << name << "info <tree file>" << endl;

		crash("invalid arguments");
	}

	void RunOctreeApplication(int argc, char **argv) {
		if (argc <= 2)
			CrashShowingUsage(argv[0]);

		string cmd = argv[2];

		if (cmd == "obj2tree") {
			if (argc <= 6)
				CrashShowingUsage(argv[0]);

			string obj_file = argv[3];
			string tree_file = argv[4];
			int depth = atoi(argv[5]);
			int nodes_in_block = atoi(argv[6]);

			ImportObjScene(depth, nodes_in_block, obj_file, tree_file);
		} else if (cmd == "info") {
			crash("not implemented");
		}
	}

}
