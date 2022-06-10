#include "Window/MainWindow.h"
#include "Utils/Contours.h"
#include "Warpgrid/Warpgrid.h"
#include "Warpgrid/WarpTextureDesign.h"

#include <QApplication>
#include <iostream>

void printUsageForExecutable()
{
	cout << endl
		<< "MatMorpher Command Line Help:" << endl
		<< "------------------" << endl
		<< " ./MatMorpher" << endl
		<< " Description: Launch the GUI with default materials and warpgrid" << endl
		<< "------------------" << endl
		<< " ./MatMorpher gui material1_folder material2_folder warpgrid.txt" << endl
		<< " Description: Launch the GUI with materials 1 and 2 and the warpgrid" << endl
		<< "------------------" << endl
		<< " ./MatMorpher contours material_folder" << endl
		<< " Description: Apply contour detection on all maps inside material_folder" << endl
		<< " and output results in the same folder as .pdf" << endl
		<< "------------------" << endl
		<< " ./MatMorpher warpgrid mat1_folder XXXXX mat2_folder XXXXX grid_size alpha beta" << endl
		<< " Description: Compute and output the warpgrid between material 1 and 2" << endl
		<< " replace each X in XXXXX with 0 or 1, to use color, height, metallic, normal, roughness" << endl
		<< " for example, 01000 will use only the mat_folder/height.png" << endl
		<< " Optionnal arguments:" << endl
		<< " - grid_size is the height/width of the warpgrid(default: 128)" << endl
		<< " - alpha modulates the interior regularity term(default: 4000)" << endl
		<< " - beta modulates the periodic harmonicity term(default: 200)" << endl
	<< endl;
}

int mainGui(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindow window;

	window.show();

	if (argc > 2)
		window.openMaterials(argc, argv);

	return app.exec();
}

int main(int argc, char* argv[])
{
	std::string cmd;
	if (argc < 2) // no args : default materials
	{
		const int new_argc = 5;
		char* new_argv[new_argc] =
		{
			(char*)"MatMorpher",
			(char*)"gui",
			(char*)"fish4K",
			(char*)"lumber4K",
			(char*)"warp_fish4K_lumber4K.txt"
		};

		mainGui(new_argc, new_argv);
	}
	else
	{
		cmd = argv[1];

		if (cmd == "-h" || cmd == "-help")
		{
			printUsageForExecutable();
			return EXIT_SUCCESS;
		}
		else if (cmd == "gui") {
			// gui clover fish warp_clover_fish.txt
			return mainGui(argc, argv);
		}
		else if (cmd == "contours") {
			// contours clover
			if (argc >= 3)
			{
				return mainContour(argc, argv);
			}
			else
			{
				std::cerr << "too few arguments for command contours" << std::endl;
				return EXIT_FAILURE;
			}
		}
		else if (cmd == "warpgrid") {
			// warpgrid clover4K 01000 fish4K 01000 128 4000 200
			if (argc == 6 || argc == 9)
			{
				Warpgrid warpgrid(argc, argv, WarpgridType::Compute);
				return EXIT_SUCCESS;
			}
			else
			{
				std::cerr << "wrong number of arguments for command warpgrid" << std::endl;
				return EXIT_FAILURE;
			}
		}
		else {
			std::cerr << "Unknown command '" << cmd << "'" << std::endl << std::endl;
			printUsageForExecutable();
			return EXIT_FAILURE;
		}
	}
}