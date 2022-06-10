#include "MathUtils.h"

float Erf(float x)
{
	// Save the sign of x
	int sign = 1;
	if (x < 0)
		sign = -1;
	x = abs(x);

	// A&S formula 7.1.26
	float t = 1.0f / (1.0f + 0.3275911f * x);
	float y = 1.0f - (((((1.061405429f * t + -1.453152027f) * t) + 1.421413741f)
		* t + -0.284496736f) * t + 0.254829592f) * t * exp(-x * x);

	return sign * y;
}

float ErfInv(float x)
{
	float w, p;
	w = -log((1.0f - x) * (1.0f + x));
	if (w < 5.000000f)
	{
		w = w - 2.500000f;
		p = 2.81022636e-08f;
		p = 3.43273939e-07f + p * w;
		p = -3.5233877e-06f + p * w;
		p = -4.39150654e-06f + p * w;
		p = 0.00021858087f + p * w;
		p = -0.00125372503f + p * w;
		p = -0.00417768164f + p * w;
		p = 0.246640727f + p * w;
		p = 1.50140941f + p * w;
	}
	else
	{
		w = sqrt(w) - 3.000000f;
		p = -0.000200214257f;
		p = 0.000100950558f + p * w;
		p = 0.00134934322f + p * w;
		p = -0.00367342844f + p * w;
		p = 0.00573950773f + p * w;
		p = -0.0076224613f + p * w;
		p = 0.00943887047f + p * w;
		p = 1.00167406f + p * w;
		p = 2.83297682f + p * w;
	}
	return p * x;
}

float CDF(float x, float mu, float sigma)
{
	float U = 0.5f * (1 + Erf((x-mu)/(sigma*sqrtf(2.0f))));
	return U;
}

float invCDF(float U, float mu, float sigma)
{
	float x = sigma*sqrtf(2.0f) * ErfInv(2.0f*U-1.0f) + mu;
	return x;
}

float CDFTruncated(float x, float mu, float sigma)
{
	float U = 0.5f * (1 + Erf((x - mu) / (sigma * sqrtf(2.0f))) / Erf(1 / (2.0f * sigma * sqrtf(2.0f))));
	return U;
}

float invCDFTruncated(float U, float mu, float sigma)
{
	float x = sigma * sqrtf(2.0f) * ErfInv((2.0f * U - 1.0f) * Erf(1 / (2.0f * sigma * sqrtf(2.0f)))) + mu;
	return x;
}

float soft_clipping(float x, float W)
{
	if (x > 0.5f)
		return 1 - soft_clipping(1 - x, W);
	else
	{
		if (x >= (2.0f - W) / 4.0f)
			return (x - 1.0f / 2.0f) / W + 1.0f / 2.0f;
		else
		{
			if (W >= 2.0f / 3.0f)
				return 8.0f * (1.0f / W - 1.0f) * pow(x / (2.0f - W), 2.0f)  + (3.0f - 2.0f / W) * x / (2.0f - W);
			else
			{
				if (x >= (2.0f - 3.0f * W) / 4.0f)
					return pow((x - (2.0f - 3.0f * W) / 4.0f) / W, 2.0f);
				else
					return -1.0f;
			}
		}
	}
}