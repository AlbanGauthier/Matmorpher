#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif

#include <glad/glad.h> // must be the first include, and for gladLoadGL()

#include <QMouseEvent>
#include <QDirIterator>

#include "ViewerWidget.h"
#include "Rendering/Viewer.h"
#include "Rendering/Model.h"
#include "Utils/Camera.h"

using std::string;
using std::cout;
using std::endl;
using std::cerr;

void printUsage()
{
	cout << "Viewport commands:" << endl
		<< "------------------"			<< endl
		<< " <LMB>: Rotate model"		<< endl
		<< " <RMB>: Translate model"	<< endl
		<< " <mouse wheel>: Zoom"		<< endl
		<< " <+> / <-> : Tilt"			<< endl
		<< " <ctrl> + <LMB>: Interpolate" << endl
		<< " <numpad 0> : PBR rendering" << endl
		<< " <numpad 1> : basecolor"	<< endl
		<< " <numpad 2> : height"		<< endl
		<< " <numpad 3> : metallic"		<< endl
		<< " <numpad 4> : normalmap"		<< endl
		<< " <numpad 5> : roughness"	<< endl
		<< " <numpad 6> : detail map"	<< endl
		<< " <numpad 7> : geometric normal" << endl
		<< " <numpad 8> : ssao" << endl
		<< " <ctrl> + <numpad 7>: Save screenshot" << endl
		<< " <ctrl> + <numpad 8>: Switch wireframe mode" << endl
		<< " <ctrl> + <numpad 9>: Change background color" << endl
		<< " <I key> Display info" << endl
		<< " <Y key> RGB / YCbCr for albedo interpolation" << endl
		<< endl;
}

ViewerWidget::ViewerWidget(QWidget* parent)
	: QOpenGLWidget(parent), m_viewerCore(nullptr)
{
	main_layout_ = new QGridLayout;
	setLayout(main_layout_);
	main_layout_->setAlignment(Qt::AlignTop);

	QSurfaceFormat format;
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setVersion(4, 6);

#ifndef NDEBUG
	format.setOption(QSurfaceFormat::DebugContext);
#endif // !NDEBUG

	setFormat(format);

	connect(&m_renderTimer, &QTimer::timeout, this, &ViewerWidget::triggerUpdate);
	m_renderTimer.start(10);

	m_infoviewer = new InfoViewer(this, scene_state_);
	main_layout_->addWidget(m_infoviewer, 0, 0);

	connect(this, SIGNAL(reloadInfo()), m_infoviewer, SLOT(updateInfo()));

	connect(&m_titleTimer, SIGNAL(timeout()), parent, SLOT(renameWindowTitle()));
	m_titleTimer.start(100);

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
	else
		printUsage();
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
		emit reloadInfo();
	}
}

void ViewerWidget::resizeGL(int w, int h)
{
	scene_state_.width = w;
	scene_state_.height = h;

	if (!m_viewerCore)
	{
		return;
	}

	m_viewerCore->resize(w, h);
	emit reloadInfo();
}

void ViewerWidget::paintGL()
{
	if (!m_viewerCore)
	{
		return;
	}
	if (!display_infos)
		m_infoviewer->hide();
	else
		m_infoviewer->show();
	m_viewerCore->render();
}

const std::string& ViewerWidget::getWindowTitle()
{
	return m_infoviewer->getTitle();
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
		{
			if (str.contains("blur"))
				m_viewerCore->reloadShaders(4);
			else
				m_viewerCore->reloadShaders(3);
		}
	}
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
	auto mouse_pos = event->localPos();

	if (lmbIsPressed && ctrlIsPressed)
	{
		float newValue	= mouse_pos.x() / float( width());
		float newValue2 = mouse_pos.y() / float( height());
		if (colorChecked)
		{
			scene_state_.mix_color = newValue;
			emit colorHasChanged(newValue);
		}
		if (metallicChecked)
		{
			scene_state_.mix_metallic = newValue;
			emit metallicHasChanged(newValue);
		}
		if (roughnessChecked)
		{
			scene_state_.mix_roughness = newValue;
			emit roughnessHasChanged(newValue);
		}
		if (normalChecked)
		{
			scene_state_.mix_normal = newValue;
			emit normalHasChanged(newValue);
		}
		if (heightChecked)
		{
			scene_state_.mix_height = newValue;
			emit heightHasChanged(newValue);
		}
		scene_state_.global_interpolation = newValue;
		emit globalInterpolationChanged(newValue);
	}
	else
	{
		mouse_pos.rx() /= width();
		mouse_pos.ry() /= height();
		scene_state_.camera.track(mouse_pos.x(), mouse_pos.y());
	}
	update();
}

