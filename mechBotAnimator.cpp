#include <stdio.h>
#include <windows.h>
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include "mechBotAnimator.h"
#include "subdivcurve.h"

enum BotType { CUBE, SPHERE, WHEEL };
BotType botType = WHEEL;

int numCirclePoints = 30;
double circleRadius = 0.2;
int hoveredCircle = -1;
int curveIndex = 0;
int currentCurvePoint = 0;
float angle = 0.0;
float nonConvertedAngle = 0.0;
int animate = 0;
int delay = 15; // milliseconds

float cubeSize = 1.0;
float sphereRadius = 1.0;
float wheelRadius = 1.0;
float wheelThickness = 1.0;
float cannonPosition = -5;
float wheelRotation = 0.0;

GLdouble worldLeft = -12;
GLdouble worldRight = 12;
GLdouble worldBottom = -9;
GLdouble worldTop = 9;
GLdouble worldCenterX = 0.0;
GLdouble worldCenterY = 0.0;
GLdouble wvLeft = -12;
GLdouble wvRight = 12;
GLdouble wvBottom = -9;
GLdouble wvTop = 9;

GLint glutWindowWidth = 800;
GLint glutWindowHeight = 600;
GLint viewportWidth = glutWindowWidth;
GLint viewportHeight = glutWindowHeight;

// Ground Mesh material
GLfloat groundMat_ambient[] = { 0.4, 0.4, 0.4, 1.0 };
GLfloat groundMat_specular[] = { 0.01, 0.01, 0.01, 1.0 };
GLfloat groundMat_diffuse[] = { 0.4, 0.4, 0.7, 1.0 };
GLfloat groundMat_shininess[] = { 1.0 };

GLfloat light_position0[] = { 4.0, 8.0, 8.0, 1.0 };
GLfloat light_diffuse0[] = { 1.0, 1.0, 1.0, 1.0 };

GLfloat light_position1[] = { -4.0, 8.0, 8.0, 1.0 };
GLfloat light_diffuse1[] = { 1.0, 1.0, 1.0, 1.0 };

GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat model_ambient[] = { 0.5, 0.5, 0.5, 1.0 };

// 
GLdouble spin = 0.0;

// The 2D animation path curve is a subdivision curve
SubdivisionCurve subcurve;

// Use circles to **draw** (i.e. visualize) subdivision curve control points
Circle circles[MAXCONTROLPOINTS];

int lastMouseX;
int lastMouseY;
int window2D, window3D;
int window3DSizeX = 800, window3DSizeY = 600;
GLdouble aspect = (GLdouble)window3DSizeX / window3DSizeY;
GLdouble eyeX = 0.0, eyeY = 6.0, eyeZ = 22.0;
GLdouble zNear = 0.1, zFar = 40.0;
GLdouble fov = 60.0;

int main(int argc, char* argv[])
{
	glutInit(&argc, (char**)argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(glutWindowWidth, glutWindowHeight);
	glutInitWindowPosition(50, 100);

	// The 2D Window
	window2D = glutCreateWindow("Animation Path Designer");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	// Initialize the 2D profile curve system
	init2DCurveWindow();
	// A few input handlers
	glutMouseFunc(mouseButtonHandler);
	glutMotionFunc(mouseMotionHandler);
	glutPassiveMotionFunc(mouseHoverHandler);
	glutKeyboardFunc(keyboardHandler);
	glutSpecialFunc(specialKeyHandler);



	// The 3D Window
	window3D = glutCreateWindow("Mech Bot");
	glutPositionWindow(900, 100);
	glutDisplayFunc(display3D);
	glutReshapeFunc(reshape3D);
	glutMouseFunc(mouseButtonHandler3D);
	glutMotionFunc(mouseMotionHandler3D);

	// Initialize the 3D system
	init3DSurfaceWindow();

	// Annnd... ACTION!!
	glutMainLoop();

	return 0;
}

void init2DCurveWindow()
{
	glLineWidth(3.0);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glClearColor(0.4F, 0.4F, 0.4F, 0.0F);
	initSubdivisionCurve();
	initControlPoints();
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(wvLeft, wvRight, wvBottom, wvTop);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	draw2DScene();
	glutSwapBuffers();
}


void draw2DScene()
{
	drawAxes();
	drawSubdivisionCurve();
	drawControlPoints();
}

void drawAxes()
{
	glPushMatrix();
	glColor3f(1.0, 0.0, 0);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0, 8.0, 0);
	glVertex3f(0, -8.0, 0);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(-8, 0.0, 0);
	glVertex3f(8, 0.0, 0);
	glEnd();
	glPopMatrix();
}

