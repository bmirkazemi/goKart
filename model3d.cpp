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
#include "fonts.h"
#include <iostream>
using namespace std;

typedef float Flt;
typedef Flt Vec[3];
typedef Flt	Matrix[4][4];
typedef int iVec[3];
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
const Flt DTR = 1.0 / 180.0 * PI;

Display *dpy;
Window win;
GLXContext glc;

void init(void);
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_resize(XEvent *e);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void physics(void);
void render(void);
void callControls(void);

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
Image img[1] = {"./kart/bg1.jpg"};

class Texture {
    public:
    Image *backImage;
    GLuint backTexture;
    float xc[2];
    float yc[2];
};

class Global {
public:
	int done;
	char keypress[65536];
	Flt gravity;
	int xres, yres;
	Flt aspectRatio;
	Texture tex;
	GLfloat lightPosition[4];
	Vec cameraPosition;
	Vec spot;
	bool shadows;
	Global() {
		for (int i = 0; i < 65336; i++) {
			keypress[i] = 0;
		}
		shadows = true;
		gravity = 0.01;
		xres=640;
		yres=480;
		aspectRatio = (GLfloat)xres / (GLfloat)yres;
		//light is up high, right a little, toward a little
		VecMake(0.0, 2.0, -10.0, cameraPosition);
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
	Vec *norm;
	iVec *face;
	Matrix m;
	int nverts;
	int nfaces;
	Vec dir;
	Vec pos, vel, rot;
	Vec color;
	Vec a[1000][3];
	Vec b[1000][3];
	int nshadows;
public:
	~Object() {
		delete [] vert;
		delete [] face;
		delete [] norm;
	}
	Object(int nv, int nf) {
		forward = reverse = false;
		leftRotate = rightRotate = 0;
		velocity = 0;
		vert = new Vec[nv];
		face = new iVec[nf];
		norm = new Vec[nf];
		nverts = nv;
		nfaces = nf;
		VecZero(pos);
		VecZero(vel);
		VecZero(rot);
		VecMake(.9, .9, 0, color);
		g.identity33(m);
		dir[0] = 0.0 - pos[0];
		dir[1] = 0.0 - pos[1];
		dir[2] = 0.0 - pos[2];
	}
	void setColor(float r, float g, float b) {
		VecMake(r, g, b, color);
	}
	void setVert(Vec v, int i) {
		VecMake(v[0], v[1], v[2], vert[i]);
	}
	void setFace(iVec f, int i) {
		VecMake(f[0], f[2], f[1], face[i]);
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
	void setup(int type) {
		if (type == 1) {
			//type 1 = cube
			/*
			//         top
			//       0------1
			//      /.      .\
			//     / .      . \
			//    3------------2
			//    |  .      .  |
			//    |  4------5  |
			//    | /        \ |
			//    |/          \|
			//    7------------6
			//        bottom
			*/
			VecMake(-1, .1,-1, vert[0]);
			VecMake( 1, 1,-1, vert[1]);
			VecMake( 1, 1, 1, vert[2]);
			VecMake(-1, .1, 1, vert[3]);
			VecMake(-1,-1,-1, vert[4]);
			VecMake( 1,-1,-1, vert[5]);
			VecMake( 1,-1, 1, vert[6]);
			VecMake(-1,-1, 1, vert[7]);
			//opengl default for front facing
			//is counter-clockwise.
			nfaces=0;
			//top
			VecMake( 0, 2, 1, face[ 0]); VecMake( 0, 1, 1, norm[ 0]);
			VecMake( 0, 3, 2, face[ 1]); VecMake( 0, 1, 0, norm[ 1]);
			nfaces+=2;
			//right
			VecMake( 1, 2, 6, face[ 2]); VecMake( 1, 0, 1, norm[ 2]);
			VecMake( 1, 6, 5, face[ 3]); VecMake( 1, 0, 0, norm[ 3]);
			nfaces+=2;
			//bottom
			VecMake( 4, 5, 6, face[ 4]); VecMake( 0,-1, 1, norm[ 4]);
			VecMake( 4, 6, 7, face[ 5]); VecMake( 0,-1, 0, norm[ 5]);
			nfaces+=2;
			//left
			VecMake( 0, 4, 7, face[ 6]); VecMake(-1, 0, 0, norm[ 6]);
			VecMake( 0, 7, 3, face[ 7]); VecMake(-1, 0, 0, norm[ 7]);
			nfaces+=2;
			//front
			VecMake( 2, 3, 7, face[ 8]); VecMake( 0, 0, 1, norm[ 8]);
			VecMake( 2, 7, 6, face[ 9]); VecMake( 0, 0, 1, norm[ 9]);
			nfaces+=2;
			//back
			VecMake( 0, 1, 5, face[10]); VecMake( 0, 0,-1, norm[10]);
			VecMake( 0, 5, 4, face[11]); VecMake( 0, 0,-1, norm[11]);
			nfaces+=2;
			scale(0.5);
			translate(1.1,2.0,0);
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
	void doRotate(Vec r) {
		if (r[0] != 0.0)
			glRotatef(r[0], 1.0f, 0.0f, 0.0f);
		if (r[1] != 0.0)
			glRotatef(r[1], 0.0f, 1.0f, 0.0f);
		if (r[2] != 0.0)
			glRotatef(r[2], 0.0f, 0.0f, 1.0f);
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
				glColor3fv(color);
				glNormal3fv(norm);
				glVertex3fv(tv[0]);
				glVertex3fv(tv[1]);
				glVertex3fv(tv[2]);
			glEnd();
			glPopMatrix();
		}
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
} cube(8, 12), *track, *kart, *bowser;

int main(void)
{
	g.done=0;
	initXWindows();
	init_opengl();
	init();
	while (g.done == 0) {
		while (XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			check_resize(&e);
			check_mouse(&e);
			check_keys(&e);
		}
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
    setup_screen_res(640, 480);
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        printf("\n\tcannot connect to X server\n\n");
        exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if (vi == NULL) {
        printf("\n\tno appropriate visual found\n\n");
        exit(EXIT_FAILURE);
    }
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                        StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
                            vi->depth, InputOutput, vi->visual,
                            CWColormap | CWEventMask, &swa);
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
	//track
	Object *buildModel(const char *mname);
	track = buildModel("./kart/track.obj");
	track->scale(3);
	track->translate(-1.0, -2, 0);
	track->rotate(0, 90, 0);
	track->setColor(0.2,0.2,0.2);	
	//mario
	Object *buildModel(const char *mname);
	kart = buildModel("./kart/kartuse.obj");
	kart->scale(0.5);
	kart->rotate(0, 180, 0);
	kart->translate(-0.5, 0, 0);
	kart->setColor(.8,0,.9);
	
	//bowser
	Object *buildModel(const char *mname);
	bowser = buildModel("./bowser/kk_kart.obj");
	bowser->scale(0.5);
	bowser->rotate(0, 180, 0);
	bowser->translate(-0.5, 0, -3.0);
	bowser->setColor(.4,.2,.9);
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
	 g.tex.backImage = &img[0];
	 //create opengl texture elements
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
	GLint stencilBits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
	if (stencilBits < 1) {
		printf("No stencil buffer on this computer.\n");
		printf("Exiting program.\n");
		exit(0);
	}
	//------------------------------------------------------------------------
}

void check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != g.xres || xce.height != g.yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}

Object *buildModel(const char *mname)
{
	char line[200];
	Vec *vert=NULL;  //vertices in list
	iVec *face=NULL; //3 indicies per face
	int nv=0, nf=0;
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
	//count all faces
	int iface[4];
	fseek(fpi, 0, SEEK_SET);	
	while (fgets(line, 100, fpi) != NULL) {
		if (line[0] == 'f' && line[1] == ' ') {
			sscanf(line+1,"%i %i %i", &iface[0], &iface[1], &iface[2]);
			nf++;
		}
	}
	face = new iVec[nf];
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
	//second pass, read all faces
	int comment=0;
	nf=0;
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
			sscanf(line+1,"%i %i %i", &iface[0], &iface[1], &iface[2]);
			face[nf][0] = iface[1]-1;
			face[nf][1] = iface[0]-1;
			face[nf][2] = iface[2]-1;
			nf++;
		}
	}
	fclose(fpi);
	printf("nverts: %i   nfaces: %i\n", nv, nf);
	Object *o = new Object(nv, nf);
	for (int i=0; i<nv; i++) {
		o->setVert(vert[i], i);
	}
	//opengl default for front facing is counter-clockwise.
	//now build the triangles...
	for (int i=0; i<nf; i++) {
		o->setFace(face[i], i);
	}
	delete [] vert;
	delete [] face;
	return o;
}


void check_mouse(XEvent *e)
{
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button is down
		}
		if (e->xbutton.button==3) {
			//Right button is down
		}
	}
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		//Mouse moved
		savex = e->xbutton.x;
		savey = e->xbutton.y;
	}
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


