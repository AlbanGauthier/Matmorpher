#include "WarpUtils.h"

double square(double x) { return x * x; }

double cubic(double x) { return x * x * x; }

float kernel_phi(float r, float h)
{
    return std::exp(-std::pow(r / h, 2.0f));
}

double GaussianKernel(double r, double h) {
    return exp(-r * r / (2 * h * h));
}

//https://num.math.uni-goettingen.de/schaback/teaching/sc.pdf
double WendlandKernel(double r, double h) {
    if (r > h)
        return 0.0;
    return square(square(1 - r / h)) * (4 * r / h + 1);
}

void get_bilinear_interpolation(std::vector<int>& grid_coords, float& lambda, float& mu, vec2 xy, unsigned int N)
{
    vec2 xy_scaled = (N - 1.0f) * xy;

    // clamping inside the grid
    int floor_x = std::max(std::min(int(std::floor(xy_scaled[0])), int(N - 1)), 0);
    int floor_y = std::max(std::min(int(std::floor(xy_scaled[1])), int(N - 1)), 0);

    int G_kl = floor_x + N * floor_y;
    int G_k1l = floor_x + 1 + N * floor_y;
    int G_kl1 = floor_x + N * (floor_y + 1);
    int G_k1l1 = floor_x + 1 + N * (floor_y + 1);

    // collapsing cell horizontally
    if (xy[0] <= 0.f || xy[0] >= (N - 1))
    {
        G_k1l = G_kl;
        G_k1l1 = G_kl1;
    }
    // collapsing cell vertically
    if (xy[1] <= 0.f || xy[1] >= (N - 1))
    {
        G_kl1 = G_kl;
        G_k1l1 = G_k1l;
    }

    grid_coords = { G_kl, G_k1l, G_kl1, G_k1l1 };

    // decimal part
    lambda = xy_scaled[0] - int(std::floor(xy_scaled[0]));
    mu = xy_scaled[1] - int(std::floor(xy_scaled[1]));
}

vec2 get_grid_cell(int coord, unsigned int N)
{
    int y_coord = int(coord / N);
    int x_coord = coord % N;
    return vec2(float(x_coord) / float(N - 1), float(y_coord) / float(N - 1));
}
