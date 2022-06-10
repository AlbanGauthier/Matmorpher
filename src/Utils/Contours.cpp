#include "Contours.h"

//#include "cySampleElim.h"
//#include "cyPoint.h"

#include <QImage>
#include <QPainter>

void* xmalloc(size_t size)
{
	void* p;
	if (size == 0) cout << "xmalloc input: zero size" << endl;
	p = malloc(size);
	if (p == NULL) cout << "out of memory" << endl;
	return p;
}

bool read_input_image(
	std::vector<double>& out, 
	std::string filename, 
	size_t W, size_t H, 
	ColorSpace color_space,
	int map_id,
	bool debug_out)
{
	int X, Y, channels;

	uint16_t* image;

	// height, metallic or roughness
	if (color_space == ColorSpace::Linear && map_id != 3)
	{
		image = stbi_load_16(filename.c_str(), &X, &Y, &channels, 1);
		if (channels != 1)
			cout << "warning: invalid number of image components in " << filename << endl;
		channels = 1;
	}
	// color or normal
	else
	{
		image = stbi_load_16(filename.c_str(), &X, &Y, &channels, 3);
		if (channels != 3)
			cout << "warning: invalid number of image components in " << filename << endl;
		channels = 3;
	}

	if (image != NULL)
	{
		uint16_t* image_resized = (uint16_t*) xmalloc(H * W * channels * sizeof(uint16_t));

		if (image_resized == NULL) return false;

		if (X != W || Y != H)
		{
			if (stbir_resize_uint16_generic(
				image,		   X, Y, 0,
				image_resized, W, H, 0, 
				channels, STBIR_ALPHA_CHANNEL_NONE, 0,
				STBIR_EDGE_WRAP, STBIR_FILTER_TRIANGLE, 
				static_cast<stbir_colorspace>(color_space), NULL) == 0)
			{
				cout << "failed to resample image " + filename << endl;
				return false;
			}
		}
		else
		{
			memcpy(image_resized, image, H * W * channels * sizeof(uint16_t));
		}

		stbi_image_free(image);

		out.clear();
		out.resize(H * W);

		for (size_t pos_2d = 0; pos_2d < H * W; pos_2d++)
		{
			const uint16_t* pixelOffset = image_resized + pos_2d * static_cast<size_t>(channels);

			double gray_value = 0;

			// COLOR MAP
			if (color_space == ColorSpace::sRGB)
			{
				// gamma corrected read
				dvec3 rgb =
				{
					255.0 * sRGB2Linear(pixelOffset[0] / 65535.0),
					255.0 * sRGB2Linear(pixelOffset[1] / 65535.0),
					255.0 * sRGB2Linear(pixelOffset[2] / 65535.0)
				};

				gray_value = RGB2YCbCr(rgb).r;
			}
			// NORMAL MAP
			else if (channels == 3)
			{
				gray_value = 255.0 * pixelOffset[2] / 65535.0;
			}

			else // channel == 1 --> greyscale
			{
				gray_value = 255.0 * pixelOffset[0] / 65535.0;
			}

			out[pos_2d] = gray_value;
		}

		if (map_id == 1) // heightmap
		{
			double min = 10e9;
			double max = 0.0;
			for (int i = 0; i < out.size(); i++)
			{
				if (out[i] < min) min = out[i];
				else if (out[i] > max) max = out[i];
			}
			for (int i = 0; i < out.size(); i++)
			{
				out[i] = 255.0 * (out[i] - min) / (max - min);
			}
		}

		if (debug_out)
		{
			if (stbi_write_png("clover/debug.png", W, H, channels, image_resized, 0) == 0)
			{
				cout << "failed to write image resized image" << endl;
			}
		}

		free((void*) image_resized);

		return true;
	}

	else
	{
		std::cout << "could not load image" << filename << std::endl;
		return false;
	}
}

void write_render_normal(std::string filename, std::string file_out)
{
	int X, Y, channels;

	uint16_t* image = stbi_load_16(filename.c_str(), &X, &Y, &channels, 0);

	if (channels != 3)
		cout << "warning: normal map channels != 3" << endl;

	std::vector<unsigned char> gray_normal(X * Y);

	for (size_t pos_2d = 0; pos_2d < X * Y; pos_2d++)
	{
		const uint16_t* pixelOffset = image + pos_2d * static_cast<size_t>(channels);

		gray_normal[pos_2d] = 255.0f * pixelOffset[2] / 65535.0f;
	}

	if (stbi_write_png(file_out.c_str(), X, Y, 1, gray_normal.data(), 0) == 0)
	{
		cout << "failed to write image: " << filename << endl;
	}
}

