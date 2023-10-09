/**************************************************************************/
/*  prime_egl.cpp                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#if defined(GLES3_ENABLED) && defined(EGL_ENABLED)

#include "prime_egl.h"

#include "core/string/print_string.h"

// Runs inside a child. Exiting will not quit the engine.
Error PrimeEGL::_create_context() {
	if (!gladLoaderLoadEGL(nullptr)) {
		print_verbose("Unable to bootstrap EGL.");
		return ERR_UNAVAILABLE;
	}

	EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	if (!eglInitialize(egl_display, NULL, NULL)) {
		print_verbose("Can't initialize an EGL display.");
		return ERR_UNAVAILABLE;
	}

	if (!gladLoaderLoadEGL(egl_display)) {
		print_verbose("Unable to load EGL.");
		return ERR_UNAVAILABLE;
	}

	if (!eglBindAPI(EGL_OPENGL_API)) {
		print_verbose("OpenGL not supported");
		return ERR_UNAVAILABLE;
	}

	EGLint attribs[] = {
		EGL_RED_SIZE,
		1,
		EGL_BLUE_SIZE,
		1,
		EGL_GREEN_SIZE,
		1,
		EGL_DEPTH_SIZE,
		24,
		EGL_NONE,
	};

	EGLConfig egl_config;
	EGLint config_count = 0;

	eglChooseConfig(egl_display, attribs, &egl_config, 1, &config_count);

	if (config_count == 0) {
		print_verbose("No EGL config found.");
		return ERR_UNAVAILABLE;
	}

	EGLint context_attribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 3,
		EGL_NONE
	};

	EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);

	if (egl_context == EGL_NO_CONTEXT) {
		print_verbose("Unable to create an EGL context, GPU detection skipped.");
		quick_exit(1);
	}

	if (!eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context)) {
		print_verbose("Unable to set current context.");
		quick_exit(1);
	}

	return OK;
}

GLProc PrimeEGL::_get_gl_proc(const char *p_proc) {
	return (GLProc)eglGetProcAddress(p_proc);
}

#endif // defined(EGL_ENABLED) && defined(GLES3_ENABLED)
