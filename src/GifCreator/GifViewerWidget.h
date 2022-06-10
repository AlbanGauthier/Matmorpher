#ifndef H_VIEWER_WIDGET
#define H_VIEWER_WIDGET

#include "Rendering/SceneState.h"

#include <memory>

#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QGridLayout>

class Viewer;
struct Model;

/**
 * Proxy class binding the core viewer from Viewer.h to Qt's widget ecosystem.
 * It is named 'ViewerWidget' because it has kind of the same role toward
 * 'Viewer' than 'ListWidget' has toward 'ListView'.
 * It overrides from QOpenGLWidget to be able to override methods like paintGL
 * and replace them with calls to the underlying Viewer object (raw OpenGL).
 */
class ViewerWidget : public QOpenGLWidget
{
	Q_OBJECT;
public:
	explicit ViewerWidget(QWidget *parent = 0);
	~ViewerWidget();

	virtual QSize sizeHint() const { return QSize(scene_state_.width, scene_state_.height); }
	SceneState & getSceneState() { return scene_state_; }

	void setOutputType(int i)
	{
		scene_state_.gif_output = static_cast<GifOutputType>(i);
	}

	int getWidth() { return int(scene_state_.width); }
	int getHeight() { return int(scene_state_.height); }

	void setRenderSize(int renderSize) {

		switch (scene_state_.gif_output)
		{
		case GifOutputType::Render:
		{
			scene_state_.width = 2 * renderSize;
			scene_state_.height = renderSize;
			break;
		}
		case GifOutputType::TextureDesign:
		{
			scene_state_.width = renderSize;
			scene_state_.height = renderSize;
			break;
		}
		default:
			break;
		}

		scene_state_.viewport = { 0, 0, int(scene_state_.width), int(scene_state_.height) }; //for screenshots
	}

	void setMaterialsAndWarpNames(std::string mat1, std::string mat2, std::string warpgrid, std::string warpgrid_td);

protected:
	// Overrides of QOpenGLWidget that we forward to the Viewer instance
	// See individual method's documentation for details. Here is a summary:

	/**
	 * Called after the OpenGL context has been setup (internally by Qt).
	 * Here we can allocate and initialize the Viewer.
	 */
	void initializeGL() override;

	/**
	 * Called when the context is resized (meaning typically that the widget
	 * size changed).
	 * We update glViewport, and if you use some kind of camera you will need
	 * to update its projection matrix here, at least for the aspect ratio.
	 */
	void resizeGL(int w, int h) override;

	/**
	 * Called at each frame, when the widget's context is ready to be pointed
	 * on by some raw OpenGL calls.
	 */
	void paintGL() override;

	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;

private slots:
	/**
	 * Slot triggering the update() method of the widget
	 * The update method will itself call paintGL at some appropriate point
	 * NB: Widget::update() is not a slot so we cannot use it where we use triggerUpdate().
	 */
	inline void triggerUpdate() { update(); }

	void reloadShaders(const QString &);

private:

	void updateProjectionMat();

	/**
	 * The core rendering behavior is actually handled by this Viewer instance.
	 * It is a dynamically allocated pointer so that Viewer's constructor can
	 * assume that OpenGL context has been loaded first.
	 */
	Viewer		* m_viewerCore;
	QGridLayout * main_layout_;

	/**
	 * Timers used for scheduling frame redraw the widget and title.
	 */
	QTimer m_renderTimer;

	/**
	 * Watcher to monitor changes in the currently loaded shader files.
	 */
	std::unique_ptr<QFileSystemWatcher> m_watcher;

	SceneState scene_state_;
};

#endif // H_VIEWER_WIDGET
