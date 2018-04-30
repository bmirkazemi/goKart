//
// authors: Bijan Mirkazemi & Robert Pierucci
// date: Spring 2018
// purpose: 3d game similar to mario kart
//
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glxext.h>
#include "fonts.h"
#include <iostream>
#include <time.h>
using namespace std;

//some defined macros
#define VecMake(x,y,z,v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecNegate(a) (a)[0]=(-(a)[0]);\
							(a)[1]=(-(a)[1]);\
(a)[2]=(-(a)[2])
#define VecDot(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecLen(a) ((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2])
#define VecLenSq(a) sqrtf((a)[0]*(a)[0]+(a)[1]*(a)[1]+(a)[2]*(a)[2])
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecAdd(a,b,c) (c)[0]=(a)[0]+(b)[0];\
							 (c)[1]=(a)[1]+(b)[1];\
(c)[2]=(a)[2]+(b)[2]
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0];\
							 (c)[1]=(a)[1]-(b)[1];\
(c)[2]=(a)[2]-(b)[2]
#define VecS(A,a,b) (b)[0]=(A)*(a)[0]; (b)[1]=(A)*(a)[1]; (b)[2]=(A)*(a)[2]
#define VecAddS(A,a,b,c) (c)[0]=(A)*(a)[0]+(b)[0];\
								(c)[1]=(A)*(a)[1]+(b)[1];\
(c)[2]=(A)*(a)[2]+(b)[2]
#define VecCross(a,b,c) (c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1];\
							   (c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2];\
(c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0]
#define VecZero(v) (v)[0]=0.0;(v)[1]=0.0;v[2]=0.0
#define ABS(a) (((a)<0)?(-(a)):(a))
#define SGN(a) (((a)<0)?(-1):(1))
#define SGND(a) (((a)<0.0)?(-1.0):(1.0))
#define rnd() (float)rand() / (float)RAND_MAX
#define PI 3.14159265358979323846264338327950
#define MY_INFINITY 1000.0

#define setv(a,b) \
	a[0]=MY_INFINITY*(b[0]-g.lightPosition[0])+g.lightPosition[0];\
a[1]=MY_INFINITY*(b[1]-g.lightPosition[1])+g.lightPosition[1];\
a[2]=MY_INFINITY*(b[2]-g.lightPosition[2])+g.lightPosition[2]

Display *dpy;
Window win;
GLXContext glc;

typedef float Flt;
typedef Flt Vec[3];
typedef Flt	Matrix[4][4];
typedef int iVec[3];
const Flt DTR = 1.0 / 180.0 * PI;

void init(void);
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void physics(void);
void render(void);
void callControls(void);
void vecScale(Vec v, Flt scale);
void vecSub(Vec v0, Vec v1, Vec dest);
//bool pointInTriangle(Vec tri[3], Vec p, Flt *u, Flt *v);
int pointInTriangle(Vec a, Vec b, Vec c, Vec p, Flt *u, Flt *v);
Flt vecDotProduct(Vec v0, Vec v1);
void vecCrossProduct(Vec v0, Vec v1, Vec dest);
Flt vecLength(Vec vec);
const Vec upv = {0.0, 1.0, 0.0};

//-----------------------------------------------------------------------------
//Setup timers
const double OOBILLION = 1.0 / 1e9;
extern struct timespec timeStart, timeCurrent;
extern double timeDiff(struct timespec *start, struct timespec *end);
extern void timeCopy(struct timespec *dest, struct timespec *source);
//-----------------------------------------------------------------------------

class Image {
	public:
		int width, height;
		unsigned char *data;
		~Image() { delete [] data; }
		Image(const char *fname) {
			if (fname[0] == '\0')
				return;
			char name[40];
			strcpy(name, fname);
			int slen = strlen(name);
			name[slen-4] = '\0';
			char ppmname[80];
			sprintf(ppmname,"%s.ppm", name);
			char ts[100];
			sprintf(ts, "convert %s %s", fname, ppmname);
			system(ts);
			//sprintf(ts, "%s", name);
			FILE *fpi = fopen(ppmname, "r");
			if (fpi) {
				char line[200];
				fgets(line, 200, fpi);
				fgets(line, 200, fpi);
				while (line[0] == '#')
					fgets(line, 200, fpi);
				sscanf(line, "%i %i", &width, &height);
				fgets(line, 200, fpi);
				//get pixel data
				int n = width * height * 3;
				data = new unsigned char[n];
				for (int i=0; i<n; i++)
					data[i] = fgetc(fpi);
				fclose(fpi);
			} else {
				printf("ERROR opening image: %s\n",ppmname);
				exit(0);
			}
			unlink(ppmname);
		}
};
Image img[2] = {"./assets/bg1.jpg", "./assets/road.jpg"};

class Texture {
	public:
		Image *backImage, *roadImage;
		GLuint backTexture, roadTexture;
		float xc[2];
		float yc[2];
};

