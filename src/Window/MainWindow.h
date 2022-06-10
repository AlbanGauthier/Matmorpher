#pragma once

#include <QMainWindow>

class ViewerWidget;
class ControlPanel;
class InfoViewer;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = 0);
	void openMaterials(int argc, char* argv[]);

public slots:
	void openMaterial1();
	void openMaterial2();
	void openWarpGrid();
	void resizeToMinimum(bool);
	void renameWindowTitle();

protected:
	void keyPressEvent(QKeyEvent* event) override;

private: 
	QString current_path_;
	QMenuBar* menu_bar_;
	ControlPanel* controls_;
	ViewerWidget* viewer_widget_;
	
};