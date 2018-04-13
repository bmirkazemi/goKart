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
#include <time.h>
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
void vecScale(Vec v, Flt scale);
void drawSmoke(void);
void make_a_smoke(void);
void smokephysics(void);
void vecSub(Vec v0, Vec v1, Vec dest);
bool pointInTriangle(Vec tri[3], Vec p, Flt *u, Flt *v);
Flt vecDotProduct(Vec v0, Vec v1);
void vecCrossProduct(Vec v0, Vec v1, Vec dest);
Flt vecLength(Vec vec);

const Vec upv = {0.0, 1.0, 0.0};
const int MAX_SMOKES = 200000;

class Smoke {
    public:
	Vec pos;
	Vec vert[16];
	Flt radius;
	int n;
	struct timespec tstart;
	Flt maxtime;
	Flt alpha;
	Flt distance;
	Smoke() {

	}
};

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
	char keypress[65536];
	Flt gravity;
	int xres, yres;
	Flt aspectRatio;
	Texture tex;
	GLfloat lightPosition[4];
	Vec cameraPosition;
	Vec spot;
	bool shadows;
	Matrix m;
	int circling;
	int sorting;
	int billboarding;
	struct timespec smokeStart, smokeTime;
	Smoke *smoke;
	int nsmokes;
	bool wind;
	Flt windrate;
	Global() {
	    for (int i = 0; i < 65336; i++) {
		keypress[i] = 0;
	    }
	    shadows = true;
	    gravity = 0.01;
	    xres=640;
	    yres=480;
	    aspectRatio = (GLfloat)xres / (GLfloat)yres;
	    VecMake(0.0, 20.0, -1.0, cameraPosition);
	    VecMake(100.0f, 240.0f, 40.0f, lightPosition);
	    lightPosition[3] = 1.0f;
	    clock_gettime(CLOCK_REALTIME, &smokeStart);
	    nsmokes = 0;
	    smoke = new Smoke[MAX_SMOKES];
	    windrate = 0.05f;
	}
	void identity33(Matrix m) {
	    m[0][0] = m[1][1] = m[2][2] = 1.0f;
	    m[0][1] = m[0][2] = m[1][0] = m[1][2] = m[2][0] = m[2][1] = 0.0f;
	}
	~Global() {
	    if (smoke)
		delete [] smoke;
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
	    delete [] face;
	    delete [] norm;
	}
	Object(int nv, int nf) {
	    Hangle = Vangle = 0;
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
} *track, *kart, *bowser;

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
    track = buildModel("./assets/tracktext.obj");
   // track->scale(3);
   // track->translate(-1.0, -2, 0);
   // track->rotate(0, 90, 0);
   // track->setColor(0.2,0.2,0.2);
    //mario
    Object *buildModel(const char *mname);
    kart = buildModel("./mario/karttex.obj");
    kart->scale(0.2);
    kart->rotate(0, 180, 0);
    kart->translate(-0.5, 0, 0);
    kart->setColor(.8,0,.9);
    //bowser
    Object *buildModel(const char *mname);
    bowser = buildModel("./bowser/bkart.obj");
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

