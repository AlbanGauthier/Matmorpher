#pragma once

#include <vector>
#include <string>
#include <chrono>

#include "LinearSystem.h"
#include "KDTree.h"

#include "WarpIO.h"
#include "WarpUtils.h"

using std::vector;
using std::cout;
using std::endl;

void build_and_solve_linear_system(
	std::vector<vec2> const& PiInit,
	std::vector<vec2> const& Qj,
	Eigen::VectorXd& X,
	unsigned int N, float alpha, float beta,
	unsigned int NIterations = 10);

void compute_and_serialize_warpgrid(
	std::vector<vec2> const& P_xy,
	std::vector<vec2> const& Q_xy,
	Params const& cmd_inputs);