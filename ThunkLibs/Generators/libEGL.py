#!/usr/bin/python3
from ThunkHelpers import *

lib("libEGL")

fn("EGLBoolean eglBindAPI(EGLenum)")
fn("EGLBoolean eglChooseConfig(EGLDisplay, const EGLint*, void**, EGLint, EGLint*)")
fn("EGLBoolean eglDestroyContext(EGLDisplay, EGLContext)")
fn("EGLBoolean eglDestroySurface(EGLDisplay, EGLSurface)")
fn("EGLBoolean eglInitialize(EGLDisplay, EGLint*, EGLint*)")
fn("EGLBoolean eglMakeCurrent(EGLDisplay, EGLSurface, EGLSurface, EGLContext)")
fn("EGLBoolean eglQuerySurface(EGLDisplay, EGLSurface, EGLint, EGLint*)")
fn("EGLBoolean eglSurfaceAttrib(EGLDisplay, EGLSurface, EGLint, EGLint)")
fn("EGLBoolean eglSwapBuffers(EGLDisplay, EGLSurface)")
fn("EGLBoolean eglTerminate(EGLDisplay)")
fn("EGLint eglGetError()")
fn("void* eglCreateContext(EGLDisplay, EGLConfig, EGLContext, const EGLint*)")
fn("void* eglCreateWindowSurface(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*)")
fn("void* eglGetCurrentContext()")
fn("void* eglGetCurrentDisplay()")
fn("void* eglGetCurrentSurface(EGLint)")
fn("void* eglGetDisplay(EGLNativeDisplayType)")


Generate()