void ViewerWidget::mousePressEvent(QMouseEvent* event)
{
	auto mouse_pos = event->localPos();
	if (event->button() == Qt::LeftButton)
	{
		lmbIsPressed = true;
		if (ctrlIsPressed)
		{
			float newValue = mouse_pos.x() / float(width());
			float newValue2 = mouse_pos.y() / float(height());
			if (colorChecked)
			{
				scene_state_.mix_color = newValue;
				emit colorHasChanged(newValue);
			}
			if (metallicChecked)
			{
				scene_state_.mix_metallic = newValue;
				emit metallicHasChanged(newValue);
			}
			if (roughnessChecked)
			{
				scene_state_.mix_roughness = newValue;
				emit roughnessHasChanged(newValue);
			}
			if (normalChecked)
			{
				scene_state_.mix_normal = newValue;
				emit normalHasChanged(newValue);
			}
			if (heightChecked)
			{
				scene_state_.mix_height = newValue;
				emit heightHasChanged(newValue);
			}
			scene_state_.global_interpolation = newValue;
			emit globalInterpolationChanged(newValue);
		}
		else
			scene_state_.camera.start(Camera::TrackMode::Rotate, mouse_pos.rx() /= width(), mouse_pos.ry() /= height());
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
	if (event->key() == Qt::Key_Control)
	{
		ctrlIsPressed = true;
	}
	else if (event->key() == Qt::Key_Alt)
	{
		altIsPressed = true;
	}
	else if (event->key() == Qt::Key_I)
	{
		display_infos = display_infos ? false : true;
	}
	else if (event->key() == Qt::Key_Y)
	{
		scene_state_.ycbcr = scene_state_.ycbcr ? false : true;
		m_viewerCore->reloadGaussianTextures();
	}
	else if (event->key() == Qt::Key_0)
	{
		scene_state_.debug_view_mode = DebugViewMode::PBRView;
	}
	else if (event->key() == Qt::Key_1)
	{
		scene_state_.debug_view_mode = DebugViewMode::BaseColor;
	}
	else if (event->key() == Qt::Key_2)
	{
		scene_state_.debug_view_mode = DebugViewMode::Height;
	}
	else if (event->key() == Qt::Key_3)
	{
		scene_state_.debug_view_mode = DebugViewMode::Metallic;
	}
	else if (event->key() == Qt::Key_4)
	{
		scene_state_.debug_view_mode = DebugViewMode::Normal;
	}
	else if (event->key() == Qt::Key_5)
	{
		scene_state_.debug_view_mode = DebugViewMode::Roughness;
	}
	else if (event->key() == Qt::Key_6)
	{
		scene_state_.debug_view_mode = DebugViewMode::Detail;
	}
	else if (event->key() == Qt::Key_7)
	{
		if (ctrlIsPressed)
			m_viewerCore->captureScreenshot();
		else
			scene_state_.debug_view_mode = DebugViewMode::GeomNormal;
	}
	else if (event->key() == Qt::Key_8)
	{
		if (ctrlIsPressed)
			scene_state_.wireframe = scene_state_.wireframe ? false : true;
		else
			scene_state_.debug_view_mode = DebugViewMode::SSAO;
	}
	else if (event->key() == Qt::Key_9)
	{
		if (ctrlIsPressed)
			scene_state_.dark_background = scene_state_.dark_background ? false : true;
	}
	else if (event->key() == Qt::Key_Plus)
	{
		scene_state_.camera.tilt(0.1f);
	}
	else if (event->key() == Qt::Key_Minus)
	{
		scene_state_.camera.tilt(-0.1f);
	}
	emit reloadInfo();
}

void ViewerWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Control)
		ctrlIsPressed = false;
	else if (event->key() == Qt::Key_Alt)
		altIsPressed = false;
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::RightButton || event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)
		scene_state_.camera.stop();
	if (event->button() == Qt::RightButton)
		lmbIsPressed = false;
}

