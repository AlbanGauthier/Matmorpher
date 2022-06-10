#include "Warpgrid.h"
#include "Utils/Contours.h"

#include <iomanip>
#include <sstream>

Warpgrid::Warpgrid(int argc, char* argv[], WarpgridType t) : nvertices(0), nfaces(0), nedges(0)
{
    loaded = false;
    switch (t)
    {
    case WarpgridType::Compute:
        loaded = computeWarpgridFromMaps(argc, argv);
        break;
    case WarpgridType::OpenFromFile:
        loaded = loadTxtFile(argv[0]);
        break;
    default:
        break;
    }
}

bool Warpgrid::loadTxtFile(const std::string & filename)
{
    // 0 - open input file
    std::ifstream infile(filename);
    if (!infile.is_open())
    {
        std::cerr << "failed to open the file: " << filename << std::endl;
        return false;
    }

    // 1 - number of vertices
    std::string info;
    if (!get_next_uncommented_line(infile, info))
    {
        return false;
    }
    std::istringstream info_stream;
    info_stream.str(info);

    info_stream >> nvertices;

    if(nvertices != 0) {
        std::cout << "nb of vertices of the warp grid: " << int(sqrt(nvertices)) << "x" << int(sqrt(nvertices)) << std::endl;
    }

    pointsData.reserve(2 * size_t(nvertices));
    for (unsigned int i = 0; i < nvertices; ++i)
    {
        if (!get_next_uncommented_line(infile, info))
        {
            return false;
        }
        std::istringstream info_stream(info);
        glm::vec2 point;
        info_stream >> point.x >> point.y;
        pointsData.emplace_back(point.x);
        pointsData.emplace_back(point.y);
    }

    infile.close();

    return true;
}

int Warpgrid::computeWarpgridFromMaps(int argc, char* argv[])
{
	std::string mat1 = std::string(argv[2]);
	int mat_to_use1 = std::stoi(argv[3], nullptr, 2);
    std::string mat2 = std::string(argv[4]);
	int mat_to_use2 = std::stoi(argv[5], nullptr, 2);

	int grid_size = 128;
	int alpha = 200;
	int beta = 4000;

	if (argc >= 7)
	{
		grid_size	= std::stoi(std::string(argv[6]));
		alpha		= std::stof(std::string(argv[7]));
		beta		= std::stof(std::string(argv[8]));
	}

    // read cmd inputs
    Params cmd_inputs =
    {
		mat1,
		mat2,
		std::to_string(alpha),
		std::to_string(beta),
		grid_size,
		alpha,
		beta
    };

    // fill vectors
    std::vector<vec2> P_xy;
    std::vector<vec2> Q_xy;

    if (!getContoursListFromMaps(mat1, mat_to_use1, P_xy))
    {
        std::cerr << "cannot extract contour list from material: " << mat1 << std::endl;
        exit(EXIT_FAILURE);
    }

    if (!getContoursListFromMaps(mat2, mat_to_use2, Q_xy))
    {
        std::cerr << "cannot extract contour list from material: " << mat2 << std::endl;
        exit(EXIT_FAILURE);
    }

    compute_and_serialize_warpgrid(P_xy, Q_xy, cmd_inputs);

    return EXIT_SUCCESS;
}