void box(float w1, float h1, float d1)
{
	float w=w1*0.5;
	float d=d1*0.5;
	float h=h1*0.5;
	//notice the normals being set
	glBegin(GL_QUADS);
		//top
		glNormal3f( 0.0f, 1.0f, 0.0f);
		glVertex3f( w, h,-d);
		glVertex3f(-w, h,-d);
		glVertex3f(-w, h, d);
		glVertex3f( w, h, d);
		// bottom
		glNormal3f( 0.0f, -1.0f, 0.0f);
		glVertex3f( w,-h, d);
		glVertex3f(-w,-h, d);
		glVertex3f(-w,-h,-d);
		glVertex3f( w,-h,-d);
		// front
		glNormal3f( 0.0f, 0.0f, 1.0f);
		glVertex3f( w, h, d);
		glVertex3f(-w, h, d);
		glVertex3f(-w,-h, d);
		glVertex3f( w,-h, d);
		// back
		glNormal3f( 0.0f, 0.0f, -1.0f);
		glVertex3f( w,-h,-d);
		glVertex3f(-w,-h,-d);
		glVertex3f(-w, h,-d);
		glVertex3f( w, h,-d);
		// left side
		glNormal3f(-1.0f, 0.0f, 0.0f);
		glVertex3f(-w, h, d);
		glVertex3f(-w, h,-d);
		glVertex3f(-w,-h,-d);
		glVertex3f(-w,-h, d);
		// Right side
		glNormal3f( 1.0f, 0.0f, 0.0f);
		glVertex3f( w, h,-d);
		glVertex3f( w, h, d);
		glVertex3f( w,-h, d);
		glVertex3f( w,-h,-d);
		glEnd();
	glEnd();
}

