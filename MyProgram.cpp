//===========================================================================
/*
    CS277 - Experimental Haptics
    Winter 2010, Stanford University

    You may use this program as a boilerplate for starting your homework
    assignments.  Use CMake (www.cmake.org) on the CMakeLists.txt file to
    generate project files for the development tool of your choice.  The
    CHAI3D library directory (chai3d-2.1.0) should be installed as a sibling
    directory to the one containing this project.

    These files are meant to be helpful should you encounter difficulties
    setting up a working CHAI3D project.  However, you are not required to
    use them for your homework -- you may start from anywhere you'd like.

    \author    Francois Conti & Sonny Chan
    \date      January 2010
*/
//===========================================================================

//---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//---------------------------------------------------------------------------
#include "chai3d.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// DECLARED CONSTANTS
//---------------------------------------------------------------------------

// initial size (width/height) in pixels of the display window
const int WINDOW_SIZE_W         = 600;
const int WINDOW_SIZE_H         = 600;

// mouse menu options (right button)
const int OPTION_FULLSCREEN     = 1;
const int OPTION_WINDOWDISPLAY  = 2;

// maximum number of haptic devices supported in this demo
const int MAX_DEVICES           = 8;


//---------------------------------------------------------------------------
// DECLARED VARIABLES
//---------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera that renders the world in a window display
cCamera* camera;

// a light source to illuminate the objects in the virtual scene
cLight *light;

// width and height of the current window display
int displayW  = 0;
int displayH  = 0;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the first haptic device detected on this computer
cGenericHapticDevice* hapticDevice = 0;

// a 3D cursor for the haptic device
cShapeSphere* cursor = 0;

// a line to display velocity of the haptic interface
cShapeLine* velocityVector;

// labels to show haptic device position and update rate
cLabel* positionLabel;
cLabel* rateLabel;
double rateEstimate = 0;

// material properties used to render the color of the cursors
cMaterial matCursorButtonON;
cMaterial matCursorButtonOFF;

// status of the main simulation haptics loop
bool simulationRunning = false;

// has exited haptics simulation thread
bool simulationFinished = false;

cMesh* object;
int vertices[6][4];
cTexture2D* texture;
cGeneric3dofPointer* tool;

// radius of the tool proxy
double proxyRadius;

//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

// callback when the window display is resized
void resizeWindow(int w, int h);

// callback when a keyboard key is pressed
void keySelect(unsigned char key, int x, int y);

// callback when the right mouse button is pressed to select a menu item
void menuSelect(int value);

// function called before exiting the application
void close(void);

// main graphics callback
void updateGraphics(void);

// main haptics loop
void updateHaptics(void);


//===========================================================================
/*
    This application illustrates the use of the haptic device handler
    "cHapticDevicehandler" to access haptic devices connected to the computer.

    In this example the application opens an OpenGL window and displays a
    3D cursor for the first device found. If the operator presses the device
    user button, the color of the cursor changes accordingly.

    In the main haptics loop function  "updateHaptics()" , the position and 
    user switch status of the device are retrieved at each simulation iteration.
    This information is then used to update the position and color of the
    cursor. A force is then commanded to the haptic device to attract the 
    end-effector towards the device origin.
*/
//===========================================================================

