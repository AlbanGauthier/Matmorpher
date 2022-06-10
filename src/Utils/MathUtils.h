#pragma once

// Interpolator Class
#include <utility>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <math.h>

template<typename T, typename U>
T lerp(T a, T b, U f)
{
	return a + f * (b - a);
}

template<typename T>
T clamp(T val, T min, T max)
{
	return std::min(std::max(val, min), max);
}

//
// from Procedural Stochastic Textures by Tiling and Blending, Deliot et al., sample code
//

float Erf(float x);

float ErfInv(float x);

float CDF(float x, float mu, float sigma);

float invCDF(float U, float mu, float sigma);

float CDFTruncated(float x, float mu, float sigma);

float invCDFTruncated(float U, float mu, float sigma);

float soft_clipping(float x, float W);

//
// https://bulldozer00.blog/2016/05/10/linear-interpolation-in-c/
//
class Interpolator {
public:
	//On construction, we take in a vector of data point pairs
	//that represent the line we will use to interpolate
	Interpolator(const std::vector<std::pair<double, double>>& points) : _points(points)
	{
		//Defensive programming. Assume the caller has not sorted the table in
		//in ascending order
		std::sort(_points.begin(), _points.end());

		//Ensure that no 2 adjacent x values are equal,
		//lest we try to divide by zero when we interpolate.
		const double EPSILON{ 1.0E-8 };
		for (std::size_t i = 1; i < _points.size(); ++i) {
			double deltaX{ std::abs(_points[i].first - _points[i - 1].first) };
			if (deltaX < EPSILON) {
				std::string err{ "Potential Divide By Zero: Points " +
				  std::to_string(i - 1) + " And " +
				  std::to_string(i) + " Are Too Close In Value" };
				throw std::range_error(err);
			}
		}
	}

	//Computes the corresponding Y value 
	//for X using linear interpolation
	double findValue(double x) const {
		//Define a lambda that returns true if the x value
		//of a point pair is < the caller's x value
		auto lessThan =
			[](const std::pair<double, double>& point, double x)
		{return point.first < x; };

		//Find the first table entry whose value is >= caller's x value
		auto iter =
			std::lower_bound(_points.cbegin(), _points.cend(), x, lessThan);

		//If the caller's X value is greater than the largest
		//X value in the table, we can't interpolate.
		if (iter == _points.cend()) {
			return (_points.cend() - 1)->second;
		}

		//If the caller's X value is less than the smallest X value in the table,
		//we can't interpolate.
		if (iter == _points.cbegin() && x <= _points.cbegin()->first) {
			return _points.cbegin()->second;
		}

		//We can interpolate!
		double upperX{ iter->first };
		double upperY{ iter->second };
		double lowerX{ (iter - 1)->first };
		double lowerY{ (iter - 1)->second };

		double deltaY{ upperY - lowerY };
		double deltaX{ upperX - lowerX };

		return lowerY + ((x - lowerX) / deltaX) * deltaY;
	}

private:
	//Our container of (x,y) data points
	//std::pair::<double, double>.first = x value
	//std::pair::<double, double>.second = y value
	std::vector<std::pair<double, double>> _points;
};