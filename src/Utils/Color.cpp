#include "Color.h"

// https://www.nayuki.io/page/srgb-transform-library
double sRGB2Linear(double sRGBColor)
{
	if (sRGBColor <= 0.04045)
		return sRGBColor / 12.92;
	else
		return pow((sRGBColor + 0.055) / 1.055, 2.4);
}

double Linear2sRGB(double linearColor)
{
	if (linearColor <= 0.0031308)
		return linearColor * 12.92;
	else
		return 1.055 * pow(linearColor, 1.0 / 2.4) - 0.055;
}

// input  : [0, 1]
// output : [0, 1]
dvec3 RGB2YCbCr(dvec3 in)
{
	return dvec3(
		.299 * in.r + .587 * in.g + .114 * in.b,
		-.168736 * in.r - .331264 * in.g + .5 * in.b	+ 0.5,
		.5 * in.r - .418688 * in.g - .081312 * in.b		+ 0.5);
}

// input  : [0, 1]
// output : [0, 1]
dvec3 YCbCr2RGB(dvec3 in)
{
	in -= dvec3(0., 0.5, 0.5);
	return dvec3(
		in.x + 1.402 * in.z,
		in.x - 0.344136 * in.y - .714136 * in.z,
		in.x + 1.772 * in.y
	);
}