int main(int argc, char* argv[])
{
    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------

    printf ("\n");
    printf ("-----------------------------------\n");
    printf ("DH2660 - Haptics\n");
    printf ("April 2014, KTH\n");
    printf ("-----------------------------------\n");
    printf ("\n\n");

    //-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    // the color is defined by its (R,G,B) components.
    world->setBackgroundColor(0,1, 0);

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and oriente the camera
    camera->set( cVector3d (0.2, 0.0, 0.0),    // camera position (eye)
                 cVector3d (0.0, 0.0, 0.0),    // lookat position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the "up" vector

    // set the near and far clipping planes of the camera
    // anything in front/behind these clipping planes will not be rendered
    camera->setClippingPlanes(0.01, 10.0);

    // create a light source and attach it to the camera
    light = new cLight(world);
    camera->addChild(light);                   // attach light to camera
    light->setEnabled(true);                   // enable light source
    light->setPos(cVector3d( 2.0, 0.5, 1.0));  // position the light source
    light->setDir(cVector3d(-2.0, 0.5, 1.0));  // define the direction of the light beam

    // create a label that shows the haptic loop update rate
    rateLabel = new cLabel();
    rateLabel->setPos(8, 24, 0);
    camera->m_front_2Dscene.addChild(rateLabel);





    //-----------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //-----------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // read the number of haptic devices currently connected to the computer
    int numHapticDevices = handler->getNumDevices();

    // if there is at least one haptic device detected...
    cHapticDeviceInfo info;

    if (numHapticDevices)
    {
        // get a handle to the first haptic device
        handler->getDevice(hapticDevice);

        // open connection to haptic device
        hapticDevice->open();

		// initialize haptic device
		hapticDevice->initialize();

        // retrieve information about the current haptic device
        info = hapticDevice->getSpecifications();

        // create a cursor with its radius set
        cursor = new cShapeSphere(0.01);

        // add cursor to the world
        world->addChild(cursor);

        // create a small line to illustrate velocity
        velocityVector = new cShapeLine(cVector3d(0,0,0), cVector3d(0,0,0));

        // add line to the world
        world->addChild(velocityVector);
	cShapeLine *rightLine = new cShapeLine(cVector3d(0, 0.02, 1),cVector3d(0, 0.02, -1));
	cShapeLine *leftLine = new cShapeLine(cVector3d(0, -0.02, 1),cVector3d(0, -0.02, -1));
	cShapeLine *topLine = new cShapeLine(cVector3d(0, -1, 0.02),cVector3d(0, 1, 0.02));
	cShapeLine *bottomLine = new cShapeLine(cVector3d(0, -1, -0.02),cVector3d(0, 1, -0.02));
        world->addChild(rightLine);
        world->addChild(leftLine);
        world->addChild(topLine);
        world->addChild(bottomLine);
        positionLabel = new cLabel();
        positionLabel->setPos(8, 8, 0);
        camera->m_front_2Dscene.addChild(positionLabel);

	// connect the haptic device to the tool
	tool->setHapticDevice(hapticDevice);

	// initialize tool by connecting to haptic device
	tool->start();

	// map the physical workspace of the haptic device to a larger virtual workspace.
	tool->setWorkspaceRadius(1.0);

	// define a radius for the tool (graphical display)
	tool->setRadius(0.05);

	// hide the device sphere. only show proxy.
	tool->m_deviceSphere->setShowEnabled(false);

	// set the physical readius of the proxy.
	proxyRadius = 0.05;
	tool->m_proxyPointForceModel->setProxyRadius(proxyRadius);
	tool->m_proxyPointForceModel->m_collisionSettings.m_checkBothSidesOfTriangles = false;

	// enable if objects in the scene are going to rotate of translate
	// or possibly collide against the tool. If the environment
	// is entirely static, you can set this parameter to "false"
	tool->m_proxyPointForceModel->m_useDynamicProxy = true;

	// read the scale factor between the physical workspace of the haptic
	// device and the virtual workspace defined for the tool
	double workspaceScaleFactor = tool->getWorkspaceScaleFactor();

	// define a maximum stiffness that can be handled by the current
	// haptic device. The value is scaled to take into account the
	// workspace scale factor
	double stiffnessMax = info.m_maxForceStiffness / workspaceScaleFactor;


	/*
	  ODEWorld = new cODEWorld(world);
	  world->addChild(ODEWorld);
	  ODEWorld->setGravity(cVector3d(0.0, 0.0, -9.81));

	  ODEBody0 = new cODEGenericBody(ODEWorld);
	  // Cube

	  cMaterial mat0;
	  cMesh* object0 = new cMesh(world);
	  double boxSize = 0.1;
	  double stiffnessMax = 100.0;
	  createCube(object0, boxSize);
	  mat0.m_ambient.set(0.8, 0.1, 0.4);
	  mat0.m_diffuse.set(1.0, 0.15, 0.5);
	  mat0.m_specular.set(1.0, 0.2, 0.8);
	  mat0.setStiffness(0.5 * stiffnessMax);
	  mat0.setDynamicFriction(0.8);
	  mat0.setStaticFriction(0.8);
	  object0->setMaterial(mat0);

	  ODEBody0->setImageModel(object0);
	  ODEBody0->createDynamicBox(boxSize, boxSize, boxSize);
	  ODEBody0->setMass(0.05);

	  ODEBody0->setPosition(cVector3d(0, 0, 0)); */

	// create a virtual mesh
	object = new cMesh(world);

	// add object to world
	world->addChild(object);

	// set the position of the object at the center of the world
	object->setPos(0.0, 0.0, 0.0);

	/////////////////////////////////////////////////////////////////////////
	// create a cube
	/////////////////////////////////////////////////////////////////////////
	const double HALFSIZE = 0.08;

	// face -x
	vertices[0][0] = object->newVertex(-HALFSIZE,  HALFSIZE, -HALFSIZE); 
	vertices[0][1] = object->newVertex(-HALFSIZE, -HALFSIZE, -HALFSIZE);
	vertices[0][2] = object->newVertex(-HALFSIZE, -HALFSIZE,  HALFSIZE);
	vertices[0][3] = object->newVertex(-HALFSIZE,  HALFSIZE,  HALFSIZE);

	// face +x
	vertices[1][0] = object->newVertex( HALFSIZE, -HALFSIZE, -HALFSIZE);
	vertices[1][1] = object->newVertex( HALFSIZE,  HALFSIZE, -HALFSIZE);
	vertices[1][2] = object->newVertex( HALFSIZE,  HALFSIZE,  HALFSIZE);
	vertices[1][3] = object->newVertex( HALFSIZE, -HALFSIZE,  HALFSIZE);

	// face -y
	vertices[2][0] = object->newVertex(-HALFSIZE,  -HALFSIZE, -HALFSIZE);
	vertices[2][1] = object->newVertex( HALFSIZE,  -HALFSIZE, -HALFSIZE);
	vertices[2][2] = object->newVertex( HALFSIZE,  -HALFSIZE,  HALFSIZE);
	vertices[2][3] = object->newVertex(-HALFSIZE,  -HALFSIZE,  HALFSIZE);

	// face +y
	vertices[3][0] = object->newVertex( HALFSIZE,   HALFSIZE, -HALFSIZE);
	vertices[3][1] = object->newVertex(-HALFSIZE,   HALFSIZE, -HALFSIZE);
	vertices[3][2] = object->newVertex(-HALFSIZE,   HALFSIZE,  HALFSIZE);
	vertices[3][3] = object->newVertex( HALFSIZE,   HALFSIZE,  HALFSIZE);

	// face -z
	vertices[4][0] = object->newVertex(-HALFSIZE,  -HALFSIZE, -HALFSIZE);
	vertices[4][1] = object->newVertex(-HALFSIZE,   HALFSIZE, -HALFSIZE);
	vertices[4][2] = object->newVertex( HALFSIZE,   HALFSIZE, -HALFSIZE);
	vertices[4][3] = object->newVertex( HALFSIZE,  -HALFSIZE, -HALFSIZE);

	// face +z
	vertices[5][0] = object->newVertex( HALFSIZE,  -HALFSIZE,  HALFSIZE);
	vertices[5][1] = object->newVertex( HALFSIZE,   HALFSIZE,  HALFSIZE);
	vertices[5][2] = object->newVertex(-HALFSIZE,   HALFSIZE,  HALFSIZE);
	vertices[5][3] = object->newVertex(-HALFSIZE,  -HALFSIZE,  HALFSIZE);

	// create a texture
	texture = new cTexture2D();
	object->setTexture(texture);
	object->setUseTexture(true);

	// set material properties to light gray
	object->m_material.m_ambient.set(0.5f, 0.5f, 0.5f, 1.0f);
	object->m_material.m_diffuse.set(0.7f, 0.7f, 0.7f, 1.0f);
	object->m_material.m_specular.set(1.0f, 1.0f, 1.0f, 1.0f);
	object->m_material.m_emission.set(0.0f, 0.0f, 0.0f, 1.0f);

	// compute normals
	object->computeAllNormals();

	// display triangle normals
	object->setShowNormals(true);

	// set length and color of normals
	object->setNormalsProperties(0.1, cColorf(1.0, 0.0, 0.0), true);

	// compute a boundary box
	object->computeBoundaryBox(true);

	// get dimensions of object
	double size = cSub(object->getBoundaryMax(), object->getBoundaryMin()).length();

	// resize object to screen
	object->scale( 2.0 * tool->getWorkspaceRadius() / size);

	// compute collision detection algorithm
	object->createAABBCollisionDetector(1.01 * proxyRadius, true, false);

	// define a default stiffness for the object
	object->setStiffness(stiffnessMax, true);

	// define friction properties
	object->setFriction(0.2, 0.5, true);

    }
    // here we define the material properties of the cursor when the
    // user button of the device end-effector is engaged (ON) or released (OFF)

    // a light orange material color
    matCursorButtonOFF.m_ambient.set(0.5, 0.2, 0.0);
    matCursorButtonOFF.m_diffuse.set(1.0, 0.5, 0.0);
    matCursorButtonOFF.m_specular.set(1.0, 1.0, 1.0);

    // a blue material color
    matCursorButtonON.m_ambient.set(0.1, 0.1, 0.4);
    matCursorButtonON.m_diffuse.set(0.3, 0.3, 0.8);
    matCursorButtonON.m_specular.set(1.0, 1.0, 1.0);


    //-----------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //-----------------------------------------------------------------------

    // initialize GLUT
    glutInit(&argc, argv);

    // retrieve the resolution of the computer display and estimate the position
    // of the GLUT window so that it is located at the center of the screen
    int screenW = glutGet(GLUT_SCREEN_WIDTH);
    int screenH = glutGet(GLUT_SCREEN_HEIGHT);
    int windowPosX = (screenW - WINDOW_SIZE_W) / 2;
    int windowPosY = (screenH - WINDOW_SIZE_H) / 2;

    // initialize the OpenGL GLUT window
    glutInitWindowPosition(windowPosX, windowPosY);
    glutInitWindowSize(WINDOW_SIZE_W, WINDOW_SIZE_H);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);
    glutDisplayFunc(updateGraphics);
    glutKeyboardFunc(keySelect);
    glutReshapeFunc(resizeWindow);
    glutSetWindowTitle("CHAI 3D");

    // create a mouse menu (right button)
    glutCreateMenu(menuSelect);
    glutAddMenuEntry("full screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("window display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);


    //-----------------------------------------------------------------------
    // START SIMULATION
    //-----------------------------------------------------------------------

    // simulation in now running
    simulationRunning = true;

    // create a thread which starts the main haptics rendering loop
    cThread* hapticsThread = new cThread();
    hapticsThread->set(updateHaptics, CHAI_THREAD_PRIORITY_HAPTICS);

    // start the main graphics rendering loop
    glutMainLoop();

    // close everything
    close();

    // exit
    return (0);
}