class Global {
	public:
		int done;
		int renderCount;
		char keypress[65536];
		Flt gravity;
		int xres, yres;
		Flt aspectRatio;
		Texture tex;
		GLfloat lightPosition[4];
		Vec cameraPosition;
		Vec spot;
		bool passCones[10];
		bool shadows;
		bool crash;
		Matrix m;
		int lapcount;
		float cx, cy, cz;
		int conecount;
		int fps;
		int circling;
		int sorting;
		int billboarding;
		struct timespec smokeStart, smokeTime;
		struct timespec renderStart, renderTime;
		Global() {
			lapcount = 0;
			conecount;
			for(int i = 0; i< 10; i++) {
				passCones[i] = false;
			}
			fps = 0;
			for (int i = 0; i < 65336; i++) {
				keypress[i] = 0;
			}
			renderCount = 0;
			shadows = true;
			crash = false;
			cx = 0.0;
			cy = 0.0;
			cz = 0.0;
			gravity = 0.01;
			xres=640;
			yres=480;
			aspectRatio = (GLfloat)xres / (GLfloat)yres;
			VecMake(0.0, 20.0, -1.0, cameraPosition);
			VecMake(100.0f, 240.0f, 40.0f, lightPosition);
			lightPosition[3] = 1.0f;
		}
		void identity33(Matrix m) {
			m[0][0] = m[1][1] = m[2][2] = 1.0f;
			m[0][1] = m[0][2] = m[1][0] = m[1][2] = m[2][0] = m[2][1] = 0.0f;
		}
} g;

class Object {
	public:
		bool reverse;
		bool forward ;
		float leftRotate;
		float rightRotate;
		float velocity;
		Vec *vert;
		Vec *vertText;
		Vec *norm;
		iVec *face;
		iVec *faceText;
		Matrix m;
		int nverts;
		int nvertText;
		int nfaces;
		int nfaceText;
		float Hangle, Vangle;
		Vec dir;
		Vec pos, vel, rot, lastPos, nextPos;
		Vec color;
		Vec a[1000][3];
		Vec b[1000][3];
		int nshadows;
	public:
		~Object() {
			delete [] vert;
			delete [] vertText;
			delete [] face;
			delete [] faceText;
			delete [] norm;
		}
		Object(int nv, int nf, int nt, int nft) {
			Hangle = Vangle = 0;
			forward = reverse = false;
			leftRotate = rightRotate = 0;
			velocity = 0;
			vert = new Vec[nv];
			vertText = new Vec[nt];
			face = new iVec[nf];
			faceText = new iVec[nft];
			norm = new Vec[nf];
			nverts = nv;
			nfaces = nf;
			nvertText = nt;
			nfaceText = nft;
			VecZero(pos);
			VecZero(vel);
			VecZero(rot);
			VecZero(lastPos);
			VecZero(nextPos);
			VecMake(.9, .9, 0, color);
			g.identity33(m);
			VecMake(sin(Hangle), sin(Vangle), -cos(Hangle), dir);
		}
		void setColor(float r, float g, float b) {
			VecMake(r, g, b, color);
		}
		void setVert(Vec v, int i) {
			VecMake(v[0], v[1], v[2], vert[i]);
		}
		void setVertText(Vec v, int i) {
			VecMake(v[0], v[1], 0, vertText[i]);
		}
		void setFace(iVec f, int i) {
			VecMake(f[0], f[2], f[1], face[i]);
		}
		void setFaceText(iVec f, int i) {
			VecMake(f[0], f[2], f[1], faceText[i]);
		}
		void applyGravity() {
			pos[1] -= g.gravity;
		}
		void translate(Flt x, Flt y, Flt z) {
			pos[0] += x;
			pos[1] += y;
			pos[2] += z;
		}
		void rotate(Flt x, Flt y, Flt z) {
			rot[0] += x;
			rot[1] += y;
			rot[2] += z;
		}
		void scale(Flt scalar) {
			for (int i=0; i<nverts; i++) {
				VecS(scalar, vert[i], vert[i]);
			}
		}
		void getTriangleNormal(Vec tri[3], Vec norm) {
			Vec v0, v1;
			VecSub(tri[1], tri[0], v0);
			VecSub(tri[2], tri[0], v1);
			VecCross(v0, v1, norm);
			Flt VecNormalize(Vec vec);
			VecNormalize(norm);
		}

