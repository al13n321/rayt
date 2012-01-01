#include "test-application.h"
#include "scene.h"
#include "test-stopwatch.h"
#include "cpu-rendered-image-drawer.h"
#include "debug-ray-cast.h"
#include "blocking-experiment.h"
#include "compact-octree-statistics.h"
#include <cstdio>
#include <iostream>
using namespace rayt;
using namespace std;

namespace rayt {

const int winhei = 500;
const int winwid = 500;
const int imgwid = 200;
const int imghei = 200;
int main_window;

int prev_mousex;
int prev_mousey;
bool key_pressed[256];
TestStopwatch frame_stopwatch;
RenderStatistics statistics;

const float movement_speed = 3; // units per second
const float looking_speed = 0.1; // degrees per pixel

CPURenderedImageDrawer *drawer;
Scene *scene;
Camera *camera;

TestStopwatch fps_stopwatch;
const int fps_update_period = 20;
int cur_frame_number;

static void DisplayFunc() {
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
	drawer->Draw();
	glutSwapBuffers();
}

static void IdleFunc() {
	scene->RenderOnCPU(drawer->render_buffer(), *camera, &statistics);
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
	camera->MoveRelative(movement * frame_time * movement_speed);

	if (cur_frame_number % fps_update_period == 0) {
		char fps_str[100];
		double fps = 1.0 / fps_stopwatch.Restart() * fps_update_period;
		sprintf(fps_str, "%lf", fps);
		string title = string("FPS: ") + fps_str + statistics.FormatShortReport();
		statistics.Reset();

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
}

static void MouseMotionFunc(int x, int y) {
	camera->set_yaw(camera->yaw() + (x - prev_mousex) * looking_speed);
	camera->set_pitch(camera->pitch() - (y - prev_mousey) * looking_speed);
	
	prev_mousex = x;
	prev_mousey = y;
}
    
static void WriteSomeOctreeStatistics() {
    Scene *scene = new Scene(16);
    
    scene->AddSphere(fvec3(4, 4, -4), 3, Color(255, 0, 0), 8);
    
    WriteCompactOctreeStatistics(scene->nodes_pool_.root_node());
    
    delete scene;
    
    scene = new Scene(16);
    
    scene->AddSphere(fvec3(4, 4, -4), 3, Color(255, 0, 0), 10);
    
    WriteCompactOctreeStatistics(scene->nodes_pool_.root_node());
    
    delete scene;
    
    scene = new Scene(16);
    
    scene->AddSphere(fvec3(4, 4, -4), 3, Color(255, 0, 0), 6);
    
    WriteCompactOctreeStatistics(scene->nodes_pool_.root_node());
    
    delete scene;
}
    
static void WriteSomeBlockingStatistics() {
    Scene *scene = new Scene(16);
    
    scene->AddSphere(fvec3(4, 4, -4), 3, Color(255, 0, 0), 8);

    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 54, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 54, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 90, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 90, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 36, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 36, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 18, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 18, true);
    
    delete scene;
    
    scene = new Scene(16);
    
    scene->AddSphere(fvec3(4, 4, -4), 3, Color(255, 0, 0), 10);
    
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 54, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 54, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 90, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 90, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 36, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 36, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 18, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 18, true);
    
    delete scene;
    
    scene = new Scene(16);
    
    scene->AddSphere(fvec3(4, 4, -4), 3, Color(255, 0, 0), 6);
    
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 54, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 54, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 90, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 90, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 36, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 36, true);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 18, false);
    WriteBlockingStatistics(scene->nodes_pool_.root_node(), 18, true);
    
    delete scene;
    
    return;
}

void RunTestApplication(int argc, char **argv) {
    //WriteSomeOctreeStatistics(); return;
    WriteSomeBlockingStatistics(); return;
    
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
    
    drawer = new CPURenderedImageDrawer(imgwid, imghei);
	camera = new Camera();
	scene = new Scene(16);

	scene->AddSphere(fvec3(4, 4, -4), 3, Color(255, 0, 0), 8);

	/*fvec3 origin = fvec3(0.10797191, 0.26248005, -0.11126341);
	fvec3 dir = fvec3(0.013718240, 0.88270319, -0.46973050);
	OctreeRayIntersection intersection;
	DebugRayFirstIntersection(scene->nodes_pool_.root_node(), origin, dir, &intersection);*/

	//return;

	LightSource light;
	light.color = Color(255, 255, 255);
	light.position = fvec3(2, 7, 10);
	light.create_shadow = true;
	light.intensity = 1;
	light.type = kLightSourcePoint;
	scene->AddLightSource(light);

	camera->set_field_of_view(60);
	camera->set_postion(fvec3(-4, -4, 4));
	camera->set_yaw(45);
	camera->set_pitch(45);
	camera->set_aspect_ratio(1);

	glutMainLoop();
}

}