void drawSubdivisionCurve() {
	// Subdivide the given curve
	computeSubdivisionCurve(&subcurve);

	int i = 0;

	glColor3f(0.0, 1.0, 0.0);
	glPushMatrix();
	glBegin(GL_LINE_STRIP);
	for (i = 0; i < subcurve.numCurvePoints; i++) {
		glVertex3f(subcurve.curvePoints[i].x, subcurve.curvePoints[i].y, 0.0);
	}
	glEnd();
	glPopMatrix();
}

void drawControlPoints() {
	int i, j;
	for (i = 0; i < subcurve.numControlPoints; i++) {
		glPushMatrix();
		glColor3f(1.0f, 0.0f, 0.0f);
		glTranslatef(circles[i].circleCenter.x, circles[i].circleCenter.y, 0);
		// for the hoveredCircle, draw an outline and change its colour
		if (i == hoveredCircle) {
			// outline
			glColor3f(0.0, 1.0, 0.0);
			glBegin(GL_LINE_LOOP);
			for (j = 0; j < numCirclePoints; j++) {
				glVertex3f(circles[i].circlePoints[j].x, circles[i].circlePoints[j].y, 0);
			}
			glEnd();
			// colour change
			glColor3f(0.5, 0.0, 1.0);
		}
		glBegin(GL_LINE_LOOP);
		for (j = 0; j < numCirclePoints; j++) {
			glVertex3f(circles[i].circlePoints[j].x, circles[i].circlePoints[j].y, 0);
		}
		glEnd();
		glPopMatrix();
	}
}

void initSubdivisionCurve() {
	// Initialize 3 control points of the subdivision curve

	GLdouble x, y;

	x = 4 * cos(M_PI * 0.5);
	y = 4 * sin(M_PI * 0.5);
	subcurve.controlPoints[0].x = x;
	subcurve.controlPoints[0].y = y;

	x = 4 * cos(M_PI * 0.25);
	y = 4 * sin(M_PI * 0.25);
	subcurve.controlPoints[1].x = x;
	subcurve.controlPoints[1].y = y;

	x = 4 * cos(M_PI * 0.0);
	y = 4 * sin(M_PI * 0.0);
	subcurve.controlPoints[2].x = x;
	subcurve.controlPoints[2].y = y;

	x = 4 * cos(-M_PI * 0.25);
	y = 4 * sin(-M_PI * 0.25);
	subcurve.controlPoints[3].x = x;
	subcurve.controlPoints[3].y = y;

	x = 4 * cos(-M_PI * 0.5);
	y = 4 * sin(-M_PI * 0.5);
	subcurve.controlPoints[4].x = x;
	subcurve.controlPoints[4].y = y;

	subcurve.numControlPoints = 5;
	subcurve.subdivisionSteps = 4;
}

void initControlPoints() {
	int i;
	int num = subcurve.numControlPoints;
	for (i = 0; i < num; i++) {
		constructCircle(circleRadius, numCirclePoints, circles[i].circlePoints);
		circles[i].circleCenter = subcurve.controlPoints[i];
	}
}

void screenToWorldCoordinates(int xScreen, int yScreen, GLdouble* xw, GLdouble* yw)
{
	GLdouble xView, yView;
	screenToCameraCoordinates(xScreen, yScreen, &xView, &yView);
	cameraToWorldCoordinates(xView, yView, xw, yw);
}

void screenToCameraCoordinates(int xScreen, int yScreen, GLdouble* xCamera, GLdouble* yCamera)
{
	*xCamera = ((wvRight - wvLeft) / glutWindowWidth) * xScreen;
	*yCamera = ((wvTop - wvBottom) / glutWindowHeight) * (glutWindowHeight - yScreen);
}

void cameraToWorldCoordinates(GLdouble xcam, GLdouble ycam, GLdouble* xw, GLdouble* yw)
{
	*xw = xcam + wvLeft;
	*yw = ycam + wvBottom;
}

void worldToCameraCoordiantes(GLdouble xWorld, GLdouble yWorld, GLdouble* xcam, GLdouble* ycam)
{
	double wvCenterX = wvLeft + (wvRight - wvLeft) / 2.0;
	double wvCenterY = wvBottom + (wvTop - wvBottom) / 2.0;
	*xcam = worldCenterX - wvCenterX + xWorld;
	*ycam = worldCenterY - wvCenterY + yWorld;
}

int currentButton;

