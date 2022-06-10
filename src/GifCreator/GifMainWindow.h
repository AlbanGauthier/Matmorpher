#pragma once

#include <QMainWindow>

#include <string>
#include <iostream>

#include "GifCreator/GifViewerWidget.h"

class ControlPanel;
class InfoViewer;

/**
 * Minimalistic main window that is filled with a ViewerWidget, for the sake of
 * the example.
 * Actually, we could have made the project slighty smaller by creating in the
 * main() function a ViewerWidget rather than a MainWindow. It would have
 * worked, but since it is very likely that you applicatino will then feature
 * more than one viewer widget (yeah, otherwise what's the point of using Qt at
 * all?) I've added this MainWindow class.
 * It is also a way to isolate user input (e.g. here press Escape to quit) from
 * the rendering logic (located in the ViewerWidget/Viewer classes).
 */
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = 0);

	void setViewerWidgetOutputType(std::string type)
	{
		if (type == "gif")
			viewer_widget_->setOutputType(0);
		else if (type == "texdesign")
			viewer_widget_->setOutputType(1);
		else
			std::cout << "could not define the type of output for gifCreator" << std::endl;
	}

	void setViewerWidgetRenderSize(int renderSize)
	{
		viewer_widget_->setRenderSize(renderSize);

		resize(viewer_widget_->getWidth(), viewer_widget_->getHeight());
	}

	void setMaterialsAndWarpNames(std::string mat1, std::string mat2, std::string warpgrid, std::string warpgrid_td)
	{
		viewer_widget_->setMaterialsAndWarpNames(mat1, mat2, warpgrid, warpgrid_td);
	}

protected:
	// Events are handled exactly as in any Qt program, by overriding the event
	// methods like for instance here keyPressEvent.
	void keyPressEvent(QKeyEvent* event) override;

private: 
	
	QString current_path_;
	ViewerWidget* viewer_widget_;
	
};