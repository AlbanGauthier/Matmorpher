#pragma once

#include "Eigen/SVD"
#include "Eigen/Geometry"

#include "stb_image.h"

#include "Mat2.h"

#include <vector>
#include <iostream>

using glm::vec2;
using std::vector;
using std::cout;
using std::endl;
using std::cerr;

template<typename Img_type>
class TextureSampler
{
public:

    TextureSampler(char* fname_tex)
    {
        unsigned char* img_ptr = stbi_load(fname_tex, &width, &height, &nbChannels, 0);

        if (nbChannels != 1)
            cout << "warning: the texture given at the TextureSampler is not grayscale!" << endl;

        if (img_ptr)
        {
            img_data = vector<vector<float>>(height, vector<float>(width));
            for (int y = 0; y < height; y++)
                for (int x = 0; x < width; x++)
                    img_data[y][x] = img_ptr[x + y * height] / 255.0f;

            stbi_image_free(img_ptr);
        }
        else
        {
            cerr << "could not load texture for the TextureSampler" << endl;
            exit(EXIT_FAILURE);
        }
    }

    TextureSampler(const unsigned char* const data_ptr, int width_, int height_)
    {
        width = width_;
        height = height_;
        img_data = vector<vector<Img_type>>(height, vector<Img_type>(width));
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                img_data[y][x] = data_ptr[x + y * height] / 255.0f;
    }

    TextureSampler(const vector<vector<Img_type>>& data)
    {
        width = data[0].size();
        height = data.size();
        img_data = vector<vector<Img_type>>(height, vector<Img_type>(width));
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                img_data[y][x] = data[y][x];
    }

    Img_type sampleTexture(float x, float y)
    {
        x = x * width - 0.5f;
        y = y * height - 0.5f;
        int xi = floorf(x);
        int yi = floorf(y);
        float xf = x - xi;
        float yf = y - yi;
        xi = (xi % width + width) % width;
        yi = (yi % height + height) % height;
        Img_type val[4];
        val[0] = img_data[yi][xi];
        if (xi == width - 1)
            val[1] = img_data[yi][0];
        else
            val[1] = img_data[yi][xi + 1];
        if (yi == height - 1)
            val[2] = img_data[0][xi];
        else
            val[2] = img_data[yi + 1][xi];
        if (xi == width - 1 && yi == height - 1)
            val[3] = img_data[0][0];
        else if (xi == width - 1)
            val[3] = img_data[yi + 1][0];
        else if (yi == height - 1)
            val[3] = img_data[0][xi + 1];
        else
            val[3] = img_data[yi + 1][xi + 1];

        return lerp(lerp(val[0], val[1], xf), lerp(val[2], val[3], xf), yf);
    }

private:

    vector<vector<Img_type>> img_data;

    int width = 0;
    int height = 0;
    int nbChannels = 0;
};

struct Params
{
	std::string filename_P;
	std::string filename_Q;
	std::string alpha_str;
	std::string beta_str;
    int grid_size = 0;
    int alpha = 0;
    int beta = 0;
};

struct pointFeature {
    vec2 pos;
    Mat2f cov;
    vec2 cov_sigma;

    float operator [] (unsigned int c) const {
        if (c < 2) return pos[c];
        if (c < 4) return cov_sigma[c - 2];
        return 0.0;
    }

    inline float sqr(float x) const { return x * x; }

    float squareDistance(pointFeature const& o) const {
        return glm::dot(pos - o.pos, pos - o.pos)
            + sqr(sqr(cov_sigma[0]) - sqr(o.cov_sigma[0]))
            + sqr(sqr(cov_sigma[1]) - sqr(o.cov_sigma[1]));
    }

    float distance(pointFeature const& o) const {
        return sqrt(squareDistance(o));
    }
};

double square(double x);

double cubic(double x);

float kernel_phi(float r, float h = 0.01);

double GaussianKernel(double r, double h);

//https://num.math.uni-goettingen.de/schaback/teaching/sc.pdf
double WendlandKernel(double r, double h);

void get_bilinear_interpolation(std::vector<int>& grid_coords, float& lambda, float& mu, vec2 xy, unsigned int N);

vec2 get_grid_cell(int coord, unsigned int N);