		void draw() {
			//build our own rotation matrix for rotating the shadow polys.
			g.identity33(m);
			Vec vr;
			VecMake(rot[0]*DTR, rot[1]*DTR, rot[2]*DTR, vr);
			void yy_transform(const Vec rotate, Matrix a);
			yy_transform(vr, m);
			//must do for each triangle face...
			nshadows = 0;
			for (int j=0; j<nfaces; j++) {
				//transform the vertices of the tri, to use for the shadow
				Vec tv[3], v0, norm;
				int fa = face[j][0];
				int fb = face[j][1];
				int fc = face[j][2];
				void trans_vector(Matrix mat, const Vec in, Vec out);
				trans_vector(m, vert[fa], tv[0]);
				trans_vector(m, vert[fb], tv[1]);
				trans_vector(m, vert[fc], tv[2]);
				VecAdd(tv[0], pos, tv[0]);
				VecAdd(tv[1], pos, tv[1]);
				VecAdd(tv[2], pos, tv[2]);
				getTriangleNormal(tv, norm);
				//Is triangle facing the light source???
				//v0 is vector to light
				VecSub(g.lightPosition, tv[0], v0);
				if (g.shadows) {
					if (VecDot(v0, norm) > 0.0) {
						for (int i=0; i<3; i++) {
							//form the shadow tri, using the light source.
							VecCopy(tv[i], b[nshadows][i]);
							setv(a[nshadows][i], b[nshadows][i]);
						}
						nshadows++;
					}
				}
				glPushMatrix();
				glBegin(GL_TRIANGLES);
				if (j == 0) {
				    Vec colortest;
				    VecMake(1.0, 1.0, 1.0, colortest); 
				    glColor3fv(colortest);
				} else {
				    glColor3fv(color);
				}
				glNormal3fv(norm);
				glVertex3fv(tv[0]);
				glVertex3fv(tv[1]);
				glVertex3fv(tv[2]);
				glEnd();
				glPopMatrix();
			}
		}
		void drawTexture() {
			cout << nfaceText << "NFACE" << endl;
			for (int i = 0; i < nfaceText; i++) {
				glPushMatrix();
				glRotatef(90, 0, 1, 0);
				glTranslated(-1.0, -2, 0);
				glScalef(3, 3, 3);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_MODULATE);
				glBindTexture(GL_TEXTURE_2D, g.tex.roadTexture);
				glBegin(GL_TRIANGLES);

				glTexCoord2f(vertText[faceText[i][0]-1][0], vertText[faceText[i][0]-1][1]); 
				glVertex3f(vert[face[i][0]][0], vert[face[i][0]][1], vert[face[i][0]][2]);

				glTexCoord2f(vertText[faceText[i][1]-1][0], vertText[faceText[i][1]-1][1]); 
				glVertex3f(vert[face[i][1]][0], vert[face[i][1]][1], vert[face[i][1]][2]);

				glTexCoord2f(vertText[faceText[i][2]-1][0], vertText[faceText[i][1]-1][1]); 
				glVertex3f(vert[face[i][2]][0], vert[face[i][2]][1], vert[face[i][2]][2]);

				glEnd();
				glPopMatrix();	

				cout << "tex coords" << vertText[faceText[i][0]-1][0] << " " << vertText[faceText[i][0]-1][1] << "     :" << endl;
			}

			glBindTexture(GL_TEXTURE_2D, 0);
			glClear(GL_DEPTH_BUFFER_BIT);		
		}

		void triShadowVolume() {
			for (int i=0; i<nshadows; i++) {
				glPushMatrix();
				glBegin(GL_TRIANGLES);
				//back cap
				glVertex3fv(a[i][2]);
				glVertex3fv(a[i][1]);
				glVertex3fv(a[i][0]);
				//front cap
				glVertex3fv(b[i][0]);
				glVertex3fv(b[i][1]);
				glVertex3fv(b[i][2]);
				glEnd();
				glBegin(GL_QUAD_STRIP);
				glVertex3fv(b[i][0]); glVertex3fv(a[i][0]);
				glVertex3fv(b[i][1]); glVertex3fv(a[i][1]);
				glVertex3fv(b[i][2]); glVertex3fv(a[i][2]);
				glVertex3fv(b[i][0]); glVertex3fv(a[i][0]);
				glEnd();
				glPopMatrix();
			}
		}
} *track, *kart, *bowser, *block, *finish, *cones[10];

int main(void)
{
	g.done=0;
	initXWindows();
	init_opengl();
	init();
	clock_gettime(CLOCK_REALTIME, &g.renderStart);
	while (g.done == 0) {
		while (XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			check_keys(&e);
		}
		clock_gettime(CLOCK_REALTIME, &g.renderTime);
		double d = timeDiff(&g.renderStart, &g.renderTime);
		if (d >= 1.0) {
			g.fps = g.renderCount;
			g.renderCount = 0;
			timeCopy(&g.renderStart, &g.renderTime);
		}
		g.renderCount++;
		physics();
		render();
		glXSwapBuffers(dpy, win);
	}
	cleanupXWindows();
	cleanup_fonts();
	return 0;
}

void cleanupXWindows(void)
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void set_title(void)
{
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "Vector Kart");
}

void setup_screen_res(const int w, const int h)
{
	g.xres = w;
	g.yres = h;
	g.aspectRatio = (GLfloat)g.xres / (GLfloat)g.yres;
}

