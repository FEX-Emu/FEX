#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1
#define EGL_EGLEXT_PROTOTYPES 1

#include "glcorearb.h"

#include <GL/glx.h>
#include <GL/glxext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#undef GL_ARB_viewport_array
#include "glcorearb.h"