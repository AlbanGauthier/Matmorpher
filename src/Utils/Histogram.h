#pragma once

#include "MathUtils.h"
#include "Color.h"
#include "Rendering/Shader.h"

#include <iostream>
#include <numeric>
#include <chrono>

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using glm::mat3;
using glm::dvec3;

constexpr double sigma_gauss = 1.0 / 6.0;
constexpr size_t histogram_size_resampled = 4096;

vector<double> resample_cs(const vector<double>& cs, size_t hist_size = histogram_size_resampled);

// input: [0, 1]
// output : [0, 1]
vector<double> cumsum(const vector<double>& img);

// input : [0,1]
// output : [0,1]
vector<double> computeInverseCDF(vector<float>& cdf);

void simpleChannelInterpolation(
	vector<double>& result,
	const vector<double>& img_1,
	const vector<double>& img_2,
	const float t);

void getGaussianizedChannel(
	vector<double>& gaussianized_img,
	const vector<double>& data,
	int hist_size = histogram_size_resampled);

void getGaussianizedAlbedoAndCDF(
	unsigned char* const img_ptr,
	vector<float>& gaussianized,
	vector<float>& inv_cdf_LUT,
	int width,
	int height,
	int nrChannels,
	const bool ycbcr_interpolation);

void getInterpolatedAlbedoLUT(
	const vector<vector<double>>& img1,
	const vector<vector<double>>& img2,
	vector<double>& interp_cdf_LUT,
	int width,
	int height,
	int nrChannels,
	const bool ycbcr_interpolation,
	const double t);

void loadImageAsLinearChannelSeparatedVector(
	unsigned char* const img_ptr,
	vector<vector<double>>& img_out,
	int width,
	int height,
	int nrChannels);

void gaussianizedAlbedoGPU(
	vector<float>& inv_cdf_LUT,
	vector<float>& gaussianized,
	const Shader& shader,
	unsigned char* const img_ptr,
	int imgWidth,
	int imgHeight,
	int nrChannels,
	const bool ycbcr_interpolation);