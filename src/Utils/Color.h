#pragma once

#include "glm/glm.hpp"

using glm::dvec3;

double sRGB2Linear(double sRGBColor);

double Linear2sRGB(double linearColor);

// input: [0, 1]
// output : [0, 1]
dvec3 RGB2YCbCr(dvec3 in);

// input: [0, 1]
// output : [0, 1]
dvec3 YCbCr2RGB(dvec3 in);