void initXWindows(void)
{
	GLint att[] = { GLX_RGBA,
		GLX_STENCIL_SIZE, 2,
		GLX_DEPTH_SIZE, 24,
		GLX_DOUBLEBUFFER, None };
	XSetWindowAttributes swa;

	int fullscreen = 0;
	dpy = XOpenDisplay(NULL);
	Screen*  s = DefaultScreenOfDisplay(dpy);
	setup_screen_res(s->width, s->height);
	if (dpy == NULL) {
		printf("\n\tcannot connect to X server\n\n");
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XGrabKeyboard(dpy, root, False,
			GrabModeAsync, GrabModeAsync, CurrentTime);
	//fullscreen = 1;
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		printf("\n\tno appropriate visual found\n\n");
		exit(EXIT_FAILURE);
	}
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
		StructureNotifyMask | SubstructureNotifyMask;
	unsigned int winops = CWBorderPixel|CWColormap|CWEventMask;
	if (fullscreen) {
		winops |= CWOverrideRedirect;
		swa.override_redirect = True;
	}
	win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
			vi->depth, InputOutput, vi->visual,
			winops, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void reshape_window(int width, int height)
{
	//window has been resized.
	setup_screen_res(width, height);
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	set_title();
}

void init(void)
{
	//Display *dpy = glXGetCurrentDisplay();
	//GLXDrawable drawable = glXGetCurrentDrawable();
	//unsigned int swap = 0;
	//glXQueryDrawable(dpy, drawable, GLX_SWAP_INTERVAL_EXT, &swap);	
	//system("export __GL_SYNC_TO_VBLANK=0");
	//track
	Object *buildModel(const char *mname);
	track = buildModel("./assets/trackfinal.obj");
	track->scale(3);
	track->translate(-1.0, -2, 0);
	track->rotate(0, 90, 0);
	track->setColor(0.2,0.2,0.2);
	//mario
	Object *buildModel(const char *mname);
	kart = buildModel("./assets/mario/karttex.obj");
	kart->scale(0.2);
	kart->rotate(0, 180, 0);
	kart->translate(-0.5, 0, 0);
	kart->setColor(.8,0,.9);
	//bowser
	Object *buildModel(const char *mname);
	bowser = buildModel("./assets/bowser/bkart.obj");
	bowser->scale(0.5);
	bowser->rotate(0, 180, 0);
	bowser->translate(-0.5, 0, -3.0);
	bowser->setColor(.4,.2,.9);
	//finishline
	Object *buildModel(const char *mname);
	finish = buildModel("./assets/finishline.obj");
	finish->scale(1);
	finish->translate(-1.0, -2, 0);
	finish->rotate(0, 90, 0);
	finish->setColor (0.9, 0.9, 0.9);
	//cones
	for (int i = 0; i < 10; i++) {
		Object *buildModel(const char *mname);
		cones[i] = buildModel("./assets/cones.obj");
		cones[i]->scale(0.7);
		cones[i]->setColor (0.9, 0, 0.9);
	}

	cones[0]->translate(-1.8, -2, -15.0);
	cones[0]->rotate(0, 0, 0);

	cones[1]->translate(-13.5, -2, -40.0);
	cones[1]->rotate(0, 30, 0);

	cones[2]->translate(0, -2, -102.5);
	cones[2]->rotate(0, 90, 0);

	cones[3]->translate(41, -2, -85);
	cones[3]->rotate(0,30,0);

	cones[4]->translate(70, -2, -22);

	cones[5]->translate(70, -2, 94);

	cones[6]->translate(88, -2, 127);
	cones[6]->rotate(0,60,0);

	cones[7]->translate(100, -2, 172);

	cones[8]->translate(52, -2, 191);
	cones[8]->rotate(0,85,0);

	cones[9]->translate(-8, -2, 70);

}

void init_opengl(void)
{
	//OpenGL initialization
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, g.aspectRatio, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	//Enable this so material colors are the same as vert colors.
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	//Turn on a light
	glLightfv(GL_LIGHT0, GL_POSITION, g.lightPosition);
	glEnable(GL_LIGHT0);
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
	//init_textures();

	//background image texturing	
	g.tex.backImage = &img[0];
	glGenTextures(1, &g.tex.backTexture);
	int w = g.tex.backImage->width;
	int h = g.tex.backImage->height;
	glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
			GL_RGB, GL_UNSIGNED_BYTE, g.tex.backImage->data);
	g.tex.xc[0] = 0.0;
	g.tex.xc[1] = 0.25;
	g.tex.yc[0] = 0.0;
	g.tex.yc[1] = 1.0;

	//road texturing
	g.tex.roadImage = &img[1];
	glGenTextures(1, &g.tex.roadTexture);
	int wRoad = g.tex.roadImage->width;
	int hRoad = g.tex.roadImage->height;
	glBindTexture(GL_TEXTURE_2D, g.tex.roadTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, wRoad, hRoad, 0,
			GL_RGB, GL_UNSIGNED_BYTE, g.tex.roadImage->data);
	g.tex.xc[0] = 0.0;
	g.tex.xc[1] = 1.0;
	g.tex.yc[0] = 0.0;
	g.tex.yc[1] = 1.0;

	GLint stencilBits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
	if (stencilBits < 1) {
		printf("No stencil buffer on this computer.\n");
		printf("Exiting program.\n");
		exit(0);
	}
	//------------------------------------------------------------------------
}