//---------------------------------------------------------------------------

void resizeWindow(int w, int h)
{
    // update the size of the viewport
    displayW = w;
    displayH = h;
    glViewport(0, 0, displayW, displayH);
}

//---------------------------------------------------------------------------

void keySelect(unsigned char key, int x, int y)
{
    // escape key
    if ((key == 27) || (key == 'x'))
    {
        // close everything
        close();

        // exit application
        exit(0);
    }
}

//---------------------------------------------------------------------------

void menuSelect(int value)
{
    switch (value)
    {
        // enable full screen display
        case OPTION_FULLSCREEN:
            glutFullScreen();
            break;

        // reshape window to original size
        case OPTION_WINDOWDISPLAY:
            glutReshapeWindow(WINDOW_SIZE_W, WINDOW_SIZE_H);
            break;
    }
}

//---------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close the haptic devices
    if (hapticDevice)
    {
        hapticDevice->close();
    }
    tool->stop();
}

//---------------------------------------------------------------------------

void updateGraphics(void)
{
    // update the label showing the position of the haptic device
    if (cursor)
    {
        cVector3d position = cursor->getPos() * 1000.0; // convert to mm
        char buffer[128];
        sprintf(buffer, "device position: (%.2lf, %.2lf, %.2lf) mm",
            position.x, position.y, position.z);
        positionLabel->m_string = buffer;
    }

    // update the label with the haptic refresh rate
    char buffer[128];
    sprintf(buffer, "haptic rate: %.0lf Hz", rateEstimate);
    rateLabel->m_string = buffer;

    // render world
    camera->renderView(displayW, displayH);

    // Swap buffers
    glutSwapBuffers();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    // inform the GLUT window to call updateGraphics again (next frame)
    if (simulationRunning)
    {
        glutPostRedisplay();
    }
}

