#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "stb_image.h"
#include "stb_image_write.h"

extern "C"
{
#include "image.h"
#include "compass.h"
#include "bs.h"
}

using std::cerr;

class CompassOperator
{

public:
	
	CompassOperator(CompassOperator&)		= delete;
	CompassOperator(const CompassOperator&) = delete;

	CompassOperator()
	{
		imgrows = 0;
		imgcols = 0;
		numsigmas = 1;
		numangles = 1;
		nwedges = 6;
		anglewedges = 0;
		plot = 0;

		type = RGBImg;

		maxsigma = sigmas[0];
		q = nullptr;

		strength	= nullptr;
		orientation = nullptr;
	}

	~CompassOperator() {}

	std::vector<it_rgb> read_img(char* fname, int& imgcols, int& imgrows)
	{
		int nrChannels = 0;
		
		unsigned char* img_ptr = stbi_load(fname, &imgcols, &imgrows, &nrChannels, 3);
		
		if (img_ptr)
		{
			std::vector<it_rgb> imgdata;

			if (imgcols > 1024 || imgrows > 1024)
			{
				unsigned char* image_resized = (unsigned char*)malloc(imgcols * imgrows * 3 * sizeof(unsigned char));

				if (image_resized == NULL)
				{
					cout << "failed to allocate resampled image for " << fname << endl;
				}

				if (stbir_resize_uint8_generic(
					img_ptr, imgcols, imgrows, 0,
					image_resized, 1024, 1024,
					0, 3, STBIR_ALPHA_CHANNEL_NONE, 0,
					STBIR_EDGE_WRAP, STBIR_FILTER_TRIANGLE, STBIR_COLORSPACE_SRGB, NULL) == 0)
				{
					cout << "failed to resample image " << fname << endl;
				}
				else
				{
					imgcols = 1024;
					imgrows = 1024;

					imgdata.resize(imgcols * imgrows);

					for (size_t i = 0; i < imgrows; i++)
						for (size_t j = 0; j < imgcols; j++)
							imgdata[i + j * imgcols] = it_rgb{
								image_resized[3 * (j + i * imgrows) + 0],
								image_resized[3 * (j + i * imgrows) + 1],
								image_resized[3 * (j + i * imgrows) + 2] };
				}

				stbi_image_free(image_resized);
			}
			else
			{
				imgdata.resize(imgcols * imgrows);
				
				for (size_t i = 0; i < imgrows; i++)
					for (size_t j = 0; j < imgcols; j++)
						imgdata[i + j * imgcols] = it_rgb{
							img_ptr[3 * (j + i * imgrows) + 0],
							img_ptr[3 * (j + i * imgrows) + 1],
							img_ptr[3 * (j + i * imgrows) + 2] };
			}

			stbi_image_free(img_ptr);

			return imgdata;
		}
		else
		{
			cerr << "unable to open file: " << fname << endl;
			exit(EXIT_FAILURE);
		}
	}

	void write_img(char* fname_out, const std::vector<unsigned char>& img_data, int imgcols, int imgrows)
	{
		int saved = stbi_write_png(fname_out, imgcols, imgrows, 1, img_data.data(), 0);

		if (!saved)
			cerr << "Failed saving image: " << fname_out << endl;
	}

	void mainCompass(char* fname_in, char* fname_out)
	{
		std::vector<it_rgb> imgdata;
		int dims[3];
		double low = 0.5, high = 0.7;
		int maxradius = ceil(3 * maxsigma); // == 3
		double maxclusters[1] = { 30.0 };
		wedgeangle = 90.0 / nwedges;

		for (size_t i = 0; i < numangles; i++)
			if (angles[i] / wedgeangle != floor(angles[i] / wedgeangle))
				printf("Angles chosen not compatible with number of wedges");

		imgdata = read_img(fname_in, imgcols, imgrows);

		if (maxradius * 2 > imgrows || maxradius * 2 > imgcols)
			Error("Image is too small for sigma chosen");

		dimensions[0] = maxradius; // == 3
		dimensions[1] = maxradius; // == 3
		dimensions[2] = imgrows - maxradius; // == height - 3
		dimensions[3] = imgcols - maxradius; // == width - 3

		/* Allocate output arguments */
		orientation = (Matrix*)malloc(sizeof(Matrix));
		orientation->rows = dimensions[2] - dimensions[0] + 1;
		orientation->cols = dimensions[3] - dimensions[1] + 1;
		orientation->sheets = MAXRESPONSES;
		orientation->ptr = (double*)calloc(orientation->rows * orientation->cols * MAXRESPONSES, sizeof(double));
		strength = (Matrix*)malloc(sizeof(Matrix));
		strength->rows = orientation->rows;
		strength->sheets = 1;
		strength->cols = orientation->cols;
		strength->ptr = (double*)calloc(strength->rows * strength->cols, sizeof(double));

		/* Execute the compass operator */
		Compass((void*)imgdata.data(), imgrows, imgcols, type, sigmas, numsigmas,
			maxradius, spacing, dimensions, angles, numangles, nwedges,
			maxclusters, plot, strength, NULL, orientation, NULL);

		printf("done Compass()\n");

		q = strength->ptr;

		std::vector<unsigned char> out_img = std::vector<unsigned char>(imgcols * imgrows);
		for (size_t i = 0; i < strength->cols; i++)
			for (size_t j = 0; j < strength->rows; j++)
				out_img[i + maxradius + (j + maxradius) * imgrows] = (it_byte)(255 * (*q++));
				
		write_img(fname_out, out_img, imgcols, imgrows);

		free(strength->ptr);
		free(strength);
		free(orientation->ptr);
		free(orientation);
	}

private:

	void Error(char* text)
	{
		fprintf(stderr, text);
		exit(1);
	}

	int imgrows, imgcols, numsigmas, numangles, nwedges;
	int anglewedges, plot;

	enum imgtype type;

	double sigmas[1] = { 1.0 }, spacing[1] = { 1.0 }, dimensions[4] = { 0, 0, 0, 0 };
	double angles[1] = { 180 }, wedgeangle = { 0 };

	double maxsigma, * q;

	Matrix* strength, * orientation;

	const double PI = 3.141592653589793238463;
};