void drawFloor()
{
	float w = 6.0*0.5;
	float d = 4.0*0.5;
	float h = -1.0;
	glBegin(GL_QUADS);
		//top
		glNormal3f( 0.0f, 1.0f, 0.0f);
		glColor3f(0.4f, 0.4f, 0.3f);
		glVertex3f( w, h,-d);
		glVertex3f(-w, h,-d);
		glColor3f(0.6f, 0.6f, 0.5f);
		glVertex3f(-w, h, d);
		glVertex3f( w, h, d);
	glEnd();
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
	Vec camUpdate = {kart->pos[0], kart->pos[1] + 0.7f, kart->pos[2] + 2.0f};
	VecMake(camUpdate[0], camUpdate[1], camUpdate[2], g.cameraPosition);
	
	//apply gravity and physics to kart
	kart->applyGravity();
	kart->translate(-kart->leftRotate * 0.01 + kart->rightRotate * 0.01, 0, -kart->velocity);
	
	bowser->applyGravity();
	if(bowser->pos[1] <= track->pos[1]) {
		bowser->pos[1] = track->pos[1];
	}

	//check if kart collided with track
	if(kart->pos[1] <= track->pos[1]) {
		kart->pos[1] = track->pos[1];
	}
	
	//call keypress functions activated
	callControls();
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
	glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
	glBegin(GL_QUADS);
	glTexCoord2f(g.tex.xc[0], g.tex.yc[1]); glVertex2i(0, 0);
	glTexCoord2f(g.tex.xc[0], g.tex.yc[0]); glVertex2i(0, g.yres);
	glTexCoord2f(g.tex.xc[1], g.tex.yc[0]); glVertex2i(g.xres, g.yres);
	glTexCoord2f(g.tex.xc[1], g.tex.yc[1]); glVertex2i(g.xres, 0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glClear(GL_DEPTH_BUFFER_BIT);
	//
	//3D mode
	glEnable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	gluPerspective(45.0f, g.aspectRatio, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//for documentation...
	Vec up = {0,1,0};
	kart->dir[0] = -kart->leftRotate * 0.05;
	kart->dir[0] = kart->rightRotate * 0.01;
	VecMake(kart->pos[0] + kart->dir[0], kart->pos[1] + kart->dir[1], kart->pos[2] + kart->dir[2], g.spot);
	g.spot[0]-= kart->leftRotate * 0.01;
	g.spot[0]+= kart->rightRotate * 0.01;
	//
	gluLookAt(
		g.cameraPosition[0], g.cameraPosition[1], g.cameraPosition[2],
		g.spot[0], g.spot[1] + 0.5f, g.spot[2],
		up[0], up[1], up[2]);
	glLightfv(GL_LIGHT0, GL_POSITION, g.lightPosition);
	//
	track->draw();
	//bowser->draw();
	kart->draw();
	renderShadows();
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
	ggprint8b(&r, 16, 0x00887766, "3480 Demo");
	ggprint8b(&r, 16, 0x00887766, "W forward, S backward, A turn left, D turn right");
	//
	glPopAttrib();
}

void callControls() {
	//exit game
	if (g.keypress[XK_Escape]) {
		g.done = 1;
	}
	//check for stopped
	if (kart->velocity == 0) {
		kart->forward = false;
		kart->reverse = false;
	}	
	//drive forward
	if (g.keypress[XK_w]) {
		kart->reverse = false;
		kart->forward = true;
		kart->velocity += 0.001;
		if (kart->velocity >= 1.0) {
			kart->velocity = 1.0;
		}
	} else if (kart->reverse == false) {
		kart->velocity -= 0.005;
		if (kart->velocity <= 0) {
			kart->velocity = 0;
		}
	}
	
	//drive reverse
	if (g.keypress[XK_s]) {
		kart->forward = false;
		kart->reverse = true;
		kart->velocity -= 0.001;
	} else if (kart->forward == false) {
		kart->velocity += 0.005;
		if (kart->velocity >= 0) {
			kart->velocity = 0;
		}
	}
	//turn left
	if (g.keypress[XK_a]) {
		if (kart->forward == true) {
			if (kart->leftRotate < 10) {
				kart->rotate(0, 0.5, 0);
				kart->leftRotate += 0.5;
			} else {
				kart->leftRotate = 10;
			}
		}
	} else if (kart->leftRotate > 0) {
		kart->rotate(0, -0.5, 0);
		kart->leftRotate -= 0.5;
	}
	//turn right
	if (g.keypress[XK_d]) {
		if (kart->forward == true) {
			if (kart->rightRotate < 10) {
				kart->rotate(0, -0.5, 0);
				kart->rightRotate += 0.5;
			} else {
				kart->rightRotate = 10;
			}
		}
	} else if (kart->rightRotate > 0) {
		kart->rotate(0, 0.5, 0);
		kart->rightRotate -= 0.5;
	}	
}


