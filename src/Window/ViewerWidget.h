#ifndef H_VIEWER_WIDGET
#define H_VIEWER_WIDGET

#include <memory>

#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QGridLayout>

#include "ControlPanel.h"
#include "Rendering/SceneState.h"

class Viewer;
struct Model;

class ViewerWidget : public QOpenGLWidget
{
	Q_OBJECT;
public:
	explicit ViewerWidget(QWidget *parent = 0);
	~ViewerWidget();

	virtual QSize sizeHint() const { return QSize(scene_state_.width, scene_state_.height); }
	SceneState & getSceneState() { return scene_state_; }
	const std::string& getWindowTitle();

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;

private slots:
	inline void triggerUpdate() { update(); }

	void reloadShaders(const QString &);

public slots:
	void loadMaterial1(QString filename);
	void loadMaterial2(QString filename);
	void loadWarpGrid(QString filename);
	
	void setColorMix(float);
	void setRoughnessMix(float);
	void setMetallicMix(float);
	void setNormalMix(float);
	void setHeightMix(float);
	void setInterpolationMix(float);

	void setMeshToRender(int);

	void setColorStatus(bool);
	void setMetallicStatus(bool);
	void setRoughnessStatus(bool);
	void setNormalStatus(bool);
	void setHeightStatus(bool);

	void setHeightFactor(float);
	void setTesselationLevel(int);
	void setzNear(float);
	void setzFar(float);
	void setSSAORadius(float);

signals:
	void modelChanged(Model*);
	void reloadInfo();

	void colorHasChanged(float);
  	void metallicHasChanged(float);
  	void roughnessHasChanged(float);
 	void normalHasChanged(float);
  	void heightHasChanged(float);
  	void globalInterpolationChanged(float);
	void tessLevelChanged(float);

	void resizeToMinimum(bool);

private:

	void updateProjectionMat();
	bool ctrlIsPressed = false;
	bool altIsPressed = false;
	bool tabIsPressed = false;
	bool lmbIsPressed = false;
	bool colorChecked = true;
	bool metallicChecked = true;
	bool roughnessChecked = true;
	bool normalChecked = true;
	bool heightChecked = true;

	Viewer		* m_viewerCore;
	QGridLayout * main_layout_;
	InfoViewer	* m_infoviewer;

	QTimer m_renderTimer;
	QTimer m_titleTimer;

	std::unique_ptr<QFileSystemWatcher> m_watcher;

	SceneState scene_state_;

	bool display_infos = false;
};

#endif // H_VIEWER_WIDGET
