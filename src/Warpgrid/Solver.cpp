#include "Solver.h"
#include "WarpUtils.h"

using std::vector;
using std::cout;
using std::endl;

void build_and_solve_linear_system(
	std::vector<vec2> const& PiInit,
	std::vector<vec2> const& Qj,
	Eigen::VectorXd& X,
	unsigned int N, float alpha, float beta,
	unsigned int NIterations) // 10 by default
{
	unsigned int knn = 10;
	ANNidxArray id_nearest_neighbors = new ANNidx[knn];
	ANNdistArray square_distances_to_neighbors = new ANNdist[knn];

	BasicANNkdTree QKdtree;
	QKdtree.setDimension(2);
	QKdtree.build(Qj);

	std::vector<vec2> Pi = PiInit;

	for (unsigned int iter = 0; iter < NIterations; ++iter) {

		unsigned int ncolumns = 2 * N * N; // for x,y coords

		unsigned int equationIndex = 0;

		linearSystem mySystem;

		double pExponent = 0.1;
		double epsilonPrec = 0.001;
		double gaussKernelStd = 0.001;

		for (unsigned int i = 0; i < Pi.size(); i++)
		{
			vec2 pi = Pi[i]; // current point : find closest target points
			QKdtree.knearest(pi, knn, id_nearest_neighbors, square_distances_to_neighbors);

			for (unsigned int jClosestIt = 0; jClosestIt < knn; jClosestIt++)
			{
				mySystem.setDimensions(equationIndex + 2, ncolumns);

				if (int(id_nearest_neighbors[jClosestIt]) < 0 || int(id_nearest_neighbors[jClosestIt]) >= int(Qj.size()))
				{
					cout << "index: " << id_nearest_neighbors[jClosestIt] << endl;
				}

				vec2 qj = Qj[id_nearest_neighbors[jClosestIt]];

				double weight =
					std::exp(-glm::dot(pi - qj, pi - qj) / gaussKernelStd) // localized kernel
					* std::pow(glm::dot(pi - qj, pi - qj) + epsilonPrec, (pExponent - 2) / 2.0);

				std::vector<int> grid_coords;
				float u, v;
				get_bilinear_interpolation(grid_coords, u, v, PiInit[i], N);

				// X coord
				mySystem.A(equationIndex, 2 * grid_coords[0]) = weight * (1 - u) * (1 - v); // Gkl
				mySystem.A(equationIndex, 2 * grid_coords[1]) = weight * u * (1 - v);       // Gk+1l
				mySystem.A(equationIndex, 2 * grid_coords[2]) = weight * (1 - u) * v;       // Gkl+1
				mySystem.A(equationIndex, 2 * grid_coords[3]) = weight * u * v;             // Gk+1l+1
				mySystem.b(equationIndex) = weight * qj[0];
				equationIndex += 1;

				// Y coord
				mySystem.A(equationIndex, 2 * grid_coords[0] + 1) = weight * (1 - u) * (1 - v); // Gkl
				mySystem.A(equationIndex, 2 * grid_coords[1] + 1) = weight * u * (1 - v);       // Gk+1l
				mySystem.A(equationIndex, 2 * grid_coords[2] + 1) = weight * (1 - u) * v;       // Gkl+1
				mySystem.A(equationIndex, 2 * grid_coords[3] + 1) = weight * u * v;             // Gk+1l+1
				mySystem.b(equationIndex) = weight * qj[1];
				equationIndex += 1;
			}
		}

		{
			for (unsigned int l = 0; l < N; l++)
			{
				for (unsigned int k = 0; k < N; k++)
				{
					{
						mySystem.setDimensions(equationIndex + 2, ncolumns);

						double x_shift = 0.0;
						if (k == 0) x_shift = 1.0;
						if (k == N - 1) x_shift = -1.0;
						double y_shift = 0.0;
						if (l == 0) y_shift = 1.0;
						if (l == N - 1) y_shift = -1.0;

						unsigned int k_plus_one = (k == N - 1 ? 1 : k + 1);
						unsigned int k_minus_one = (k == 0 ? N - 2 : k - 1);
						unsigned int l_plus_one = (l == N - 1 ? 1 : l + 1);
						unsigned int l_minus_one = (l == 0 ? N - 2 : l - 1);

						mySystem.A(equationIndex, 2 * (k + l * N)) = -4.0 * alpha;    // x_tkl
						mySystem.A(equationIndex, 2 * (k_minus_one + l * N)) = alpha; // x_tk-1l
						mySystem.A(equationIndex, 2 * (k_plus_one + l * N)) = alpha; // x_tk+1l
						mySystem.A(equationIndex, 2 * (k + l_minus_one * N)) = alpha; // x_tkl-1
						mySystem.A(equationIndex, 2 * (k + l_plus_one * N)) = alpha; // x_tkl+1
						mySystem.b(equationIndex) = x_shift * alpha;
						equationIndex += 1;

						mySystem.A(equationIndex, 2 * (k + l * N) + 1) = -4.0 * alpha;    // y_tkl
						mySystem.A(equationIndex, 2 * (k + l_minus_one * N) + 1) = alpha; // y_tkl-1
						mySystem.A(equationIndex, 2 * (k + l_plus_one * N) + 1) = alpha; // y_tkl+1
						mySystem.A(equationIndex, 2 * (k_minus_one + (l * N)) + 1) = alpha; // y_tk-1l
						mySystem.A(equationIndex, 2 * (k_plus_one + (l * N)) + 1) = alpha; // y_tk+1l
						mySystem.b(equationIndex) = y_shift * alpha;
						equationIndex += 1;
					}

					// vertical edges
					if ((k == 0 || k == N - 1) && l != 0 && l != N - 1)
					{
						mySystem.setDimensions(equationIndex + 1, ncolumns);

						mySystem.A(equationIndex, 2 * (k + l * N)) = beta; // x_tkl
						if (k == N - 1) mySystem.b(equationIndex) = beta;
						else mySystem.b(equationIndex) = 0.0;
						equationIndex += 1;

						if (k == 0) {
							mySystem.setDimensions(equationIndex + 1, ncolumns);
							// add only once this condition to make the results "tilable" in the weak sense (-> periodic)
							mySystem.A(equationIndex, 2 * ((N - 1) + l * N) + 1) = -beta;
							mySystem.A(equationIndex, 2 * (0 + l * N) + 1) = beta;
							mySystem.b(equationIndex) = 0.0;
							equationIndex += 1;
						}
					}

					// horizontal edges
					if ((l == 0 || l == N - 1) && k != 0 && k != N - 1)
					{
						mySystem.setDimensions(equationIndex + 1, ncolumns);

						mySystem.A(equationIndex, 2 * (k + l * N) + 1) = beta; // y_tkl
						if (l == N - 1) mySystem.b(equationIndex) = beta;
						else mySystem.b(equationIndex) = 0.0;
						equationIndex += 1;


						if (l == 0) {
							mySystem.setDimensions(equationIndex + 1, ncolumns);
							// add only once this condition to make the results "tilable" in the weak sense (-> periodic)
							mySystem.A(equationIndex, 2 * (k + (N - 1) * N)) = -beta;
							mySystem.A(equationIndex, 2 * (k + 0 * N)) = beta;
							mySystem.b(equationIndex) = 0.0;
							equationIndex += 1;
						}
					}

					// corners
					if ((k == 0 || k == N - 1) && (l == 0 || l == N - 1))
					{
						mySystem.setDimensions(equationIndex + 2, ncolumns);

						mySystem.A(equationIndex, 2 * (k + l * N)) = beta; //x_tkl
						if (k == N - 1) mySystem.b(equationIndex) = beta;  else mySystem.b(equationIndex) = 0.0;
						equationIndex += 1;

						mySystem.A(equationIndex, 2 * (k + l * N) + 1) = beta; //y_tkl
						if (l == N - 1) mySystem.b(equationIndex) = beta;  else mySystem.b(equationIndex) = 0.0;
						equationIndex += 1;
					}
				}
			}
		}

		mySystem.preprocess();
		mySystem.solve(X);

		float advectionStep = float((iter + 1) / NIterations);

		std::cout << "Linear system solve: " << iter << "/" << NIterations - 1 << std::endl;

		for (unsigned int i = 0; i < Pi.size(); ++i) {

			// find out where the grid put the point:
			std::vector<int> grid_coords;
			float u, v;
			get_bilinear_interpolation(grid_coords, u, v, PiInit[i], N);

			vec2 PiTarget = (1 - u) * (1 - v) * vec2(X[2 * grid_coords[0]], X[2 * grid_coords[0] + 1]) +
				u * (1 - v) * vec2(X[2 * grid_coords[1]], X[2 * grid_coords[1] + 1]) +
				(1 - u) * v * vec2(X[2 * grid_coords[2]], X[2 * grid_coords[2] + 1]) +
				u * v * vec2(X[2 * grid_coords[3]], X[2 * grid_coords[3] + 1]);

			Pi[i] += advectionStep * (PiTarget - Pi[i]);
		}
	}

	delete[] id_nearest_neighbors;
	delete[] square_distances_to_neighbors;
}

void compute_and_serialize_warpgrid(
	std::vector<vec2> const& P_xy,
	std::vector<vec2> const& Q_xy,
	Params const& cmd_inputs)
{
	// init system solution
	Eigen::VectorXd X;
	std::vector<vec2> P_xy_transformed;

	build_and_solve_linear_system(P_xy, Q_xy, X, cmd_inputs.grid_size, cmd_inputs.alpha, cmd_inputs.beta);

	std::string filename = "warp_" 
		+ cmd_inputs.filename_P
		+ "_" 
		+ cmd_inputs.filename_Q 
		+ ".png";

	saveGridImage(X, filename, cmd_inputs.grid_size);

	writeIntoFile(X, cmd_inputs.filename_P + "_" + cmd_inputs.filename_Q);
}