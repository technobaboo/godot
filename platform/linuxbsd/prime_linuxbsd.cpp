/**************************************************************************/
/*  prime_linuxbsd.cpp                                                    */
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

#include "prime_linuxbsd.h"

#ifdef GLES3_ENABLED

#include "core/string/print_string.h"

#include <thirdparty/glad/glad/gl.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// To prevent shadowing warnings.
#undef glGetString

int PrimeLinuxBSD::detect_gpu() {
	pid_t p;

	Device devices[4] = {};

	for (int i = 0; i < 4; ++i) {
		int fdset[2];

		if (pipe(fdset) == -1) {
			print_verbose("Failed to pipe(), using default GPU");
			return 0;
		}

		// Fork so the driver initialization can crash without taking down the engine.
		p = fork();

		if (p > 0) {
			// Main thread

			int stat_loc = 0;
			char string[201];
			string[200] = '\0';

			close(fdset[1]);

			waitpid(p, &stat_loc, 0);

			if (!stat_loc) {
				// No need to do anything complicated here. Anything less than
				// PIPE_BUF will be delivered in one read() call.
				// Leave it 'Unknown' otherwise.
				if (read(fdset[0], string, sizeof(string) - 1) > 0) {
					devices[i].vendor = string;
					devices[i].renderer = string + strlen(string) + 1;
				}
			}

			close(fdset[0]);
		} else {
			// In child, exit() here will not quit the engine.

			// Prevent false leak reports as we will not be properly
			// cleaning up these processes, and fork() makes a copy
			// of all globals.
			CoreGlobals::leak_reporting_enabled = false;

			char string[201];

			close(fdset[0]);

			setenv("DRI_PRIME", itos(i).utf8().get_data(), 1);

			Error err = _create_context();

			if (err != OK) {
				print_verbose("Couldn't create a context, skipping GPU detection.");
				quick_exit(0);
			}

			PFNGLGETSTRINGPROC glGetString = (PFNGLGETSTRINGPROC)_get_gl_proc("glGetString");
			if (glGetString == nullptr) {
				print_verbose("Couldn't get glGetString method, skipping GPU detection.");
				quick_exit(0);
			}

			const char *vendor = (const char *)glGetString(GL_VENDOR);
			const char *renderer = (const char *)glGetString(GL_RENDERER);

			unsigned int vendor_len = strlen(vendor) + 1;
			unsigned int renderer_len = strlen(renderer) + 1;

			if (vendor_len + renderer_len >= sizeof(string)) {
				renderer_len = 200 - vendor_len;
			}

			memcpy(&string, vendor, vendor_len);
			memcpy(&string[vendor_len], renderer, renderer_len);

			if (write(fdset[1], string, vendor_len + renderer_len) == -1) {
				print_verbose("Couldn't write vendor/renderer string.");
			}

			close(fdset[1]);

			// The function quick_exit() is used because exit() will call destructors on
			// static objects copied by fork(). These objects will be freed anyway when
			// the process finishes execution.
			quick_exit(0);
		}
	}

	int preferred = 0;
	int priority = 0;

	// DEBUG
	if (devices[0].vendor == devices[1].vendor) {
		print_verbose("Only one GPU found, using default.");
		return 0;
	}

	for (int i = 3; i >= 0; --i) {
		for (const Vendor &v : vendor_map) {
			if (v.name == devices[i].vendor) {
				devices[i].priority = v.priority;

				if (v.priority >= priority) {
					priority = v.priority;
					preferred = i;
				}
			}
		}
	}

	print_verbose("Renderers found:");
	for (int i = 0; i < 4; ++i) {
		print_verbose(vformat("Renderer %d: %s with priority %d", i, devices[i].renderer, devices[i].priority));
	}

	print_verbose(vformat("Using renderer: %s", devices[preferred].renderer));
	return preferred;
}

PrimeLinuxBSD::PrimeLinuxBSD() {
}

PrimeLinuxBSD::~PrimeLinuxBSD() {
}
#endif // GLES3_ENABLED
