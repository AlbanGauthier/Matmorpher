#include <QKeyEvent>
#include <QMenuBar>
#include <QFileDialog>
#include <QSplitter>

#include "GifMainWindow.h"
#include "GifViewerWidget.h"

MainWindow::MainWindow(QWidget* parent)
{
	setWindowTitle("GifCreator");
	
	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	setCentralWidget(splitter);

	// opengl view
	viewer_widget_ = new ViewerWidget(this);
	viewer_widget_->setFocusPolicy(Qt::ClickFocus);
	splitter->addWidget(viewer_widget_);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape) {
		close();
	}
}