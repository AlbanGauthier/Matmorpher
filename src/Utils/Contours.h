#pragma once

#include <iostream>
#include <vector>

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

#include "glm/glm.hpp"

#include "Utils/Color.h"

extern "C" 
{
#include "smooth_contours.h"
#include "io.h"
}

using std::cout;
using std::endl;
using glm::dvec3;
using glm::vec2;

enum class ColorSpace
{
	Linear	= STBIR_COLORSPACE_LINEAR,
	sRGB	= STBIR_COLORSPACE_SRGB
};

void* xmalloc(size_t size);

bool read_input_image(
	std::vector<double>& out,
	std::string filename,
	size_t W, size_t H,
	ColorSpace color_space,
	int map_id,
	bool debug_out = false);

void write_render_normal(std::string filename, std::string file_out);

bool getContoursListFromMaps(std::string mat_name, int material_to_use, std::vector<vec2>& contourPts);

void saveContourImage(
	double* x, double* y, int* curve_limits, int M,
	char* filename, int X, int Y, double width);

int mainContour(int argc, char* argv[]);