void mouseButtonHandler(int button, int state, int xMouse, int yMouse)
{
	int i;

	currentButton = button;
	if (button == GLUT_LEFT_BUTTON)
	{
		switch (state) {
		case GLUT_DOWN:
			if (hoveredCircle > -1) {
				screenToWorldCoordinates(xMouse, yMouse, &circles[hoveredCircle].circleCenter.x, &circles[hoveredCircle].circleCenter.y);
				screenToWorldCoordinates(xMouse, yMouse, &subcurve.controlPoints[hoveredCircle].x, &subcurve.controlPoints[hoveredCircle].y);
			}
			break;
		case GLUT_UP:
			glutSetWindow(window3D);
			glutPostRedisplay();
			break;
		}
	}
	else if (button == GLUT_MIDDLE_BUTTON)
	{
		switch (state) {
		case GLUT_DOWN:
			break;
		case GLUT_UP:
			if (hoveredCircle == -1 && subcurve.numControlPoints < MAXCONTROLPOINTS) {
				GLdouble newPointX;
				GLdouble newPointY;
				screenToWorldCoordinates(xMouse, yMouse, &newPointX, &newPointY);
				subcurve.controlPoints[subcurve.numControlPoints].x = newPointX;
				subcurve.controlPoints[subcurve.numControlPoints].y = newPointY;
				constructCircle(circleRadius, numCirclePoints, circles[subcurve.numControlPoints].circlePoints);
				circles[subcurve.numControlPoints].circleCenter = subcurve.controlPoints[subcurve.numControlPoints];
				subcurve.numControlPoints++;
			}
			else if (hoveredCircle > -1 && subcurve.numControlPoints > MINCONTROLPOINTS) {
				subcurve.numControlPoints--;
				for (i = hoveredCircle; i < subcurve.numControlPoints; i++) {
					subcurve.controlPoints[i].x = subcurve.controlPoints[i + 1].x;
					subcurve.controlPoints[i].y = subcurve.controlPoints[i + 1].y;
					circles[i].circleCenter = circles[i + 1].circleCenter;
				}
			}

			glutSetWindow(window3D);
			glutPostRedisplay();
			break;
		}
	}

	glutSetWindow(window2D);
	glutPostRedisplay();
}

void mouseMotionHandler(int xMouse, int yMouse)
{
	if (currentButton == GLUT_LEFT_BUTTON) {
		if (hoveredCircle > -1) {
			screenToWorldCoordinates(xMouse, yMouse, &circles[hoveredCircle].circleCenter.x, &circles[hoveredCircle].circleCenter.y);
			screenToWorldCoordinates(xMouse, yMouse, &subcurve.controlPoints[hoveredCircle].x, &subcurve.controlPoints[hoveredCircle].y);
		}
	}
	else if (currentButton == GLUT_MIDDLE_BUTTON) {
	}
	glutPostRedisplay();
}

void mouseHoverHandler(int xMouse, int yMouse)
{
	hoveredCircle = -1;
	GLdouble worldMouseX, worldMouseY;
	screenToWorldCoordinates(xMouse, yMouse, &worldMouseX, &worldMouseY);
	int i;
	// see if we're hovering over a circle
	for (i = 0; i < subcurve.numControlPoints; i++) {
		GLdouble distToX = worldMouseX - circles[i].circleCenter.x;
		GLdouble distToY = worldMouseY - circles[i].circleCenter.y;
		GLdouble euclideanDist = sqrt(distToX * distToX + distToY * distToY);
		//printf("Dist from point %d is %.2f\n", i, euclideanDist);
		if (euclideanDist < 2.0) {
			hoveredCircle = i;
		}
	}

	glutPostRedisplay();
}


void keyboardHandler(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':
	case 'Q':
	case 27:
		// Esc, q, or Q key = Quit 
		exit(0);
		break;
	case 'a':
		// Add code to create timer and call animation handler
		glutSetWindow(window3D);
		animationHandler(1);
		// Use this to set to 3D window and redraw it
		glutSetWindow(window3D);
		glutPostRedisplay();
		break;
	case 'r':
		// reset object position at beginning of curve
		currentCurvePoint = 0;
		glutSetWindow(window3D);
		glutPostRedisplay();
		break;
	case 'c':
		botType = CUBE;
		glutSetWindow(window3D);
		glutPostRedisplay();
		break;
	case 's':
		botType = SPHERE;
		glutSetWindow(window3D);
		glutPostRedisplay();
		break;
	case 'w':
		botType = WHEEL;
		glutSetWindow(window3D);
		glutPostRedisplay();
		break;
	default:
		break;
	}
	glutPostRedisplay();
}

