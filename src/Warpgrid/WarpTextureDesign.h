#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>

#include <QColor>
#include <QImage>
#include <QPainter>

#include <glm/glm.hpp>

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"

#include "Utils/MathUtils.h"
#include "Warpgrid/WarpUtils.h"

using std::vector;
using std::string;
using glm::vec2;
using std::cerr;
using std::cout;
using std::endl;

bool resize_img(const unsigned char* img_in, int oldX, int oldY, unsigned char* output, int newX, int newY);

void write_warpgrid(const char* fname_in, const vector<vector<vec2>>& warp_in);

void write_warpgrid_image(const char* fname_in, const vector<vector<vec2>>& warp_in);

void saveGridImage(
    vector<vector<vec2>> const& G,
    std::string const& filename,
    int N);

void saveGridImage_test(
    std::vector<vec2> const& neigh,
    vector<vector<vec2>> const& G,
    std::string const& filename,
    int N);

bool computeWarpgridTextureDesign(string fname_in, string fname_in_2, float alpha, int output_size);

vector<vector<vec2>> get_padded_warpgrid(const vector<vector<vec2>>& warpgrid);

std::vector<vec2> get_positions(vec2 coords, int scale);

std::vector<vec2> get_positions(const vector<vector<vec2>>& warpgrid, size_t i, size_t j);

float compute_frobenius_norm(const vector<vector<vec2>>& warp_pad_in, float x, float y, int i, int j);

vector<vector<vec2>> upsample_warpgrid(const vector<vector<vec2>>& warp_in);

vector<vector<vec2>> resize_final_warpgrid(const vector<vector<vec2>>& warp_in);

