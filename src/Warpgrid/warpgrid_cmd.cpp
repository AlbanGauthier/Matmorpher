#include "Warpgrid/solver.h"

void printUsage()
{
    cerr << "Usage : ./exe_name method_nb N alpha beta Pi Qj" << endl;
}

int mainWarpgrid(int argc, char ** argv) 
{    
    if (argc < 7)
    {
        printUsage ();
        exit (EXIT_FAILURE);
    }

    float anisotropyFactor = 0.0;
    if (argc >= 8 ) anisotropyFactor = std::stof(std::string(argv[7]));

    std::string mat1_fullname = std::string(argv[5]);
    size_t lastindex = mat1_fullname.find_last_of(".");
    string mat1_rawname = mat1_fullname.substr(0, lastindex);

    std::string mat2_fullname = std::string(argv[6]);
    lastindex = mat2_fullname.find_last_of(".");
    string mat2_rawname = mat2_fullname.substr(0, lastindex);

    // read cmd inputs
    Params cmd_inputs =
    {
        std::stoi(std::string(argv[1])),
        std::stoi(std::string(argv[2])),
        std::stof(std::string(argv[3])),
        std::stof(std::string(argv[4])),
        std::string(argv[3]),
        std::string(argv[4]),
        mat1_rawname,
        mat2_rawname,
        anisotropyFactor
    };

    // auto start = std::chrono::system_clock::now();

    // fill vectors
    std::vector<vec2> P_xy;
    std::vector<vec2> Q_xy;
    if(!loadTxtFile(mat1_fullname, P_xy))
    {
        std::cerr << "cannot read material 1 values from file: " << mat1_fullname << std::endl;
        exit (EXIT_FAILURE);
    }
    if(!loadTxtFile(mat2_fullname, Q_xy))
    {
        std::cerr << "cannot read material 1 from file: " << mat2_fullname << std::endl;
        exit (EXIT_FAILURE);
    }

    //    // mirror y for Q:
    //    for( Vec2 & p : Q_xy )
    //        p[1] = 1.0 - p[1];

    switch (cmd_inputs.method_nb)
    {
    case 1:
        execute_method_1(P_xy, Q_xy, cmd_inputs);
        break;
    case 3:
        execute_method_3(P_xy, Q_xy, cmd_inputs);
        break;
    case 4:
        execute_method_4(P_xy, Q_xy, cmd_inputs);
        break;
    case 5:
        execute_method_5(P_xy, Q_xy, cmd_inputs);
        break;
    default:
        std::cerr << "method number " << cmd_inputs.method_nb << " does not exist" << std::endl;
        break;
    }

    // benchmarker
    // auto end = std::chrono::system_clock::now();
    // std::chrono::duration<double> diff = end-start;
    // std::cout << "main executed in " << diff.count() << " s" << std::endl;

    // testlinearSystem();

    // Eigen::MatrixXd idMatrix(3,3);
    // idMatrix(0,0) = 1.0;   idMatrix(1,1) = 1.0;   idMatrix(2,2) = 1.0;
    // idMatrix(0,1) = 0.0;   idMatrix(1,0) = 0.0;   idMatrix(2,0) = 0.0;
    // idMatrix(0,2) = 0.0;   idMatrix(1,2) = 0.0;   idMatrix(2,1) = 0.0;

    return EXIT_SUCCESS;
}