void specialKeyHandler(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		// add code here
		cannonPosition -= 0.5;
		glutSetWindow(window3D);
		glutPostRedisplay();
		break;
	case GLUT_KEY_RIGHT:
		// add code here;
		cannonPosition += 0.5;
		glutSetWindow(window3D);
		glutPostRedisplay();
		break;
	}
	glutPostRedisplay();
}


void reshape(int w, int h)
{
	glutWindowWidth = (GLsizei)w;
	glutWindowHeight = (GLsizei)h;
	glViewport(0, 0, glutWindowWidth, glutWindowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(wvLeft, wvRight, wvBottom, wvTop);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}



/************************************************************************************
 *
 *
 * 3D Window Code
 *
 * Fill in the code in the empty functions
 ************************************************************************************/



void init3DSurfaceWindow()
{
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_AMBIENT, model_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT1, GL_AMBIENT, model_ambient);
	glLightfv(GL_LIGHT1, GL_POSITION, light_position1);

	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);    // Renormalize normal vectors 
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glClearColor(0.4F, 0.4F, 0.4F, 0.0F);  // Color and depth for glClear

	glViewport(0, 0, (GLsizei)window3DSizeX, (GLsizei)window3DSizeY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, aspect, zNear, zFar);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0, 6.0, 22.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}


void reshape3D(int w, int h)
{
	glutWindowWidth = (GLsizei)w;
	glutWindowHeight = (GLsizei)h;
	glViewport(0, 0, glutWindowWidth, glutWindowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, aspect, zNear, zFar);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0, 6.0, 22.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}



void animationHandler(int param)
{
	if (currentCurvePoint < (subcurve.numCurvePoints - 2)) {
		
		currentCurvePoint += 1;
		
		if (botType == WHEEL) {
			wheelRotation += 2.0;
		}
		
			nonConvertedAngle = atan(((1.0 *subcurve.curvePoints[currentCurvePoint + 1].y) - (1.0 *subcurve.curvePoints[currentCurvePoint].y))/ ((1.0 * subcurve.curvePoints[currentCurvePoint + 1].x) - (1.0*subcurve.curvePoints[currentCurvePoint].x)));
			
			angle = (nonConvertedAngle / M_PI) * 180.0;
			glutSetWindow(window3D);
			glutPostRedisplay();
			glutTimerFunc(100, animationHandler, 0);
			
		
	}
}

void display3D()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(eyeX, eyeY, eyeZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	drawGround();
	draw3DSubdivisionCurve();
	draw3DControlPoints();
	glPushMatrix();
		glTranslatef(subcurve.curvePoints[currentCurvePoint].x, 0, -subcurve.curvePoints[currentCurvePoint].y);
		drawBot();
	glPopMatrix();

	glPushMatrix();
		glTranslatef(cannonPosition, 0.0, 0.0);
		drawCannon();
	glPopMatrix();
	glutSwapBuffers();
}

GLfloat threeDCurve_ambient[] = { 0.6172f,0.0f,0.0f,0.0f };

void draw3DSubdivisionCurve()
{
	// Subdivide the given curve
	computeSubdivisionCurve(&subcurve);

	int i = 0;

	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, threeDCurve_ambient);
	glBegin(GL_LINE_STRIP);
	for (i = 0; i < subcurve.numCurvePoints; i++) {
		glVertex3f(subcurve.curvePoints[i].x, 0.5, -subcurve.curvePoints[i].y);
	}
	glEnd();
	glPopMatrix();
}

void draw3DControlPoints()
{
	
	int i, j;
	for (i = 0; i < subcurve.numControlPoints; i++) {
		glPushMatrix();
		glTranslatef(circles[i].circleCenter.x, 0.5, -circles[i].circleCenter.y);
		// for the hoveredCircle, draw an outline and change its colour
		if (i == hoveredCircle) {
			// outline
			//glColor3f(0.0, 1.0, 0.0);
			glBegin(GL_LINE_LOOP);
			for (j = 0; j < numCirclePoints; j++) {
				glVertex3f(circles[i].circlePoints[j].x, 0, -circles[i].circlePoints[j].y);
			}
			glEnd();
			// colour change
			//glColor3f(0.5, 0.0, 1.0);
		}
		glBegin(GL_LINE_LOOP);
		for (j = 0; j < numCirclePoints; j++) {
			glVertex3f(circles[i].circlePoints[j].x, 0, -circles[i].circlePoints[j].y);
		}
		glEnd();
		glPopMatrix();
	
	}
}