//---------------------------------------------------------------------------

void updateHaptics(void)
{
    // a clock to estimate the haptic simulation loop update rate
    cPrecisionClock pclock;
    pclock.setTimeoutPeriodSeconds(1.0);
    pclock.start(true);
    cPrecisionClock clock;
    clock.start(true);
    int counter = 0;

    // main haptic simulation loop
    while(simulationRunning)
    {
		if (!hapticDevice) continue;
		
        // read position of haptic device
        cVector3d newPosition;
        hapticDevice->getPosition(newPosition);

        // update position and orientation of cursor
        cursor->setPos(newPosition);

        // read linear velocity from device
        cVector3d linearVelocity;
        hapticDevice->getLinearVelocity(linearVelocity);

        // update the line showing velocity
        velocityVector->m_pointA = newPosition;
        velocityVector->m_pointB = newPosition + linearVelocity;

        // read user button status
        bool buttonStatus;
        hapticDevice->getUserSwitch(0, buttonStatus);

        // adjust the color of the cursor according to the status of
        // the user switch (ON = TRUE / OFF = FALSE)
	//        cursor->m_material = buttonStatus ? matCursorButtonON : matCursorButtonOFF;
        cursor->m_material = newPosition[1] > 0.02 ? matCursorButtonON : matCursorButtonOFF;

        // compute a reaction force (a spring that pulls the device to the origin)
	const double stiffness = 100.0; // [N/m]
        cVector3d force = cVector3d(0, 0, 0);
	const double WALL_FORCE = 10000.0;

	if(newPosition.y > 0.02) {
	  force.y = -WALL_FORCE * (newPosition.y -0.02);
	}
	if(newPosition.y < -0.02) {
	  force.y = -WALL_FORCE * (newPosition.y +0.02);
	}
	if(newPosition.z > 0.02) {
	  force.z = -WALL_FORCE * (newPosition.z -0.02);
	}
	if(newPosition.z < -0.02) {
	  force.z = -WALL_FORCE * (newPosition.z + 0.02);
	}


        // send computed force to haptic device
        hapticDevice->setForce(force);

        // estimate the refresh rate
        ++counter;
        if (pclock.timeoutOccurred()) {
            pclock.stop();
            rateEstimate = counter;
            counter = 0;
            pclock.start(true);
        }
    }
    
    // exit haptics thread
    simulationFinished = true;
}
void createCube(cMesh* a_mesh, double a_size)
{
    const double HALFSIZE = a_size / 2.0;
    int vertices [6][6];

    // face -x
    vertices[0][0] = a_mesh->newVertex(-HALFSIZE,  HALFSIZE, -HALFSIZE);
    vertices[0][1] = a_mesh->newVertex(-HALFSIZE, -HALFSIZE, -HALFSIZE);
    vertices[0][2] = a_mesh->newVertex(-HALFSIZE, -HALFSIZE,  HALFSIZE);
    vertices[0][3] = a_mesh->newVertex(-HALFSIZE,  HALFSIZE,  HALFSIZE);

    // face +x
    vertices[1][0] = a_mesh->newVertex( HALFSIZE, -HALFSIZE, -HALFSIZE);
    vertices[1][1] = a_mesh->newVertex( HALFSIZE,  HALFSIZE, -HALFSIZE);
    vertices[1][2] = a_mesh->newVertex( HALFSIZE,  HALFSIZE,  HALFSIZE);
    vertices[1][3] = a_mesh->newVertex( HALFSIZE, -HALFSIZE,  HALFSIZE);

    // face -y
    vertices[2][0] = a_mesh->newVertex(-HALFSIZE,  -HALFSIZE, -HALFSIZE);
    vertices[2][1] = a_mesh->newVertex( HALFSIZE,  -HALFSIZE, -HALFSIZE);
    vertices[2][2] = a_mesh->newVertex( HALFSIZE,  -HALFSIZE,  HALFSIZE);
    vertices[2][3] = a_mesh->newVertex(-HALFSIZE,  -HALFSIZE,  HALFSIZE);

    // face +y
    vertices[3][0] = a_mesh->newVertex( HALFSIZE,   HALFSIZE, -HALFSIZE);
    vertices[3][1] = a_mesh->newVertex(-HALFSIZE,   HALFSIZE, -HALFSIZE);
    vertices[3][2] = a_mesh->newVertex(-HALFSIZE,   HALFSIZE,  HALFSIZE);
    vertices[3][3] = a_mesh->newVertex( HALFSIZE,   HALFSIZE,  HALFSIZE);

    // face -z
    vertices[4][0] = a_mesh->newVertex(-HALFSIZE,  -HALFSIZE, -HALFSIZE);
    vertices[4][1] = a_mesh->newVertex(-HALFSIZE,   HALFSIZE, -HALFSIZE);
    vertices[4][2] = a_mesh->newVertex( HALFSIZE,   HALFSIZE, -HALFSIZE);
    vertices[4][3] = a_mesh->newVertex( HALFSIZE,  -HALFSIZE, -HALFSIZE);

    // face +z
    vertices[5][0] = a_mesh->newVertex( HALFSIZE,  -HALFSIZE,  HALFSIZE);
    vertices[5][1] = a_mesh->newVertex( HALFSIZE,   HALFSIZE,  HALFSIZE);
    vertices[5][2] = a_mesh->newVertex(-HALFSIZE,   HALFSIZE,  HALFSIZE);
    vertices[5][3] = a_mesh->newVertex(-HALFSIZE,  -HALFSIZE,  HALFSIZE);

    // create triangles
    for (int i=0; i<6; i++)
    {
    a_mesh->newTriangle(vertices[i][0], vertices[i][1], vertices[i][2]);
    a_mesh->newTriangle(vertices[i][0], vertices[i][2], vertices[i][3]);
    }

    // set material properties to light gray
    a_mesh->m_material.m_ambient.set(0.5f, 0.5f, 0.5f, 1.0f);
    a_mesh->m_material.m_diffuse.set(0.7f, 0.7f, 0.7f, 1.0f);
    a_mesh->m_material.m_specular.set(1.0f, 1.0f, 1.0f, 1.0f);
    a_mesh->m_material.m_emission.set(0.0f, 0.0f, 0.0f, 1.0f);

    // compute normals
    a_mesh->computeAllNormals();

    // compute collision detection algorithm
    a_mesh->createAABBCollisionDetector(1.01 * proxyRadius, true, false);
}
 
//---------------------------------------------------------------------------

