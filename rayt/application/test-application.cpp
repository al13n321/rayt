#include "test-application.h"
#include "test-stopwatch.h"
#include "blocking-experiment.h"
#include "stored-octree-test.h"
#include "camera.h"
#include "cl-context.h"
#include "gpu-ray-tracer.h"
#include "rendered-image-drawer.h"
#include "binary-util.h"
#include "debug.h"
#include "debug-ray-cast.h"
#include "debug-check-tree.h"
#include "import-obj-scene.h"
#include <cstdio>
#include <iostream>
using namespace std;
using namespace boost;

namespace rayt {

    const int winhei = 512;
    const int winwid = 512;
    const int imgwid = 512;
    const int imghei = 512;
    int main_window;

    int prev_mousex;
    int prev_mousey;
    bool key_pressed[256];
    TestStopwatch frame_stopwatch;
    
    const float movement_speed = 0.5; // units per second
    const float looking_speed = 0.2; // degrees per pixel

    TestStopwatch fps_stopwatch;
    const int fps_update_period = 20;
    int cur_frame_number;
    
    Camera camera;
    shared_ptr<StoredOctreeLoader> loader;
    shared_ptr<CLContext> context;
    scoped_ptr<GPURayTracer> raytracer;
    scoped_ptr<RenderedImageDrawer> drawer;
    shared_ptr<GPUOctreeCacheManager> cache_manager;

    static void DisplayFunc() {
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        drawer->DrawImageFromBuffer(*raytracer->out_image());
        glutSwapBuffers();
    }

    static void IdleFunc() {
        raytracer->RenderFrame(camera);
        glutPostRedisplay();
        
        fvec3 movement(0, 0, 0);
        if (key_pressed['a'])
            movement.x -= 1;
        if (key_pressed['d'])
            movement.x += 1;
        if (key_pressed['w'])
            movement.z -= 1;
        if (key_pressed['s'])
            movement.z += 1;
        double frame_time = frame_stopwatch.Restart();
        camera.MoveRelative(movement * frame_time * movement_speed);

        if (cur_frame_number % fps_update_period == 0) {
            char fps_str[100];
            double fps = 1.0 / fps_stopwatch.Restart() * fps_update_period;
            sprintf(fps_str, "%lf", fps);
            string title = string("FPS: ") + fps_str;

            glutSetWindowTitle(title.c_str());
        }

        ++cur_frame_number;
    }

    static void KeyboardFunc(unsigned char key, int x, int y) {
        if (key == 27)
            exit(0);
        key_pressed[key] = true;
    }

    static void KeyboardUpFunc(unsigned char key, int x, int y) {
        key_pressed[key] = false;
    }

    static void MouseFunc(int button, int state, int x, int y) {
        if (button == GLUT_LEFT_BUTTON) {
            if (state == GLUT_DOWN) {
                prev_mousex = x;
                prev_mousey = y;
            }
        }
        if (button == GLUT_RIGHT_BUTTON) {
            if (state == GLUT_DOWN) {
                int test_x = x;
                int test_y = winhei - y;
                
                fmat4 inv_proj = camera.ViewProjectionMatrix().Inverse();
                fvec3 tmp((test_x + 0.5f) / winwid * 2 - 1, (test_y + 0.5f) / winhei * 2 - 1, -1);
                fvec3 origin =  inv_proj.Transform(tmp);
                tmp.z = 0;
                fvec3 direction = inv_proj.Transform(tmp) - origin;
                direction.NormalizeMe();
                DebugRayCast(*cache_manager->data()->ChannelByName("node links")->cl_buffer(), cache_manager->root_node_index(), origin, direction);
            }
        }
    }

    static void MouseMotionFunc(int x, int y) {
        camera.set_yaw(camera.yaw() + (x - prev_mousex) * looking_speed);
        camera.set_pitch(camera.pitch() - (y - prev_mousey) * looking_speed);
        
        prev_mousex = x;
        prev_mousey = y;
    }

    static void OctreeReadWriteTest() {
        WriteTestOctreeSphere          (fvec3(0.3, 0.4, 0.65), 0.23, 11, "sphere.tree", 737);
        cout << "written" << endl;
        CheckTestOctreeSphereWithLoader(fvec3(0.3, 0.4, 0.65), 0.23, 11, "sphere.tree");
        cout << "checked" << endl;
    }
    
