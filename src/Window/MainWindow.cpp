#ifdef _WIN32
#include <windows.h>
#elif !defined(APIENTRY)
#define APIENTRY
#endif
#include <glad/glad.h>

#include "MainWindow.h"
#include "ViewerWidget.h"
#include "ControlPanel.h"

#include <QKeyEvent>
#include <QMenuBar>
#include <QFileDialog>
#include <QSplitter>

// settings
const unsigned int SCREEN_WIDTH = 1400;
const unsigned int SCREEN_HEIGHT = 650;

MainWindow::MainWindow(QWidget* parent)
{
	setWindowTitle("MatMorpher");
	resize(SCREEN_WIDTH, SCREEN_HEIGHT);

	QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
	splitter->setChildrenCollapsible(false);
	setCentralWidget(splitter);

	// opengl (left)
	viewer_widget_ = new ViewerWidget(this);
	viewer_widget_->setFocusPolicy(Qt::ClickFocus);
	splitter->addWidget(viewer_widget_);

	// control (right)
	controls_ = new ControlPanel(splitter);
	controls_->resize(0, 0);
	splitter->addWidget(controls_);

	splitter->setSizes({1250, 150});

	connect(viewer_widget_, SIGNAL(resizeToMinimum(bool)), this, SLOT(resizeToMinimum(bool)));

	connect(controls_, SIGNAL(ColorChanged(float)), viewer_widget_, SLOT(setColorMix(float)));
	connect(controls_, SIGNAL(RoughnessChanged(float)), viewer_widget_, SLOT(setRoughnessMix(float)));
	connect(controls_, SIGNAL(MetallicChanged(float)), viewer_widget_, SLOT(setMetallicMix(float)));
	connect(controls_, SIGNAL(NormalChanged(float)), viewer_widget_, SLOT(setNormalMix(float)));
	connect(controls_, SIGNAL(HeightChanged(float)), viewer_widget_, SLOT(setHeightMix(float)));
	connect(controls_, SIGNAL(interpolationChanged(float)), viewer_widget_, SLOT(setInterpolationMix(float)));
	
	connect(controls_, SIGNAL(meshIsSelected(int)), viewer_widget_, SLOT(setMeshToRender(int)));
	
	connect(controls_, SIGNAL(setColorStatus(bool)), viewer_widget_, SLOT(setColorStatus(bool)));
	connect(controls_, SIGNAL(setMetallicStatus(bool)), viewer_widget_, SLOT(setMetallicStatus(bool)));
	connect(controls_, SIGNAL(setRoughnessStatus(bool)), viewer_widget_, SLOT(setRoughnessStatus(bool)));
	connect(controls_, SIGNAL(setNormalStatus(bool)), viewer_widget_, SLOT(setNormalStatus(bool)));
	connect(controls_, SIGNAL(setHeightStatus(bool)), viewer_widget_, SLOT(setHeightStatus(bool)));

	connect(controls_, SIGNAL(HeightFactorChanged(float)), viewer_widget_, SLOT(setHeightFactor(float)));
	connect(controls_, SIGNAL(TessLevelChanged(int)), viewer_widget_, SLOT(setTesselationLevel(int)));
	connect(controls_, SIGNAL(radiusChanged(float)), viewer_widget_, SLOT(setSSAORadius(float)));

	connect(viewer_widget_, SIGNAL(colorHasChanged(float)), controls_, SLOT(colorHasChanged(float)));
	connect(viewer_widget_, SIGNAL(metallicHasChanged(float)), controls_, SLOT(metallicHasChanged(float)));
	connect(viewer_widget_, SIGNAL(roughnessHasChanged(float)), controls_, SLOT(roughnessHasChanged(float)));
	connect(viewer_widget_, SIGNAL(normalHasChanged(float)), controls_, SLOT(normalHasChanged(float)));
	connect(viewer_widget_, SIGNAL(heightHasChanged(float)), controls_, SLOT(heightHasChanged(float)));
	connect(viewer_widget_, SIGNAL(globalInterpolationChanged(float)), controls_, SLOT(globalInterpolationChanged(float)));
	connect(viewer_widget_, SIGNAL(tessLevelChanged(float)), controls_, SLOT(tessLevelChanged(float)));

	// Menu Bar
	menu_bar_ = new QMenuBar(this);
	setMenuBar(menu_bar_);

	QMenu* file_menu = menu_bar_->addMenu("File");
	QAction* open_action_1 = file_menu->addAction("Open Material 1");
	connect(open_action_1, SIGNAL(triggered(bool)), this, SLOT(openMaterial1()));

	QAction* open_action_2 = file_menu->addAction("Open Material 2");
	connect(open_action_2, SIGNAL(triggered(bool)), this, SLOT(openMaterial2()));

	QAction* open_action_3 = file_menu->addAction("Open Warp Grid");
	connect(open_action_3, SIGNAL(triggered(bool)), this, SLOT(openWarpGrid()));

	QAction* exit_action = file_menu->addAction("Exit");
	exit_action->setShortcut(QKeySequence(Qt::Key_Escape));
	connect(exit_action, SIGNAL(triggered(bool)), this, SLOT(close()));
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape) {
		close();
	}
}

void MainWindow::resizeToMinimum(bool tex_view)
{
	if (tex_view)
		resize(100, 100);
	else
		resize(800, 800);
}

void MainWindow::openMaterials(int argc, char* argv[])
{
	QFileInfo fileinfo(argv[2]);
	if (fileinfo.exists())
	{
		viewer_widget_->loadMaterial1(fileinfo.absoluteFilePath());
	}
	else
	{
		std::cout << "could not find: " << argv[2] << std::endl;
	}
	if (argc > 3)
	{
		QFileInfo fileinfo(argv[3]);
		if (fileinfo.exists())
		{
			viewer_widget_->loadMaterial2(fileinfo.absoluteFilePath());
		}
		else
		{
			std::cout << "could not find: " << argv[3] << std::endl;
		}
	}
	if (argc > 4)
	{
		QFileInfo fileinfo(argv[4]);
		if (fileinfo.exists())
		{
			viewer_widget_->loadWarpGrid(fileinfo.absoluteFilePath());
		}
		else
		{
			std::cout << "could not find: " << argv[4] << std::endl;
		}
	}
}

void MainWindow::openMaterial1()
{
	QString path = QFileDialog::getExistingDirectory(this,
		tr("Open Material 1 Directory"), current_path_);
	QFileInfo fileinfo(path);
	if (fileinfo.exists())
		viewer_widget_->loadMaterial1(path);
}

void MainWindow::openMaterial2()
{
	QString path = QFileDialog::getExistingDirectory(this,
		tr("Open Material 2 Directory"), current_path_);
	QFileInfo fileinfo(path);
	if (fileinfo.exists())
		viewer_widget_->loadMaterial2(path);
}

void MainWindow::openWarpGrid()
{
	QString path = QFileDialog::getOpenFileName(this,
		tr("Open warp grid file"), current_path_, tr("File (*.txt)"));
	QFileInfo fileinfo(path);
	if (fileinfo.exists())
		viewer_widget_->loadWarpGrid(path);
}

void MainWindow::renameWindowTitle()
{
	setWindowTitle(QString::fromStdString(viewer_widget_->getWindowTitle()));
}