Object *buildModel(const char *mname)
{
	char line[200];
	Vec *vert=NULL;  //vertices in list
	Vec *verttc=NULL;
	iVec *face=NULL; //3 indicies per face
	iVec *text=NULL; //textures mapped to faces
	int nv=0, nf=0, ntf=0, nt=0;
	printf("void buildModel(%s)...\n",mname);
	//Model exported from Blender. Assume an obj file.
	FILE *fpi = fopen(mname,"r");
	if (!fpi) {
		printf("ERROR: file **%s** not found.\n", mname);
		return NULL;
	}
	//count all vertices
	fseek(fpi, 0, SEEK_SET);
	while (fgets(line, 100, fpi) != NULL) {
		if (line[0] == 'v' && line[1] == ' ')
			nv++;
	}
	vert = new Vec[nv];
	if (!vert) {
		printf("ERROR: out of mem (vert)\n");
		exit(EXIT_FAILURE);
	}
	printf("n verts: %i\n", nv);

	//cout all texture vertices
	fseek(fpi, 0, SEEK_SET);
	while (fgets(line, 100, fpi) != NULL) {
		if (line[0] == 'v' && line[1] == 't')
			nt++;
	}
	verttc = new Vec[nt];
	if (!verttc) {
		printf("ERROR: out of mem (vert)\n");
		exit(EXIT_FAILURE);
	}
	printf("n verttc: %i\n", nt);

	//count all faces
	int iface[4];
	int itext[4];
	fseek(fpi, 0, SEEK_SET);
	while (fgets(line, 100, fpi) != NULL) {
		if (line[0] == 'f' && line[1] == ' ') {
			sscanf(line+1,"%i/%i %i/%i %i/%i", &iface[0], &itext[0], &iface[1], &itext[1], &iface[2], &itext[2]);
			nt++;
			nf++;
		}
	}
	face = new iVec[nf];
	text = new iVec[nt];
	if (!face) {
		printf("ERROR: out of mem (face)\n");
		exit(EXIT_FAILURE);
	}
	printf("n faces: %i\n", nf);
	//first pass, read all vertices
	nv=0;
	fseek(fpi, 0, SEEK_SET);
	while (fgets(line, 100, fpi) != NULL) {
		if (line[0] == 'v' && line[1] == ' ') {
			sscanf(line+1,"%f %f %f",&vert[nv][0],&vert[nv][1],&vert[nv][2]);
			nv++;
		}
	}
	//second pass, read all vertext texture points
	nt=0;
	fseek(fpi, 0, SEEK_SET);
	while (fgets(line, 100, fpi) != NULL) {
		if (line[0] == 'v' && line[1] == 't') {
			cout << line << endl;
			sscanf(line+2, "%f %f", &verttc[nt][0], &verttc[nt][1]);
			nt++;
		}
	}
	//third pass, read all faces/textmapspoints
	int comment=0;
	nf=0;
	ntf=0;
	fseek(fpi, 0, SEEK_SET);
	while (fgets(line, 100, fpi) != NULL) {
		if (line[0] == '/' && line[1] == '*') {
			comment=1;
		}
		if (line[0] == '*' && line[1] == '/') {
			comment=0;
			continue;
		}
		if (comment)
			continue;
		if (line[0] == 'f' && line[1] == ' ') {
			sscanf(line+1,"%i/%i %i/%i %i/%i", &iface[0], &itext[0], &iface[1], &itext[1], &iface[2], &itext[2]);
			face[nf][0] = iface[1]-1;
			face[nf][1] = iface[0]-1;
			face[nf][2] = iface[2]-1;
			text[ntf][0] = itext[1]-1;
			text[ntf][1] = itext[0]-1;
			text[ntf][2] = itext[2]-1;
			ntf++;
			nf++;
		}
	}
	fclose(fpi);
	printf("nverts: %i   nfaces: %i    nt: %i\n", nv, nf, nt);

	Object *o = new Object(nv, nf, nt, ntf);
	for (int i=0; i<nv; i++) {
		o->setVert(vert[i], i);
	}
	cout << "done 1" << endl;
	for (int i=0; i<nt; i++) {
		o->setVertText(verttc[i], i);
	}
	cout << "done 2" << endl;
	for (int i=0; i<nf; i++) {
		o->setFace(face[i], i);
	}
	cout << "done 3" << endl;
	for (int i=0; i<ntf; i++) {
		o->setFaceText(text[i], i);
	}
	cout << "done 4" << endl;
	delete [] vert;
	delete [] verttc;
	delete [] face;
	delete [] text;
	return o;
}

int check_keys(XEvent *e)
{
	//Was there input from the keyboard?
	static int shift = 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		g.keypress[key] = 1;
		if(g.keypress[XK_Shift_R] || g.keypress[XK_Shift_L]) {
			shift = 1;
		}
	}
	if (e->type == KeyRelease) {
		g.keypress[key] = 0;
		if(g.keypress[XK_Shift_R] || g.keypress[XK_Shift_L]) {
			shift = 0;
		}
	}
	return 0;
}

Flt VecNormalize(Vec vec) {
	Flt len, tlen;
	len = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
	if (len == 0.0) {
		VecMake(0.0,0.0,1.0,vec);
		return 1.0;
	}
	len = sqrt(len);
	tlen = 1.0 / len;
	vec[0] *= tlen;
	vec[1] *= tlen;
	vec[2] *= tlen;
	return(len);
}

int pointInTriangle(Vec a, Vec b, Vec c, Vec p, Flt *u, Flt *v)
{
    //source:
    //http://blogs.msdn.com/b/rezanour/archive/2011/08/07/
    //
    Vec cross0,cross1,cross2;
    Vec ba,ca,pa;
    vecSub(b,a,ba);
    vecSub(c,a,ca);
    vecSub(p,a,pa);
    //This is a half-space test
    vecCrossProduct(ca,pa,cross1);
    vecCrossProduct(ca,ba,cross0);
    if (vecDotProduct(cross0, cross1) < 0.0)
        return 0;
    //This is a half-space test
    vecCrossProduct(ba,pa,cross2);
    vecCrossProduct(ba,ca,cross0);
    if (vecDotProduct(cross0, cross2) < 0.0)
        return 0;
    //Point is within 2 half-spaces
    //Get area proportions
    //Area is actually length/2
    Flt areaABC = vecLength(cross0);
    Flt areaV = vecLength(cross1);
    Flt areaU = vecLength(cross2);
    *u = areaU / areaABC;
    *v = areaV / areaABC;
    return (*u >= 0.0 && *v >= 0.0 && *u + *v <= 1.0);
}

