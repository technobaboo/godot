/**************************************************************************/
/*  detect_prime_x11.cpp                                                  */
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

#if defined(X11_ENABLED) && defined(GLES3_ENABLED)

#include "prime_x11.h"

#include "core/string/print_string.h"

#include "thirdparty/glad/glad/glx.h"

#ifdef SOWRAP_ENABLED
#include "x11/dynwrappers/xlib-so_wrap.h"
#else
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display *, GLXFBConfig, GLXContext, Bool, const int *);

Error PrimeX11::_create_context() {
	Display *x11_display = XOpenDisplay(nullptr);
	Window x11_window;
	GLXContext glx_context;

	static int visual_attribs[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER, true,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DEPTH_SIZE, 24,
		None
	};

	if (gladLoaderLoadGLX(x11_display, XScreenNumberOfScreen(XDefaultScreenOfDisplay(x11_display))) == 0) {
		print_verbose("Unable to load GLX");
		return ERR_UNAVAILABLE;
	}
	int fbcount;
	GLXFBConfig fbconfig = nullptr;
	XVisualInfo *vi = nullptr;

	XSetWindowAttributes swa;
	swa.event_mask = StructureNotifyMask;
	swa.border_pixel = 0;
	unsigned long valuemask = CWBorderPixel | CWColormap | CWEventMask;

	GLXFBConfig *fbc = glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs, &fbcount);
	if (!fbc) {
		return ERR_UNAVAILABLE;
	}

	vi = glXGetVisualFromFBConfig(x11_display, fbc[0]);

	fbconfig = fbc[0];

	static int context_attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
	};

	glx_context = glXCreateContextAttribsARB(x11_display, fbconfig, nullptr, true, context_attribs);

	swa.colormap = XCreateColormap(x11_display, RootWindow(x11_display, vi->screen), vi->visual, AllocNone);
	x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi->screen), 0, 0, 10, 10, 0, vi->depth, InputOutput, vi->visual, valuemask, &swa);

	if (!x11_window) {
		return ERR_UNAVAILABLE;
	}

	glXMakeCurrent(x11_display, x11_window, glx_context);
	XFree(vi);

	return OK;
}

GLProc PrimeX11::_get_gl_proc(const char *p_proc) {
	return (GLProc)glXGetProcAddressARB((GLubyte *)p_proc);
}

#endif // X11_ENABLED && GLES3_ENABLED
