#include "Histogram.h"
#include "MathUtils.h"

vector<double> resample_cs(const vector<double>& cs, size_t hist_size)
{
	std::vector<std::pair<double, double>> points(cs.size());
	for (unsigned int i = 0; i < cs.size(); i++)
	{
		points[i] = std::pair<double, double>(double(i) / double(cs.size()), cs[i]);
	}
	Interpolator interp_cs{points};
	vector<double> upsampled_cs(hist_size, 0);
	for (unsigned int i = 0; i < hist_size; i++)
	{
		upsampled_cs[i] = interp_cs.findValue(double(i) / double(hist_size));
	}
	return upsampled_cs;
}

// input: [0, 1]
// output : [0, 1]
vector<double> cumsum(const vector<double>& img)
{
	vector<int> hist(256, 0);
	vector<double> cdf(256, 0);
	for (unsigned int i = 0; i < img.size(); i++)
	{
		hist[static_cast<int>(255.0f * img[i])] += 1;
	}
	std::partial_sum(hist.begin(), hist.end(), cdf.begin());
	double sum = cdf.back(); // = img.size()
	assert(sum == img.size());
	for (unsigned int i = 0; i < 256; i++)
	{
		cdf[i] /= sum;
	}
	return cdf;
}

// input : [0,1]
// output : [0,1]
vector<float> computeInverseCDF(vector<double>& cdf)
{
	size_t n = cdf.size();
	vector<float> inv_cdf(cdf.size(), 0);
	int last_c = -1;
	double x0 = 0;
	for (unsigned int i = 0; i < n; ++i)
	{
		double x1 = cdf[i] * (n - 1);
		double x0_to_x1 = (std::floor(x1) - x0) * (n - 1);
		double cumulated_before_x0 = double(i) / (n - 1.0f);
		for (unsigned int c = last_c + 1; c < std::floor(x1); ++c)
		{
			inv_cdf[c] = cumulated_before_x0 + (c - x0 + 1.0f) / x0_to_x1;
			last_c = c;
		}
		x0 = std::floor(x1);
		for (unsigned int c = last_c + 1; c < n; c++)
			inv_cdf[c] = 1.0f;
	}
	return inv_cdf;
}

void simpleChannelInterpolation(
	vector<double>& result,
	const vector<double>& img_1,
	const vector<double>& img_2,
	const float t)
{
#pragma omp parallel for
	for (int i = 0; i < img_1.size(); i++)
	{
		result[i] = (1.0f - t) * img_1[i] + t * img_2[i];
	}
}

void getGaussianizedChannel(
	vector<double>& gaussianized_img,
	const vector<double>& data,
	int hist_size)
{
	vector<double> cs_8bit = cumsum(data);
	vector<double> LUT = resample_cs(cs_8bit);

	// normalize and Gaussianize LUT
	for (size_t i = 0; i < hist_size; i++)
	{
		LUT[i] = invCDFTruncated(LUT[i], 0.5f, sigma_gauss);
	}

//#pragma omp parallel for

	// apply LUT to texture
	for (int i = 0; i < data.size(); i++)
	{
		gaussianized_img[i] = LUT[static_cast<int>(double(hist_size - 1) * data[i])];
	}
}

void getGaussianizedAlbedoAndCDF(
	unsigned char* const img_ptr,
	vector<float>& gaussianized,
	vector<float>& inv_cdf_LUT,
	int width,
	int height,
	int nrChannels,
	const bool ycbcr_interpolation)
{
	unsigned int img_2d_size = width * height;
	vector<double> img_channel(img_2d_size);

	vector<vector<double>> img_per_channel = { img_channel, img_channel, img_channel };
	vector<vector<double>> per_channel_result(img_per_channel);

	//
	// read input image
	//
	for (size_t position_2d = 0; position_2d < img_2d_size; position_2d++)
	{
		// read image values and separate into one vectors per channel
		unsigned char* pixelOffset = img_ptr + position_2d * static_cast<size_t>(nrChannels);

		// gamma corrected read
		dvec3 rgb = {
			std::pow(pixelOffset[0] / 255.0, 2.2),
			std::pow(pixelOffset[1] / 255.0, 2.2),
			std::pow(pixelOffset[2] / 255.0, 2.2)
		};

		if (ycbcr_interpolation)
		{
			rgb = RGB2YCbCr(rgb);
		}

		for (int k = 0; k < 3; k++)
		{
			img_per_channel[k][position_2d] = clamp(rgb[k], 0.0, 1.0);
		}
	}

	// only gaussianize Y = 0 channel
	if (ycbcr_interpolation)
	{
		getGaussianizedChannel(per_channel_result[0], img_per_channel[0]);
		per_channel_result[1] = img_per_channel[1];
		per_channel_result[2] = img_per_channel[2];
	}
	else
	{
		for (int k = 0; k < 3; k++)
		{
			getGaussianizedChannel(per_channel_result[k], img_per_channel[k]);
		}
	}

	for (int k = 0; k < 3; k++)
	{
		vector<double> cs_8bit = cumsum(img_per_channel[k]);
		vector<double> resampled_cs = resample_cs(cs_8bit);
		vector<float> inv_cdf = computeInverseCDF(resampled_cs);
		inv_cdf_LUT.insert(inv_cdf_LUT.end(), inv_cdf.begin(), inv_cdf.end());
	}

	//
	// construction of the result image
	//
	for (unsigned int j = 0; j < height; j++)
	{
		for (unsigned int i = 0; i < width; i++)
		{
			size_t position_2d = i + static_cast<size_t>(width) * j;

			for (int k = 0; k < 3; k++)
			{
				// do not gamma correct, since the texture is read linearly by the OpenGL call
				gaussianized[position_2d * nrChannels + k] = clamp(per_channel_result[k][position_2d], 0., 1.);
			}
		}
	}
}

