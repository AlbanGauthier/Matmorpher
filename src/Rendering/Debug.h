/**
 * This file is part of Augen Light
 *
 * Copyright (c) 2017 - 2018 -- �lie Michel <elie.michel@exppad.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the �Software�), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided �as is�, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#ifndef H_DEBUGUTILS
#define H_DEBUGUTILS

#ifdef _WIN32
#include <windows.h>
#elif !defined(APIENTRY)
#define APIENTRY
#endif

#include <glad/glad.h>

/**
 * Setup the opengl debug callback
 */
void enableGlDebug();

/**
 * Callback to use with glDebugMessageCallback
 * credits: https://blog.nobel-joergensen.com/2013/02/17/debugging-opengl-part-2-using-gldebugmessagecallback/
 */
void APIENTRY openglCallbackFunction(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam);

#ifdef GLAD_DEBUG
/**
 * logs every gl call to the console
 */
void openglPreFunction(const char *name, void *funcptr, int len_args, ...);
#endif

#endif // H_DEBUGUTILS
