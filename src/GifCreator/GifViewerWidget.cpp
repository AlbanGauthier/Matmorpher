#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif

#include <glad/glad.h> // must be the first include, and for gladLoadGL()

#include <QMouseEvent>
#include <QDirIterator>

#include "GifViewerWidget.h"
#include "Rendering/Viewer.h"
#include "Utils/Camera.h"

using std::string;
using std::cout;
using std::endl;
using std::cerr;

ViewerWidget::ViewerWidget(QWidget* parent)
	: QOpenGLWidget(parent), m_viewerCore(nullptr)
{
	main_layout_ = new QGridLayout;
	setLayout(main_layout_);
	main_layout_->setAlignment(Qt::AlignTop);

	// Inform Qt backend what kind of OpenGL context to allocate for this
	// widget. In level0, this is hidden in utils/windows.h. It is important
	// that this is done before the context is created, so before the widget is
	// shown. In particular, this could not go in initializeGL() which is
	// called right *after* the context has been created.
	// We ask here for a pure ("core") OpenGL 4.5 context.
	QSurfaceFormat format;
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setVersion(4, 6);

	//multisampling
	format.setSamples(16);

#ifndef NDEBUG
	// Without this, setting an OpenGL debug callback with
	// glDebugMessageCallback would have no effect (see utils/debug.cpp)
	format.setOption(QSurfaceFormat::DebugContext);
#endif // !NDEBUG

	setFormat(format);

	// This somehow sets the FPS: render a new frame every 5ms, so 200 FPS
	connect(&m_renderTimer, &QTimer::timeout, this, &ViewerWidget::triggerUpdate);
	m_renderTimer.start(5);

	//reload shaders when modified
	QStringList shaderList = {};
	string shader_loc = "../src/shaders/";
	QDirIterator it(shader_loc.c_str(), QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
		shaderList.push_back(it.next().toStdString().c_str());

	m_watcher = std::unique_ptr<QFileSystemWatcher>(new QFileSystemWatcher(shaderList));

	connect(m_watcher.get(), SIGNAL(fileChanged(const QString&)), this, SLOT(reloadShaders(const QString&)));

	if (shaderList.empty())
	{
		cerr << endl;
		cerr << "The executable could not find the shader folder located in " + shader_loc << endl;
		cerr << endl;
		system("pause");
		exit(EXIT_FAILURE);
	}
}

ViewerWidget::~ViewerWidget()
{
	if (m_viewerCore)
	{
		delete m_viewerCore;
	}
}

void ViewerWidget::initializeGL()
{
	if (gladLoadGL())
	{
		m_viewerCore = new Viewer(scene_state_);

		//m_viewerCore->loadMaterial1("./fish");
		//m_viewerCore->loadMaterial2("./lumber");
	}
}

void ViewerWidget::resizeGL(int w, int h)
{
	//scene_state_.width = w;
	//scene_state_.height = h;

	if (!m_viewerCore)
	{
		return;
	}

	//m_viewerCore->resize(w, h);
}

void ViewerWidget::paintGL()
{
	if (!m_viewerCore)
	{
		return;
	}

	if (m_viewerCore->ready())
		m_viewerCore->render();
}

void ViewerWidget::setMaterialsAndWarpNames(
	std::string mat1, 
	std::string mat2, 
	std::string warpgrid, 
	std::string warpgrid_td)
{
	scene_state_.mat1_path = mat1;
	scene_state_.mat2_path = mat2;
	scene_state_.warp_grid = warpgrid;
	scene_state_.warp_grid_tex = warpgrid_td;
}

void ViewerWidget::reloadShaders(const QString& str)
{
	if (!m_viewerCore) {
		return;
	}
	else {
		if (str.contains("matmorpher"))
			m_viewerCore->reloadShaders(0);
		else if (str.contains("screen"))
			m_viewerCore->reloadShaders(1);
		else if (str.contains("normal"))
			m_viewerCore->reloadShaders(2);
		else if (str.contains("ssao"))
			m_viewerCore->reloadShaders(3);
		else if (str.contains("ssao_blur"))
			m_viewerCore->reloadShaders(4);
	}
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
	auto mouse_pos = event->localPos();
	update();
}

void ViewerWidget::mousePressEvent(QMouseEvent* event)
{
	auto mouse_pos = event->localPos();
	if (event->button() == Qt::LeftButton)
	{
		"coucou";
	}
	else if (event->button() == Qt::RightButton)
	{
		scene_state_.camera.start(Camera::TrackMode::Translate, mouse_pos.rx() /= width(), mouse_pos.ry() /= height());
	}
	else if (event->button() == Qt::MiddleButton)
	{
		scene_state_.camera.start(Camera::TrackMode::Zoom, mouse_pos.rx() /= width(), mouse_pos.ry() /= height());
	}
}

void ViewerWidget::keyPressEvent(QKeyEvent* event)
{
	
}

void ViewerWidget::keyReleaseEvent(QKeyEvent* event)
{
	
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
	
}

void ViewerWidget::wheelEvent(QWheelEvent* event)
{
	float factor = event->angleDelta().y() / (8. * 360);
	scene_state_.camera.zoom(-factor);
}
