#pragma once

#include <sstream>
#include <iostream>

#include <QWidget>
#include <QElapsedTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QSlider>
#include <QRadioButton>

#include "MainWindow.h"
#include "Rendering/SceneState.h"

constexpr float unit_slider_maximum = 10000;
constexpr float tesselation_slider_maximum = 64;

class ControlPanel : public QWidget
{
	Q_OBJECT
public:
	ControlPanel(QWidget* parent = 0);

signals:
	void ColorChanged(float);
	void RoughnessChanged(float);
	void MetallicChanged(float);
	void NormalChanged(float);
	void HeightChanged(float);
	void interpolationChanged(float);

	void meshIsSelected(int);

	void setColorStatus(bool);
	void setMetallicStatus(bool);
	void setRoughnessStatus(bool);
	void setNormalStatus(bool);
	void setHeightStatus(bool);

	void HeightFactorChanged(float);
	void TessLevelChanged(int);
	void radiusChanged(float);

public slots:
	void colorHasChanged(float);
	void metallicHasChanged(float);
	void roughnessHasChanged(float);
	void normalHasChanged(float);
	void heightHasChanged(float);
	void globalInterpolationChanged(float);
	void tessLevelChanged(float);

private slots:
	void setInterpolationColor(int);
	void setInterpolationRoughness(int);
	void setInterpolationMetallic(int);
	void setInterpolationNormal(int);
	void setInterpolationHeight(int);
	void setGlobalInterpolation(int);

	void colorChecked(int);
	void metallicChecked(int);
	void roughnessChecked(int);
	void normalChecked(int);
	void heightChecked(int);

	void setHeightFactorValue(int);
	void setTesselationLevel(int);
	void setSSAORadius(int);

	void PlaneSelected();
	void SphereSelected();

private:

	QSlider* interpolation_slider_color;
	QSlider* interpolation_slider_metallic;
	QSlider* interpolation_slider_roughness;
	QSlider* interpolation_slider_height;
	QSlider* interpolation_slider_normal;
	QSlider* global_interpolation_slider;
	QSlider* tesslevel_slider;

	QRadioButton* radioPlane;
	QRadioButton* radioCube;
	QRadioButton* radioSphere;
	QRadioButton* radioOBJ;

	QRadioButton* radioWarpLinear;
	QRadioButton* radioWarpTexDesign;

};


///////////////////////////////////////////////
/// InfoViewer (materials, warp_map, ycbcr) ///
///////////////////////////////////////////////

class InfoViewer : public QWidget
{
	Q_OBJECT
public:

	InfoViewer(QWidget* parent, SceneState& state)
		: QWidget(parent), scene_state_(state)
	{
		QVBoxLayout* main_layout = new QVBoxLayout(this);
		setLayout(main_layout);
		main_layout->setAlignment(Qt::AlignTop);

		label_materials_ = new QLabel;
		main_layout->addWidget(label_materials_);
		label_warp_ = new QLabel;
		main_layout->addWidget(label_warp_);
		label_ycbcr_ = new QLabel;
		main_layout->addWidget(label_ycbcr_);
	}

	~InfoViewer() {}

	void update() {
		label_materials_	->setText(QString::fromStdString(scene_state_.mat1_name + " / " + scene_state_.mat2_name));
		label_warp_			->setText(QString::fromStdString(scene_state_.warp_grid));
		label_ycbcr_		->setText(QString::fromStdString("YCbCr: ") + boolToString(scene_state_.ycbcr));
	};

	size_t getTriangleNb() { return scene_state_.triangle_count; }

	QString boolToString(bool b)
	{
		QString result;
		if (b)
			result = "True";
		else
			result = "False";
		return result;
	}

	const std::string& getTitle()
	{
		double ms_time = double(scene_state_.timequery) / 1e6;
		int fps = int(1000.0f / ms_time);
		windowTitle = "MatMorpher - "
			+ formatFloatingPoint(ms_time) + " ms - "
			+ std::to_string(fps) + " FPS - "
			+ " " + formatInteger(getTriangleNb()) + " triangles";
		return windowTitle;
	}

	struct SpaceFormater : std::numpunct<char>
	{
		char do_thousands_sep() const { return ' '; }    // separate with dots
		std::string do_grouping() const { return "\3"; } // groups of 3 digits
	};

	template <typename T>
	std::string formatInteger(T number)
	{
		std::ostringstream ostr;
		ostr.imbue(std::locale(ostr.getloc(), new SpaceFormater));
		ostr << number;
		return ostr.str();
	}

	template<typename T>
	std::string formatFloatingPoint(T number)
	{
		std::string num_text = std::to_string(number);
		return num_text.substr(0, num_text.find(".") + 3);
	}

private:

	size_t tri_ = 0;

	SceneState& scene_state_;

	std::string windowTitle;

	QLabel* label_materials_;
	QLabel* label_warp_;
	QLabel* label_ycbcr_;

public slots:
	void updateInfo() { update(); };

};