bool getContoursListFromMaps(std::string mat_name, int material_to_use, std::vector<vec2>& contourPts)
{
	int N, M;			/* result: N contour points, forming M curves */
	double Q = 2.0;		/* default Q=2, here we assume a smaller pixel quantization than compressed natural images */
	bool result = true;

	int M_total = 0;
	int N_total = 0;

	std::vector<double> contour_x_total;
	std::vector<double> contour_y_total;
	std::vector<int> curve_limits_total;

	std::vector<std::string> PBR_maps = {
		"color",
		"height",
		"metallic",
		"normal",
		"roughness" };

	std::vector<ColorSpace> PBR_colorspaces = {
		ColorSpace::sRGB,
		ColorSpace::Linear,
		ColorSpace::Linear,
		ColorSpace::Linear,
		ColorSpace::Linear};

	size_t X = 1024, Y = 1024;
	std::vector<double> sc_input;

	contourPts.clear();

	for (int map_id = 0; map_id < 5; map_id++)
	{
		if ( !(material_to_use & (1 << (4 - map_id))) ) continue;

		std::string mat_filename = mat_name + "/" + PBR_maps[map_id] + ".png";

		cout << "using " << mat_filename << " to compute warpgrid" << endl;

		stbi_set_flip_vertically_on_load(true);

		if (read_input_image(sc_input, mat_filename, X, Y, PBR_colorspaces[map_id], map_id))
		{
			double* contour_x;  /* x[n] coordinates of result contour point n */
			double* contour_y;  /* y[n] coordinates of result contour point n */
			int* curve_limits;  /* limits of the curves in the x[] and y[] */

			smooth_contours(&contour_x, &contour_y, &N, &curve_limits, &M, sc_input.data(), X, Y, Q);
			
			/* write curves */
			for (int k = 0; k < M; k++) /* write curves */
			{
				for (int i = curve_limits[k]; i < curve_limits[k + 1]; i++)
				{
					contour_x_total.push_back(contour_x[i]);
					contour_y_total.push_back(Y - contour_y[i]);

					//inputPoints.push_back(cy::Point2f(contour_x[i] / float(X), contour_y[i] / float(Y)));
					contourPts.push_back(vec2(contour_x[i] / float(X), contour_y[i] / float(Y)));
				}

				curve_limits_total.push_back(curve_limits[k] + N_total);
			}

			M_total += M;
			N_total += N;

			/* free memory */
			free((void*)curve_limits);
			free((void*)contour_x);
			free((void*)contour_y);
		}
		else
		{
			cout << "could not read " + mat_filename << endl;
			result = false;
		}
	}

	stbi_set_flip_vertically_on_load(false);
	
	curve_limits_total.push_back(M_total);

	cout << "detected " << M << " contours with a total of " << N << " points" << endl;

	std::string contours_out = mat_name + "/warp_contours.png";

	saveContourImage(contour_x_total.data(), contour_y_total.data(), curve_limits_total.data(), M_total, &contours_out[0], X, Y, 2.0);

	return result;
}

void saveContourImage(
	double* x, double* y, int* curve_limits, int M,
	char* filename, int X, int Y, double width)
{
	auto format = QImage::Format_RGB888;

	float multiplier = 1.0f;

	QImage image(multiplier * X, multiplier * Y, format);
	QPainter painter(&image);

	painter.setRenderHint(QPainter::Antialiasing, true);

	painter.fillRect(0, 0, multiplier * X, multiplier * Y, QColor(255, 255, 255));

	QPen pen;
	pen.setWidth(width);
	pen.setColor(Qt::black);
	painter.setPen(pen);

	for (int k = 0; k < M; k++) /* write curves */
	{
		/* initate chain */
		int i = curve_limits[k];

		vec2 current_point = multiplier * vec2(x[i], Y - y[i]); /* first point */
		vec2 last_point = vec2(0);
		vec2 next_point = vec2(0);
		vec2 tangent = vec2(0);

		/* add remaining points of the curve */
		for (int j = i + 1; j < curve_limits[k + 1]; j++)
		{	
			next_point = multiplier * vec2(x[j], Y - y[j]);
			pen.setColor(Qt::black);
			painter.setPen(pen);
			painter.drawLine(current_point.x, current_point.y, next_point.x, next_point.y);

			if (j - 1 == i)
			{
				tangent = next_point - current_point;
			}
			else if (j == curve_limits[k + 1] - 1)
			{
				last_point = multiplier * vec2(x[j - 2], Y - y[j - 2]);
				tangent = current_point - last_point;
			}
			else
			{
				last_point = multiplier * vec2(x[j - 2], Y - y[j - 2]);
				tangent = (next_point - last_point) / 2.0f;
			}

			vec2 normal = multiplier * 10.0f * glm::normalize(vec2(-tangent.y, tangent.x));
			vec2 target = current_point + normal;
			
			current_point = next_point;
		}
	}

	painter.end();

	image.mirrored(false, true).save(QString::fromStdString(filename));
}

int mainContour(int argc, char* argv[])
{
	int N, M;			/* result: N contour points, forming M curves */
	double Q = 2.0;		/* default Q=2 */
	double W = 1.3;		/* PDF line width 1.3 */

	std::string mat_name = argv[2];

	std::vector<std::string> PBR_maps = {
		"color", 
		"height", 
		"metallic", 
		"normal", 
		"roughness"};

	std::vector<ColorSpace> PBR_colorspaces = {
		ColorSpace::sRGB,
		ColorSpace::Linear,
		ColorSpace::Linear,
		ColorSpace::Linear,
		ColorSpace::Linear};

	size_t X = 1024, Y = 1024;
	std::vector<double> sc_input;

	write_render_normal(mat_name + "/normal.png", mat_name + "/normal_render.png");

	for (int map_id = 0; map_id < 5; map_id++)
	{
		std::string mat_filename	= mat_name + "/" + PBR_maps[map_id] + ".png";
		std::string contour_out		= mat_name + "/acontrs_" + PBR_maps[map_id] + ".png";

		if (read_input_image(sc_input, mat_filename, X, Y, PBR_colorspaces[map_id], map_id))
		{
			double* contour_x;  /* x[n] coordinates of result contour point n */
			double* contour_y;  /* y[n] coordinates of result contour point n */
			int* curve_limits;  /* limits of the curves in the x[] and y[] */

			smooth_contours(&contour_x, &contour_y, &N, &curve_limits, &M, sc_input.data(), X, Y, Q);

			saveContourImage(contour_x, contour_y, curve_limits, M, &contour_out[0], X, Y, 1.0);

			cout << "writing " + contour_out << endl;

			/* free memory */
			free((void*)curve_limits);
			free((void*)contour_x);
			free((void*)contour_y);
		}
		else
		{
			cout << "could not read " + mat_filename << endl;
		}
	}
	
	return EXIT_SUCCESS;
}