void make_view_matrix(const Vec p1, const Vec p2, Matrix m)
{
    //Line between p1 and p2 form a LOS Line-of-sight.
    //A rotation matrix is built to transform objects to this LOS.
    //Diana Gruber  http://www.makegames.com/3Drotation/
    m[0][0]=m[1][1]=m[2][2]=1.0f;
    m[0][1]=m[0][2]=m[1][0]=m[1][2]=m[2][0]=m[2][1]=0.0f;
    Vec out = { p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2] };
    //
    Flt l1, len = out[0]*out[0] + out[1]*out[1] + out[2]*out[2];
    if (len == 0.0f) {
	VecMake(0.0f,0.0f,1.0f,out);
    } else {
	l1 = 1.0f / sqrtf(len);
	out[0] *= l1;
	out[1] *= l1;
	out[2] *= l1;
    }
    m[2][0] = out[0];
    m[2][1] = out[1];
    m[2][2] = out[2];
    Vec up = { -out[1] * out[0], upv[1] - out[1] * out[1], -out[1] * out[2] };
    //
    len = up[0]*up[0] + up[1]*up[1] + up[2]*up[2];
    if (len == 0.0f) {
	VecMake(0.0f,0.0f,1.0f,up);
    }
    else {
	l1 = 1.0f / sqrtf(len);
	up[0] *= l1;
	up[1] *= l1;
	up[2] *= l1;
    }
    m[1][0] = up[0];
    m[1][1] = up[1];
    m[1][2] = up[2];
    //make left vector.
    VecCross(up, out, m[0]);
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
    Vec camUpdate = {kart->pos[0], kart->pos[1] + 1.0f, kart->pos[2] + 3.0f};
    VecMake(camUpdate[0], camUpdate[1], camUpdate[2], g.cameraPosition);

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

    //call keypress functions activated
    callControls();
    smokephysics();

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
    
    //
    
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
    
    
    glBindTexture(GL_TEXTURE_2D, g.tex.roadTexture);
	glBegin(GL_QUADS);
	
	glTexCoord2f(g.tex.xc[0], g.tex.yc[1]); glVertex3f(track->vert[3][0], track->vert[3][1], track->vert[3][2]);
    glTexCoord2f(g.tex.xc[0], g.tex.yc[0]); glVertex3f(track->vert[2][0], track->vert[2][1], track->vert[2][2]);
    glTexCoord2f(g.tex.xc[1], g.tex.yc[0]); glVertex3f(track->vert[0][0], track->vert[0][1], track->vert[0][2]);
    glTexCoord2f(g.tex.xc[1], g.tex.yc[1]); glVertex3f(track->vert[1][0], track->vert[1][1], track->vert[1][2]);
	
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
    gluLookAt(
	    //kart->lastPos[0], kart->lastPos[1] + 2.0f, kart->lastPos[2] + 2.0f,
	    g.cameraPosition[0], g.cameraPosition[1], g.cameraPosition[2],
	    kart->nextPos[0], kart->nextPos[1], kart->nextPos[2],
	    up[0], up[1], up[2]);
    glLightfv(GL_LIGHT0, GL_POSITION, g.lightPosition);
    //
    track->draw();
    bowser->draw();
    kart->draw();
    //renderShadows();
    //drawSmoke();
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
    //ggprint8b(&r, 16, 0x00000000, "smoke[0] x: %f", g.smoke[0].pos[0]);
    //ggprint8b(&r, 16, 0x00000000, "smoke[0] y: %f", g.smoke[0].pos[1]);
    //ggprint8b(&r, 16, 0x00000000, "smoke[0] z: %f", g.smoke[0].pos[2]);
    for (int i = 0; i < track->nverts; i++) {
		ggprint8b(&r, 16, 0xFFFFFFFFF, "track vert %i = %f %f %f", i, track->vert[i][0], track->vert[i][1], track->vert[i][2]);
	}
    
    

    //
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

void vecScale(Vec v, Flt scale) {
    v[0] *= scale;
    v[1] *= scale;
    v[2] *= scale;
}