/*
bool pointInTriangle(Vec tri[3], Vec p, Flt *u, Flt *v)
{
	//source: http://blogs.msdn.com/b/rezanour/archive/2011/08/07/
	//This function determines if point p is inside triangle tri.
	//   step 1: 3D half-space tests
	//   step 2: find barycentric coordinates
	//
	Vec cross0, cross1, cross2;
	Vec ba, ca, pa;
	//setup sub-triangles
	vecSub(tri[1], tri[0], ba);
	vecSub(tri[2], tri[0], ca);
	vecSub(p, tri[0], pa);
	//This is a half-space test
	vecCrossProduct(ca, pa, cross1);
	vecCrossProduct(ca, ba, cross0);
	if (vecDotProduct(cross0, cross1) < 0.0)
		return 0;
	//This is a half-space test
	vecCrossProduct(ba,pa,cross2);
	vecCrossProduct(ba,ca,cross0);
	if (vecDotProduct(cross0, cross2) < 0.0)
		return 0;
	//Point is within 2 half-spaces
	//Get area proportions
	//Area is actually length/2, but we just care about the relationship.
	Flt areaABC = vecLength(cross0);
	Flt areaV = vecLength(cross1);
	Flt areaU = vecLength(cross2);
	*u = areaU / areaABC;
	*v = areaV / areaABC;
	return (*u >= 0.0 && *v >= 0.0 && *u + *v <= 1.0);
}
*/

Flt vecDotProduct(Vec v0, Vec v1)
{
	return (v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2]);
}

void vecCrossProduct(Vec v0, Vec v1, Vec dest)
{
	dest[0] = v0[1]*v1[2] - v1[1]*v0[2];
	dest[1] = v0[2]*v1[0] - v1[2]*v0[0];
	dest[2] = v0[0]*v1[1] - v1[0]*v0[1];
}

Flt vecLength(Vec vec)
{
	return sqrt(vecDotProduct(vec, vec));
}

void vecSub(Vec v0, Vec v1, Vec dest)
{
	dest[0] = v0[0] - v1[0];
	dest[1] = v0[1] - v1[1];
	dest[2] = v0[2] - v1[2];
}

void vecScale(Vec v, Flt scale) {
	v[0] *= scale;
	v[1] *= scale;
	v[2] *= scale;
}

void yy_transform(const Vec rotate, Matrix a)
{
	//This function applies a rotation to a matrix.
	//Call this function first, then call trans_vector() to apply the
	//rotations to an object or vertex.
	//
	if (rotate[0] != 0.0f) {
		Flt ct = cos(rotate[0]), st = sin(rotate[0]);
		Flt t10 = ct*a[1][0] - st*a[2][0];
		Flt t11 = ct*a[1][1] - st*a[2][1];
		Flt t12 = ct*a[1][2] - st*a[2][2];
		Flt t20 = st*a[1][0] + ct*a[2][0];
		Flt t21 = st*a[1][1] + ct*a[2][1];
		Flt t22 = st*a[1][2] + ct*a[2][2];
		a[1][0] = t10;
		a[1][1] = t11;
		a[1][2] = t12;
		a[2][0] = t20;
		a[2][1] = t21;
		a[2][2] = t22;
		//return;
	}
	if (rotate[1] != 0.0f) {
		Flt ct = cos(rotate[1]), st = sin(rotate[1]);
		Flt t00 = ct*a[0][0] - st*a[2][0];
		Flt t01 = ct*a[0][1] - st*a[2][1];
		Flt t02 = ct*a[0][2] - st*a[2][2];
		Flt t20 = st*a[0][0] + ct*a[2][0];
		Flt t21 = st*a[0][1] + ct*a[2][1];
		Flt t22 = st*a[0][2] + ct*a[2][2];
		a[0][0] = t00;
		a[0][1] = t01;
		a[0][2] = t02;
		a[2][0] = t20;
		a[2][1] = t21;
		a[2][2] = t22;
		//return;
	}
	if (rotate[2] != 0.0f) {
		Flt ct = cos(rotate[2]), st = sin(rotate[2]);
		Flt t00 = ct*a[0][0] - st*a[1][0];
		Flt t01 = ct*a[0][1] - st*a[1][1];
		Flt t02 = ct*a[0][2] - st*a[1][2];
		Flt t10 = st*a[0][0] + ct*a[1][0];
		Flt t11 = st*a[0][1] + ct*a[1][1];
		Flt t12 = st*a[0][2] + ct*a[1][2];
		a[0][0] = t00;
		a[0][1] = t01;
		a[0][2] = t02;
		a[1][0] = t10;
		a[1][1] = t11;
		a[1][2] = t12;
		//return;
	}
}

void trans_vector(Matrix mat, const Vec in, Vec out)
{
	Flt f0 = mat[0][0] * in[0] + mat[1][0] * in[1] + mat[2][0] * in[2];
	Flt f1 = mat[0][1] * in[0] + mat[1][1] * in[1] + mat[2][1] * in[2];
	Flt f2 = mat[0][2] * in[0] + mat[1][2] * in[1] + mat[2][2] * in[2];
	out[0] = f0;
	out[1] = f1;
	out[2] = f2;
}

