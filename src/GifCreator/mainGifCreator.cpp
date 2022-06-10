#include <QApplication>

// Visual Leak Detector
// https://github.com/KindDragon/vld
//
//#include <vld.h>

#include "GifCreator/GifMainWindow.h"

#include <iostream>

using std::cout;
using std::endl;

void printUsageForGifCreation()
{
	cout << endl
		<< "GifCreator CommandLine Help:" << endl
		<< "------------------" << endl
		<< "GifCreator.exe gif 1024 mat1 mat2 warpgrid.txt" << endl
		<< "------------------" << endl
		<< "GifCreator.exe texdesign 128 mat1 mat2 warpgrid.txt _td" << endl
		<< endl;
}

int mainGifCreator(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindow window;
	
	window.setViewerWidgetOutputType(argv[1]);

	window.setViewerWidgetRenderSize(std::atoi(argv[2]));

	window.setMaterialsAndWarpNames(argv[3], argv[4], argv[5], argv[6]);

	window.show();

	return app.exec();
}

int main(int argc, char* argv[])
{
	std::string cmd;
	if (argc < 2) // no args : default materials
	{
		//argc = 6;
		//char* new_argv[6] =
		//{
		//	"gifCreator",
		//	"gif",
		//	"1024",
		//	"armor4K",
		//	"lumber4K",
		//	"new_warp_armor4K_lumber4K.txt",
		//	//"identity.txt"
		//};
		//mainGifCreator(argc, new_argv);

		cmd = "-h";
	}
	else
	{
		cmd = argv[1];
	}
	if (cmd == "-h" || cmd == "-help")
	{
		printUsageForGifCreation();
		return EXIT_SUCCESS;
	}
	else if (cmd == "gif" || cmd == "texdesign") 
	{
		// gif 1024 armor4K lumber4K warpgrid.txt
		if (argc >= 7)
		{
			return mainGifCreator(argc, argv);
		}
		else
		{
			cout << "not enough arguments for GifCreator" << endl;
		}
	}
	else 
	{
		std::cerr << "Unknown command '" << cmd << "'" << std::endl << std::endl;
		printUsageForGifCreation();
		return EXIT_FAILURE;
	}
}