void drawSmoke()
{
    bool swapped;
    for (int i = 0; i < g.nsmokes; i++) {
	g.smoke[i].distance = sqrt(pow((g.smoke[i].pos[0] - g.cameraPosition[0]), 2) + pow((g.smoke[i].pos[1] - g.cameraPosition[1]), 2) + pow((g.smoke[i].pos[2] - g.cameraPosition[2]), 2));
    }

    if (g.sorting) {
	for (int i = 0; i < g.nsmokes - 1; i++) {
	    swapped = false;
	    for (int j = 0; j < g.nsmokes - 1; j++) {
		if (g.smoke[j].distance < g.smoke[j+1].distance) {
		    swap(g.smoke[j], g.smoke[j+1]);
		    swapped = true;
		}
	    }
	    if (swapped == false) {
		break;
	    }
	}
    }
    //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i=0; i<g.nsmokes; i++) {
	glPushMatrix();
	glTranslatef(g.smoke[i].pos[0],g.smoke[i].pos[1],g.smoke[i].pos[2]);
	if (g.billboarding) {
	    Vec v;
	    v[0] = g.smoke[i].pos[0] - g.cameraPosition[0];
	    v[1] = g.smoke[i].pos[1] - g.cameraPosition[1];
	    v[2] = g.smoke[i].pos[2] - g.cameraPosition[2];
	    Vec z = {0.0f, 0.0f, 0.0f};
	    make_view_matrix(z, v, g.m);

	    float mat[16];
	    mat[ 0] = g.m[0][0];
	    mat[ 1] = g.m[0][1];
	    mat[ 2] = g.m[0][2];
	    mat[ 4] = g.m[1][0];
	    mat[ 5] = g.m[1][1];
	    mat[ 6] = g.m[1][2];
	    mat[ 8] = g.m[2][0];
	    mat[ 9] = g.m[2][1];
	    mat[10] = g.m[2][2];
	    mat[ 3] = mat[ 7] = mat[11] = mat[12] = mat[13] = mat[14] = 0.0f;
	    mat[15] = 1.0f;
	    glMultMatrixf(mat);
	}
	glColor4ub(255, 0, 0, (unsigned char)g.smoke[i].alpha);
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0, 0.0, 1.0);
	for (int j=0; j<g.smoke[i].n; j++) {
	    //each vertex of the smoke
	    //glVertex3fv(g.smoke[i].vert[j]);
	    VecNormalize(g.smoke[i].vert[j]);
	    vecScale(g.smoke[i].vert[j], g.smoke[i].radius);
	    glVertex3fv(g.smoke[i].vert[j]);
	}
	glEnd();
	glPopMatrix();
    }
    glDisable(GL_BLEND);
}

void make_a_smoke()
{
    if (g.nsmokes < MAX_SMOKES) {
	Smoke *s = &g.smoke[g.nsmokes];
	s->pos[0] = kart->pos[2];
	s->pos[2] = kart->pos[0];
	s->pos[1] = kart->pos[1];
	s->radius = 0.01;
	s->n = rand() % 5 + 5;
	Flt angle = 0.0;
	Flt inc = (PI*2.0) / (Flt)s->n;
	for (int i=0; i<s->n; i++) {
	    s->vert[i][0] = cos(angle) * s->radius;
	    s->vert[i][1] = sin(angle) * s->radius;
	    s->vert[i][2] = 0.0;
	    angle += inc;
	}
	s->maxtime = 8.0;
	s->alpha = 100.0;
	clock_gettime(CLOCK_REALTIME, &s->tstart);
	++g.nsmokes;
    }
}

void smokephysics() {
    clock_gettime(CLOCK_REALTIME, &g.smokeTime);
    double d = timeDiff(&g.smokeStart, &g.smokeTime);
    if (d > 0.05) {
	//time to make another smoke particle
	for (int i = 0; i < 100; i++) {
	    make_a_smoke();
	}
	timeCopy(&g.smokeStart, &g.smokeTime);
    }
    //move smoke particles
    for (int i=0; i<g.nsmokes; i++) {
	//smoke rising
	g.smoke[i].pos[1] += 0.015;
	g.smoke[i].pos[1] += ((g.smoke[i].pos[1]*0.24) * (rnd() * 0.075));
	//expand particle as it rises
	// g.smoke[i].radius += g.smoke[i].pos[1]*0.002;
	//wind might blow particle
	//if (g.smoke[i].pos[1] > 10.0) {
	//    g.smoke[i].pos[0] -= rnd() * 0.1;
	//}
	//if (g.wind == true) {
	//    g.smoke[i].pos[0] += g.windrate;
	//}
    }
    //check for smoke out of time
    int i=0;
    while (i < g.nsmokes) {
	struct timespec bt;
	clock_gettime(CLOCK_REALTIME, &bt);
	double d = timeDiff(&g.smoke[i].tstart, &bt);
	if (d > g.smoke[i].maxtime - 3.0) {
	    g.smoke[i].alpha *= 0.95;
	    if (g.smoke[i].alpha < 1.0)
		g.smoke[i].alpha = 1.0;
	}
	if (d > g.smoke[i].maxtime) {
	    //delete this smoke
	    --g.nsmokes;
	    g.smoke[i] = g.smoke[g.nsmokes];
	    continue;
	}
	++i;
    }
}