void physics(void)
{
	//apply gravity to kart
	kart->applyGravity();

	bowser->applyGravity();
	if(bowser->pos[1] <= track->pos[1]) {
		bowser->pos[1] = track->pos[1];
	}

	//check if kart collided with track
	if(kart->pos[1] <= track->pos[1]) {
		kart->pos[1] = track->pos[1];
	}


	//Flt u,v;

	for (int i=0; i<1; i++) {
		for (int j=0; j<3; j++) {
		    g.cx += track->vert[track->face[i][j]][0];
		    g.cy += track->vert[track->face[i][j]][1];
		    g.cz += track->vert[track->face[i][j]][2];
		}
		g.cx /= 3;
		g.cy /= 3;
		g.cz /= 3;
	    	/*
		Vec tri[3];
		for (int j=0; j<3; j++) {
			VecMake(track->vert[track->face[i][j]][0], track->vert[track->face[i][j]][1], track->vert[track->face[i][j]][2], tri[j]);
		}
		cout << "tri0 " << tri[0][0] << " " << tri[0][1] << tri[0][2] << endl;
		if (pointInTriangle(tri[0], tri[1], tri[2], kart->pos, &u, &v)) {
			g.crash = true;
		}
		if (!g.crash) 
		    kart->applyGravity();
		*/
	}


	//call keypress functions activated
	callControls();

	//lap system
	if (kart->pos[2] <= cones[0]->pos[2] && g.passCones[0] == false) {
		cones[0]->setColor(1.0, 1.0, 1.0);
		g.conecount++;
		g.passCones[0] = true;
	}

	if (kart->pos[2] <= cones[1]->pos[2] && g.passCones[0] == true && g.passCones[1] == false) {
		cones[1]->setColor(1.0, 1.0, 1.0);
		g.conecount++;
		g.passCones[1] = true;
	}

	if (kart->pos[0] >= cones[2]->pos[0] && kart->pos[2] <= cones[2]->pos[2] + 3.0f && g.passCones[1] == true 
			&& g.passCones[2] == false) {
		cones[2]->setColor(1.0, 1.0, 1.0);
		g.conecount++;
		g.passCones[2] = true;
	} 

	if (kart->pos[0] >= cones[3]->pos[0] && kart->pos[2] >= cones[3]->pos[2] && g.passCones[2] == true
			&& g.passCones[3] == false) {
		cones[3]->setColor(1.0, 1.0, 1.0);
		g.conecount++;
		g.passCones[3] = true;
	}

	if (kart->pos[2] >= cones[4]->pos[2] && g.passCones[3] == true && g.passCones[4] == false) {
		cones[4]->setColor(1.0, 1.0, 1.0);
		g.conecount++;
		g.passCones[4] = true;
	}

	if (kart->pos[2] >= cones[5]->pos[2] && g.passCones[4] == true && g.passCones[5] == false) {
		cones[5]->setColor(1.0, 1.0, 1.0);
		g.conecount++;
		g.passCones[5] = true;
	}


}

