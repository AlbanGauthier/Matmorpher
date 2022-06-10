#pragma once

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <math.h>

#include "Warpgrid/Solver.h"
#include "Utils/MathUtils.h"

using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using glm::vec2;

enum class WarpgridType {
    Compute,
    OpenFromFile
};

class Warpgrid
{
public:

    Warpgrid() = delete;
    explicit Warpgrid(int argc, char* argv[], WarpgridType t);

    static int computeWarpgridFromMaps(int argc, char* argv[]);

    bool isLoaded() { return loaded; }
    unsigned int getNumberOfVertices() { return nvertices; }
    unsigned int getGridSideWidth() { return int(sqrt(nvertices)); }
    const vector<float>& getPointDataConstRef() { return pointsData; }

protected:
    unsigned int nvertices, nfaces, nedges;
    bool loaded;
    vector<float> pointsData;

private:
    bool loadTxtFile(const std::string &);
    
    static bool get_next_uncommented_line(std::ifstream &infile, std::string &result)
    {
        while (getline(infile, result))
        {
            if (result.length() >= 1 && result[0] != '#')
            {
                return true;
            }
        }
        return false;
    }
};
