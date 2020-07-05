#include <GL/glx.h>
#include <GL/gl.h>
#include <unistd.h>
#include <iostream>

#define GLX_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB		0x2092
typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

int main (int argc, char ** argv){
	Display *dpy = XOpenDisplay(0);

	int nelements;
	GLXFBConfig *fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), 0, &nelements);

	static int attributeList[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, None };
	XVisualInfo *vi = glXChooseVisual(dpy, DefaultScreen(dpy),attributeList);

	XSetWindowAttributes swa;
	swa.colormap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask;
	Window win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, 100, 100, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa);

	XMapWindow (dpy, win);

	//oldstyle context:
	//	GLXContext ctx = glXCreateContext(dpy, vi, 0, GL_TRUE);

	std::cout << "glXCreateContextAttribsARB " << (void*) glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB") << std::endl;
	GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (GLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

	int attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 0,
		0};

	GLXContext ctx = glXCreateContextAttribsARB(dpy, *fbc, 0, true, attribs);

	glXMakeCurrent (dpy, win, ctx);

	for (;;) {
		glClearColor (rand() / double(RAND_MAX), rand() / double(RAND_MAX), rand() / double(RAND_MAX), 1);
		glClear (GL_COLOR_BUFFER_BIT);
		glXSwapBuffers (dpy, win);
	}
	ctx = glXGetCurrentContext(); 
	glXDestroyContext(dpy, ctx); 
}