GLfloat robotBody_mat_ambient[] = { 0.0f,0.0f,0.0f,1.0f };
GLfloat robotBody_mat_specular[] = { 0.45f,0.55f,0.45f,1.0f };
GLfloat robotBody_mat_diffuse[] = { 0.1f,0.35f,0.1f,1.0f };
GLfloat robotBody_mat_shininess[] = { 20.0F };

void drawBot()
{
	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, robotBody_mat_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, robotBody_mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, robotBody_mat_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, robotBody_mat_shininess);
	glTranslatef(0, 1.0, 0);
	glRotatef(angle, 0.0, 1.0, 0.0);

	if (botType == CUBE)
	{
		glPushMatrix();
		 glTranslatef(0.0, -0.5, 0.0);
		 glRotatef(-90, 0.0, 1.0, 0.0);
		 glScalef(1.0, 1.0, 3.0);
		 glutSolidCube(cubeSize);
		glPopMatrix();
	}
	else if (botType == SPHERE)
	{
		glPushMatrix();
		glutSolidSphere(sphereRadius, 30, 30);
		glPopMatrix();
	}
	else if (botType == WHEEL)
	{
		glPushMatrix();
		glTranslatef(0.0, 0.0, -wheelThickness / 2.0);
		glPushMatrix();
		glRotatef(wheelRotation, 0.0, 0.0, 1.0);
			glPushMatrix();
				gluDisk(gluNewQuadric(), 0, wheelRadius, 20, 10);
			glPopMatrix();
			glPushMatrix();
				glTranslatef(0, 0, wheelThickness);
				gluDisk(gluNewQuadric(), 0, wheelRadius, 20, 10);
			glPopMatrix();
			gluCylinder(gluNewQuadric(),wheelRadius,wheelRadius,wheelThickness,30,30);
		glPopMatrix();
		glPopMatrix();
	}
	glPopMatrix();
}


void drawCannon()
{
	glPushMatrix();
	glTranslatef(0.0, 1.0, 12.0);
		glPushMatrix();
			glTranslatef(0.0,2.0,0.0);
			glutSolidSphere(0.5, 30, 30);
		glPopMatrix();
		glPushMatrix();
			glRotatef(-90, 1.0, 0.0, 0.0);
			glutSolidCone(1.0, 2.0, 30, 30);
		glPopMatrix();
	glPopMatrix();
}

void drawGround() {
	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, groundMat_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, groundMat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, groundMat_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, groundMat_shininess);
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);
	glVertex3f(-12.0f, -1.0f, -12.0f);
	glVertex3f(-12.0f, -1.0f, 12.0f);
	glVertex3f(12.0f, -1.0f, 12.0f);
	glVertex3f(12.0f, -1.0f, -12.0f);
	glEnd();
	glPopMatrix();
}

void mouseButtonHandler3D(int button, int state, int x, int y)
{

	currentButton = button;
	lastMouseX = x;
	lastMouseY = y;
	switch (button)
	{
	case GLUT_LEFT_BUTTON:

		break;
	case GLUT_RIGHT_BUTTON:

		break;
	case GLUT_MIDDLE_BUTTON:

		break;
	default:
		break;
	}
}


void mouseMotionHandler3D(int x, int y)
{
	int dx = x - lastMouseX;
	int dy = y - lastMouseY;
	if (currentButton == GLUT_LEFT_BUTTON) {
		if (dy > 0) {
			eyeZ += 0.05;
			
		}
		else {
			eyeZ -= 0.05;
		}
		glutSetWindow(window3D);
		glutPostRedisplay();
	}
	if (currentButton == GLUT_RIGHT_BUTTON)
	{
	}
	else if (currentButton == GLUT_MIDDLE_BUTTON)
	{
	}
	lastMouseX = x;
	lastMouseY = y;
	glutPostRedisplay();
}



// Some Utility Functions

Vector3D crossProduct(Vector3D a, Vector3D b) {
	Vector3D cross;

	cross.x = a.y * b.z - b.y * a.z;
	cross.y = a.x * b.z - b.x * a.z;
	cross.z = a.x * b.y - b.x * a.y;

	return cross;
}

Vector3D normalize(Vector3D a) {
	GLdouble norm = sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	Vector3D normalized;
	normalized.x = a.x / norm;
	normalized.y = a.y / norm;
	normalized.z = a.z / norm;
	return normalized;
}