void drawShadow() {
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glDisable(GL_DEPTH_TEST);
	//darkness of shadow is here.
	glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glVertex2i(0, 0);
	glVertex2i(0, 1);
	glVertex2i(1, 1);
	glVertex2i(1, 0);
	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void renderShadows() {
	if (!g.shadows)
		return;
	//disable writing of frame buffer color components.
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	//disable writing into the depth buffer.
	glDepthMask(GL_FALSE);
	//enable culling of back-facing polygons.
	glEnable(GL_CULL_FACE);
	//enable stencil testing.
	glEnable(GL_STENCIL_TEST);
	//specify a depth offset for polygon rendering.
	glEnable(GL_POLYGON_OFFSET_FILL);
	//glPolygonOffset(-0.002f, 100.0f);
	glPolygonOffset(0.0f, 100.0f);
	//enable culling of front-facing polygons.
	glCullFace(GL_FRONT);
	//set front and back function and reference value for stencil testing.
	glStencilFunc(GL_ALWAYS, 0x0, 0xff);
	//set front and back stencil test actions.
	glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
	//----------------------------------------------------------------------
	//render the shadow surfaces.
	//cube.triShadowVolume();
	track->triShadowVolume();
	kart->triShadowVolume();
	//----------------------------------------------------------------------
	//enable culling of back-facing polygons.
	glCullFace(GL_BACK);
	//set front and back function and reference value for stencil testing.
	glStencilFunc(GL_ALWAYS, 0x0, 0xff);
	//set front and back stencil test actions.
	glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
	//----------------------------------------------------------------------
	//render the shadow surfaces.
	track->triShadowVolume();
	kart->triShadowVolume();
	//----------------------------------------------------------------------
	//reset for rendering to screen
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	//
	//set front and back function and reference value for stencil testing.
	glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
	//set front and back stencil test actions.
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	//draw the whole stencil buffer to the screen, revealing the shadows.
	drawShadow();
	//disable stencil testing.
	glDisable(GL_STENCIL_TEST);
}

void render(void)
{
	Rect r;
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//
	glViewport(0, 0, g.xres, g.yres);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, g.xres, 0, g.yres);
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glColor3f(1.0, 1.0, 1.0);

	//draw background (2D Mode)
	glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
	glBegin(GL_QUADS);
	glTexCoord2f(g.tex.xc[0], g.tex.yc[1]); glVertex2i(0, 0);
	glTexCoord2f(g.tex.xc[0], g.tex.yc[0]); glVertex2i(0, g.yres);
	glTexCoord2f(g.tex.xc[1], g.tex.yc[0]); glVertex2i(g.xres, g.yres);
	glTexCoord2f(g.tex.xc[1], g.tex.yc[1]); glVertex2i(g.xres, 0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glClear(GL_DEPTH_BUFFER_BIT);

	//3D mode
	glEnable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	gluPerspective(60.0f, g.aspectRatio, 0.1f, 1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//for documentation...
	Vec up = {0,1,0};
	gluLookAt(
			//kart->lastPos[0], kart->lastPos[1] + 2.0f, kart->lastPos[2] + 2.0f,
			g.cameraPosition[0], g.cameraPosition[1], g.cameraPosition[2],
			//track->vert[1][0], track->vert[1][1], track->vert[1][2],
			kart->nextPos[0], kart->nextPos[1], kart->nextPos[2],
			up[0], up[1], up[2]);
	glLightfv(GL_LIGHT0, GL_POSITION, g.lightPosition);

	//track->drawTexture();
	for (int i = 0; i < 10; i++) {
		cones[i]->draw();
	}
	track->draw();
	finish->draw(); 
	//bowser->draw();
	kart->draw();
	//renderShadows();
	//
	//switch to 2D mode
	//
	glViewport(0, 0, g.xres, g.yres);
	glMatrixMode(GL_MODELVIEW);   glLoadIdentity();
	glMatrixMode (GL_PROJECTION); glLoadIdentity();
	gluOrtho2D(0, g.xres, 0, g.yres);
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	r.bot = g.yres - 20;
	r.left = 10;
	r.center = 0;
	ggprint8b(&r, 16, 0x00000000, "W -> forward");
	ggprint8b(&r, 16, 0x00000000, "S -> backwards");
	ggprint8b(&r, 16, 0x00000000, "Arrow Keys -> right/left");
	ggprint8b(&r, 16, 0xFFFFFFFF, "kart x: %f", kart->pos[0]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "kart y: %f", kart->pos[1]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "kart z: %f", kart->pos[2]);

	//fps counter
	ggprint8b(&r, 16, 0xFFFFFFFF, "fps: %i", g.fps);
	ggprint8b(&r, 16, 0xFFFFFFFF, "conecount: %i", g.conecount);
	ggprint8b(&r, 16, 0xFFFFFFFF, "crash: %i", g.crash);
	ggprint8b(&r, 16, 0xFFFFFFFF, "x: %f", g.cx);
	ggprint8b(&r, 16, 0xFFFFFFFF, "y: %f", g.cy);
	ggprint8b(&r, 16, 0xFFFFFFFF, "z: %f", g.cz);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri0x: %f", track->vert[track->face[0][0]][0]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri0y: %f", track->vert[track->face[0][0]][1]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri0z: %f", track->vert[track->face[0][0]][2]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri1x: %f", track->vert[track->face[0][1]][0]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri1y: %f", track->vert[track->face[0][1]][1]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri1z: %f", track->vert[track->face[0][1]][2]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri2x: %f", track->vert[track->face[0][2]][0]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri2y: %f", track->vert[track->face[0][2]][1]);
	ggprint8b(&r, 16, 0xFFFFFFFF, "tri2z: %f", track->vert[track->face[0][2]][2]);
	glPopAttrib();
}

void callControls() {
	//exit game
	if (g.keypress[XK_Escape]) {
		g.done = 1;
	}
	//drive forward
	if (g.keypress[XK_w]) {
		kart->vel[0] += 0.002;
		kart->vel[2] += 0.002;
		if (kart->vel[0] >= 0.3) {
			kart->vel[0] = 0.3;
		}
		if (kart->vel[2] >= 0.3) {
			kart->vel[2] = 0.3;
		}
		kart->pos[0] += kart->dir[0] * kart->vel[0];
		kart->pos[2] += kart->dir[2] * kart->vel[2];
	} else {
		kart->vel[0] -= 0.01;
		kart->vel[2] -= 0.01;
		if (kart->vel[0] <= 0) {
			kart->vel[0] = 0;
		}
		if (kart->vel[2] <= 0) {
			kart->vel[2] = 0;
		}
		kart->pos[0] += kart->dir[0] * kart->vel[0];
		kart->pos[2] += kart->dir[2] * kart->vel[2];
	}
	//turn left
	if (g.keypress[XK_a]) {
		kart->Hangle -= 0.02f;
		kart->dir[0] = sin(kart->Hangle);
		kart->dir[2] = -cos(kart->Hangle);
		kart->rotate(0, 1.147, 0);
	}
	//turn right
	if (g.keypress[XK_d]) {
		kart->Hangle += 0.02f;
		kart->dir[0] = sin(kart->Hangle);
		kart->dir[2] = -cos(kart->Hangle);
		kart->rotate(0, -1.147, 0);
	}
	//getting last position for the camera
	kart->nextPos[0] = kart->pos[0] + kart->dir[0];
	kart->nextPos[1] = kart->pos[1] + kart->dir[1];
	kart->nextPos[2] = kart->pos[2] + kart->dir[2];

	kart->lastPos[0] = kart->pos[0] - kart->dir[0];
	kart->lastPos[1] = kart->pos[1] - kart->dir[1] + 0.5f;
	kart->lastPos[2] = kart->pos[2] - kart->dir[2];

	g.cameraPosition[0] = kart->lastPos[0];
	g.cameraPosition[1] = kart->lastPos[1];
	g.cameraPosition[2] = kart->lastPos[2];

}

