#pragma once

#pragma warning( disable : 6993 ) // Code analysis ignores OpenMP constructs; analyzing single - threaded code

#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif

#include <glad/glad.h> // must be the first include

#include <stdio.h>
#include <vector>
#include <iostream>
#include <limits>
#include <chrono>

#include "MathUtils.h"
#include "Rendering/Shader.h"

#include "glm/glm.hpp"

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::vector;
using glm::mat3;
using glm::vec3;
using glm::vec2;

void printWorkGroupsCapabilities();

void normalizeTable(
    vector<float>& inputVec,
    float& hf,
    float depth,
    float max_radius,
    bool print_histo = false);

void processDiffTable(
    const string& mat_path,
    const vector<float>& diffTable,
    vector<float>& res,
    unsigned int width,
    unsigned int height,
    unsigned int depth);

void computeHeightFactor(
    const Shader& shader,
    int mat_id,
    const string& mat_name,
    float& computed_heightF);