void gaussianizedAlbedoGPU(
	vector<float>& inv_cdf_LUT,
	vector<float>& gaussianized,
	const Shader& shader,
	unsigned char* const img_ptr,
	int imgWidth,
	int imgHeight,
	int nrChannels,
	const bool ycbcr_interpolation)
{
	// Parameters of the color histogram
	const GLsizei histogramWidth = 256;
	const GLsizei histogramHeight = 3;
	const GLsizei cdfWidth = histogram_size_resampled;

	// Init image
	GLuint imgID;
	glCreateTextures(GL_TEXTURE_2D, 1, &imgID);
	glTextureStorage2D(imgID, 1, GL_RGBA8, imgWidth, imgHeight);
	glTextureSubImage2D(imgID, 0, 0, 0, imgWidth, imgHeight, GL_RGBA, GL_UNSIGNED_BYTE, img_ptr);

	// Init histogram
	GLuint histID;
	glCreateTextures(GL_TEXTURE_2D, 1, &histID);
	glTextureStorage2D(histID, 1, GL_R32I, histogramWidth, histogramHeight);

	// Init CDF resampled
	GLuint cdfID;
	glCreateTextures(GL_TEXTURE_2D, 1, &cdfID);
	glTextureStorage2D(cdfID, 1, GL_R32F, cdfWidth, histogramHeight);

	// Init gaussianized tex
	GLuint gaussTexID;
	glCreateTextures(GL_TEXTURE_2D, 1, &gaussTexID);
	glTextureStorage2D(gaussTexID, 1, GL_RGBA32F, imgWidth, imgHeight);

	////////////////////
	// COMPUTE SHADER //
	////////////////////

	shader.use();

	shader.setInt("YCbCr", ycbcr_interpolation);

	glBindImageTexture(0, imgID,		0, GL_TRUE, 0, GL_READ_ONLY,  GL_RGBA8);
	glBindImageTexture(1, histID,		0, GL_TRUE, 0, GL_READ_WRITE, GL_R32I);
	glBindImageTexture(2, cdfID,		0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
	glBindImageTexture(3, gaussTexID,	0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// init hist and image with zeros
	shader.setInt("uStep", 0);
	glDispatchCompute(imgWidth, imgHeight, 1);

	// compute histogram
	shader.setInt("uStep", 1);
	glDispatchCompute(imgWidth, imgHeight, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//////////////////////
	// CPU SIDE COMPUTE //
	//////////////////////

	// recover histogram on CPU
	size_t hist_size = histogramWidth * histogramHeight;
	std::vector<GLuint> histData(hist_size);
	glGetTextureImage(histID, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, hist_size * sizeof(GLuint), histData.data());

	// compute cdf
	std::vector<double> cdfData(hist_size, 0);
	std::partial_sum(histData.begin(),				histData.begin() + 256,		cdfData.begin());
	std::partial_sum(histData.begin() + 256,		histData.begin() + 2 * 256, cdfData.begin() + 256);
	std::partial_sum(histData.begin() + 2 * 256,	histData.end(),				cdfData.begin() + 2 * 256);

	std::vector<std::vector<double>> cdfDataPerChannel(3, std::vector<double>(histogramWidth, 0));

	// normalize cdf
	double sum = size_t(imgWidth) * size_t(imgHeight);
	for (unsigned int i = 0; i < histogramWidth; i++)
	{
		for (unsigned int k = 0; k < 3; k++)
		{
			cdfData[i + k * size_t(histogramWidth)] /= sum;
			cdfDataPerChannel[k][i] = cdfData[i + k * size_t(histogramWidth)];
		}
	}

	// compute gaussianized inv cdf
	std::vector<double> cdfDataResampled(cdfWidth * histogramHeight, 0);
	for (int k = 0; k < 3; k++)
	{
		vector<double> resampled_cs = resample_cs(cdfDataPerChannel[k], histogram_size_resampled);

		for (size_t i = 0; i < cdfWidth; i++)
			cdfDataResampled[i + k * cdfWidth] = invCDFTruncated(resampled_cs[i], 0.5f, sigma_gauss);

		vector<float> inv_cdf = computeInverseCDF(resampled_cs);
		inv_cdf_LUT.insert(inv_cdf_LUT.end(), inv_cdf.begin(), inv_cdf.end());
	}

	std::vector<GLfloat> cdfGPU(cdfDataResampled.size());
	for (int i = 0; i < cdfDataResampled.size(); i++)
		cdfGPU[i] = cdfDataResampled[i];

	glTextureSubImage2D(cdfID, 0, 0, 0, cdfWidth, histogramHeight, GL_RED, GL_FLOAT, cdfGPU.data());
	shader.setInt("cdf_size", cdfWidth);

	////////////////////
	// COMPUTE SHADER //
	////////////////////

	// gaussianize
	shader.setInt("uStep", 2);
	glDispatchCompute(imgWidth, imgHeight, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glGetTextureImage(gaussTexID, 0, GL_RGBA, GL_FLOAT, imgHeight * imgWidth * 4 * sizeof(GLfloat), gaussianized.data());
}

void getInterpolatedAlbedoLUT(
	const vector<vector<double>>& img1,
	const vector<vector<double>>& img2,
	vector<double>& interp_cdf_LUT,
	int width,
	int height,
	int nrChannels,
	const bool ycbcr_interpolation,
	const double t)
{
	unsigned int img_2d_size = width * height;
	vector<double> img_channel(img_2d_size, 0);
	vector<vector<double>> interp_img = { img_channel, img_channel, img_channel };

	// only interpolate Y channel
	if (ycbcr_interpolation)
	{
		vector<double> img_y_1(img_2d_size);
		vector<double> img_y_2(img_2d_size);

		for (int i = 0; i < img_2d_size; i++)
		{
			dvec3 rgb = {img1[0][i], img1[1][i], img1[2][i]};
			dvec3 ycbcr = RGB2YCbCr(rgb);
			img_y_1[i] = ycbcr.x;
			
			rgb = { img2[0][i], img2[1][i], img2[2][i] };
			ycbcr = RGB2YCbCr(rgb);
			img_y_2[i] = ycbcr.x;
		}

		simpleChannelInterpolation(interp_img[0], img_y_1, img_y_2, t);
		vector<double> cs_8bit = cumsum(interp_img[0]);
		vector<double> resampled_cs = resample_cs(cs_8bit);

		interp_cdf_LUT.insert(interp_cdf_LUT.end(), resampled_cs.begin(), resampled_cs.end());

		// fill the rest with zeros
		vector<double> zeros(histogram_size_resampled, 0);
		interp_cdf_LUT.insert(interp_cdf_LUT.end(), zeros.begin(), zeros.end());
		interp_cdf_LUT.insert(interp_cdf_LUT.end(), zeros.begin(), zeros.end());
	}
	else
	{
		for (int k = 0; k < 3; k++)
		{
			simpleChannelInterpolation(interp_img[k], img1[k], img2[k], t);

			// compute cdf
			vector<double> cs_8bit = cumsum(interp_img[k]);
			vector<double> resampled_cs = resample_cs(cs_8bit);
			interp_cdf_LUT.insert(interp_cdf_LUT.end(), resampled_cs.begin(), resampled_cs.end());
		}
	}
}

void loadImageAsLinearChannelSeparatedVector(
	unsigned char* const img_ptr,
	vector<vector<double>>& img_out,
	int width,
	int height,
	int nrChannels)
{
	unsigned int img_2d_size = width * height;

	for (size_t position_2d = 0; position_2d < img_2d_size; position_2d++)
	{
		unsigned char* pixelOffset = img_ptr + position_2d * static_cast<size_t>(nrChannels);

		//// gamma corrected read
		//dvec3 rgb = {
		//	std::pow(pixelOffset[0] / 255.0, 2.2),
		//	std::pow(pixelOffset[1] / 255.0, 2.2),
		//	std::pow(pixelOffset[2] / 255.0, 2.2)
		//};

		dvec3 rgb = {
			pixelOffset[0] / 255.0,
			pixelOffset[1] / 255.0,
			pixelOffset[2] / 255.0
		};

		for (int k = 0; k < nrChannels; k++)
		{
			img_out[k][position_2d] = clamp(rgb[k], 0., 1.);
		}
	}
}