void ViewerWidget::wheelEvent(QWheelEvent* event)
{
	float factor = event->angleDelta().y() / (8. * 360);
	scene_state_.camera.zoom(-factor);
}

void ViewerWidget::loadMaterial1(QString filename)
{
	// Makes the context current in the current thread : prevents parallel thread exec
	makeCurrent();
	m_viewerCore->loadMaterial(filename.toStdString(), 1);
	emit reloadInfo();
}

void ViewerWidget::loadMaterial2(QString filename)
{
	// Makes the context current in the current thread : prevents parallel thread exec
	makeCurrent();
	m_viewerCore->loadMaterial(filename.toStdString(), 2);
	emit reloadInfo();
}

void ViewerWidget::loadWarpGrid(QString filename)
{
	// Makes the context current in the current thread : prevents parallel thread exec
	makeCurrent();
	m_viewerCore->loadWarpGrid(filename.toStdString());
	emit reloadInfo();
}

void ViewerWidget::setMeshToRender(int radioBtnIdx)
{
	makeCurrent();
	scene_state_.renderedMesh = static_cast<RenderedMesh>(radioBtnIdx);
	if (scene_state_.renderedMesh == RenderedMesh::Plane)
	{
		scene_state_.tess_level = 16;
		emit tessLevelChanged(scene_state_.tess_level);
	}
	else if (scene_state_.renderedMesh == RenderedMesh::Sphere)
	{
		scene_state_.tess_level = 48;
		emit tessLevelChanged(scene_state_.tess_level);
	}
	m_viewerCore->setupMesh();
}

void ViewerWidget::setColorMix(float val)
{
	scene_state_.mix_color = val;
}
void ViewerWidget::setRoughnessMix(float val)
{
	scene_state_.mix_roughness = val;
}
void ViewerWidget::setMetallicMix(float val)
{
	scene_state_.mix_metallic = val;
}
void ViewerWidget::setNormalMix(float val)
{
	scene_state_.mix_normal = val;
}
void ViewerWidget::setHeightMix(float val)
{
	scene_state_.mix_height = val;
}
void ViewerWidget::setInterpolationMix(float val)
{
	scene_state_.global_interpolation = val;
	if (colorChecked)
	{
		scene_state_.mix_color = val;
		emit colorHasChanged(val);
	}
	if (metallicChecked)
	{
		scene_state_.mix_metallic = val;
		emit metallicHasChanged(val);
	}
	if (roughnessChecked)
	{
		scene_state_.mix_roughness = val;
		emit roughnessHasChanged(val);
	}
	if (normalChecked)
	{
		scene_state_.mix_normal = val;
		emit normalHasChanged(val);
	}
	if (heightChecked)
	{
		scene_state_.mix_height = val;
		emit heightHasChanged(val);
	}
	emit reloadInfo();
}

void ViewerWidget::setColorStatus(bool status)
{
	colorChecked = status;
}
void ViewerWidget::setMetallicStatus(bool status)
{
	metallicChecked = status;
}
void ViewerWidget::setRoughnessStatus(bool status)
{
	roughnessChecked = status;
}
void ViewerWidget::setNormalStatus(bool status)
{
	normalChecked = status;
}
void ViewerWidget::setHeightStatus(bool status)
{
	heightChecked = status;
}

void ViewerWidget::setHeightFactor(float val)
{
	scene_state_.height_factor = val;
}
void ViewerWidget::setTesselationLevel(int val)
{
	scene_state_.tess_level = val;
}
void ViewerWidget::setzNear(float val)
{
	scene_state_.zNear = val;
}

void ViewerWidget::setzFar(float val)
{
	scene_state_.zFar = val;
}

void ViewerWidget::setSSAORadius(float val) {
	scene_state_.ssao_radius = val;
}
