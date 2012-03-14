#include "debug-check-tree.h"
#include "stored-octree-traverser.h"
#include "octree-constants.h"
using namespace std;
using namespace boost;

namespace rayt {

	static bool CheckSubtree(shared_ptr<StoredOctreeTraverser::Node> node, int depth_left, int &counter) {
		if (++counter % 10000 == 0) {
			cout << "checked " << counter << endl;
		}

		bool ret = true;

		if (depth_left < 0) {
			cout << "too deep" << endl;
			ret = false;
		}
		
		vector<shared_ptr<StoredOctreeTraverser::Node> > c = node->Children();
		bool has_children = false;
		for (int i = 0; i < 8; ++i) {
			if (c[i] != NULL) {
				has_children = true;
				ret &= CheckSubtree(c[i], depth_left - 1, counter);
			}
		}
		if (!has_children && depth_left > 0) {
			cout << "not deep enough" << endl;
			ret = false;
		}

		return ret;
	}

	void DebugCheckConstantDepthTree(std::string file_name, int expected_depth) {
		shared_ptr<StoredOctreeLoader> loader(new StoredOctreeLoader(file_name));
		StoredOctreeTraverser traverser(loader);

		int counter = 0;
		if (CheckSubtree(traverser.Root(), expected_depth, counter)) {
			cout << "checked ok" << endl;
		} else {
			cout << "there were errors" << endl;
		}
	}

	//static void CheckSubtree(const char *data, uint index, 

	void DebugCheckConstantDepthTree(GPUOctreeData *gpu_data, int expected_depth) {
		const CLBuffer *clbuf = gpu_data->ChannelByName(kNodeLinkChannelName)->cl_buffer();
		char *data = new char[clbuf->size()];
		clbuf->Read(0, clbuf->size(), data);

	}

}