    static void OctreeGPUTest() {
        WriteTestOctreeSphere(fvec3(0.3, 0.4, 0.65), 0.23, 3, "sphere.tree", 131);
        cout << "written" << endl;
        shared_ptr<StoredOctreeLoader> loader(new StoredOctreeLoader("sphere.tree"));
        GPUOctreeCacheManager cache_manager(loader->header().blocks_count, loader, context);
        CheckTestOctreeSphereWithGPUData(fvec3(0.3, 0.4, 0.65), 0.23, 3, cache_manager.data(), cache_manager.root_node_index(), true);
        cout << "checked" << endl;
    }
    
    void RunTestApplication(int argc, char **argv) {
        if (!BinaryUtil::CheckEndianness())
            crash("wrong endianness");
        
		//ImportObjScene(15, 1000, "/Users/me/codin/raytracer/scenes/hairball.obj", "/Users/me/codin/raytracer/scenes/hairball15.tree", true);
		//ImportObjScene(2, 10, "H:/rayt scenes/hairball.obj", "H:/rayt scenes/hairball2.tree", true);
		//WriteTestOctreeSphere(fvec3(.5F, .5F, .5F), .45F, 7, "H:/rayt scenes/sphere7.tree", 100);
        //return;

		//DebugCheckConstantDepthTree("H:/rayt scenes/hairball9.tree", 9);
		//CheckTestOctreeSphereWithLoader(fvec3(.5F, .5F, .5F), .45F, 7, "H:/rayt scenes/sphere7.tree");
		//cout << "passed" << endl;
		//return;
        
        glutInit(&argc, argv);
        
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
        glutInitWindowSize(winwid, winhei);
        main_window = glutCreateWindow("upchk");
        glutDisplayFunc(DisplayFunc);
        glutIdleFunc(IdleFunc);
        glutMouseFunc(MouseFunc);
        glutMotionFunc(MouseMotionFunc);
        glutKeyboardFunc(KeyboardFunc);
        glutKeyboardUpFunc(KeyboardUpFunc);
        
        if (glewInit() != GLEW_OK)
            cout << "GLEW is not initialized!" << endl;
        
        context = shared_ptr<CLContext>(new CLContext());
        
		//WriteTestOctreeSphere(fvec3(.75, .75, .25), 3./16, 6, "sphere_small.tree", 8);
        //WriteTestOctreeSphere(fvec3(.5, .5, .5), 0.3, 9, "/Users/me/codin/raytracer/scenes/sphere9.tree", 10000);
		//WriteTestOctreeSphere(fvec3(.5, .5, .5), 0.3, 9, "H:/rayt scenes/sphere9.tree", 10000);
		
        camera.set_field_of_view(60);
        
        camera.set_yaw(-54.1999893);
        camera.set_pitch(6.00000143);
        camera.set_postion(fvec3(2.36080551, 0.5, 1.63407874));
        
        //camera.set_postion(fvec3(0.5, 0.5f, -2.0f));
        //camera.set_yaw(180);
        
        camera.set_aspect_ratio(1);
        
        loader = shared_ptr<StoredOctreeLoader>(new StoredOctreeLoader("H:/rayt scenes/sphere7.tree"));
		//loader = shared_ptr<StoredOctreeLoader>(new StoredOctreeLoader("/Users/me/codin/raytracer/scenes/hairball11.tree"));
        cache_manager = shared_ptr<GPUOctreeCacheManager>(new GPUOctreeCacheManager(loader->header().blocks_count, loader, context));

		CheckTestOctreeSphereWithGPUData(fvec3(.5F, .5F, .5F), .45F, 7, cache_manager->data(), cache_manager->root_node_index(), true);
		cout << "passed" << endl;
		return;
        
		raytracer.reset(new GPURayTracer(cache_manager, imgwid, imghei, context));
        //raytracer->set_lod_voxel_size(2);
		raytracer->set_lod_voxel_size(1.5);
        drawer.reset(new RenderedImageDrawer(imgwid, imghei, context));
        
        glutMainLoop();
    }

}
