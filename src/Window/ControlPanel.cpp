#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>

#include "ControlPanel.h"
#include "ViewerWidget.h"

ControlPanel::ControlPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *main_layout = new QVBoxLayout;
    setLayout(main_layout);
    main_layout->setAlignment(Qt::AlignTop);

	QFont font;

	// MESH SETUP
	{
		QGroupBox* groupBox = new QGroupBox(tr("Mesh"));
		radioPlane = new QRadioButton(tr("Plane"));
		radioSphere = new QRadioButton(tr("Sphere"));

		radioSphere->setChecked(true);

		connect(radioPlane, SIGNAL(clicked()), this, SLOT(PlaneSelected()));
		connect(radioSphere, SIGNAL(clicked()), this, SLOT(SphereSelected()));

		QVBoxLayout* vbox = new QVBoxLayout;
		vbox->addWidget(radioPlane);
		vbox->addWidget(radioSphere);
		groupBox->setLayout(vbox);

		main_layout->addWidget(groupBox);
	}

	////////////////////////////
	/// Technical Parameters ///
	////////////////////////////
	{
		QLabel* heightCtrl_text = new QLabel("Technical Parameters", this);
		heightCtrl_text->setFont(font);
		main_layout->addWidget(heightCtrl_text);

		QLabel* heightfactor_text = new QLabel("Height factor", this);
		main_layout->addWidget(heightfactor_text);

		QSlider* heightfactor_slider = new QSlider(Qt::Horizontal, this);
		heightfactor_slider->setMinimum(0);
		heightfactor_slider->setMaximum(10);
		heightfactor_slider->setValue(1);
		connect(heightfactor_slider, SIGNAL(valueChanged(int)), this, SLOT(setHeightFactorValue(int)));
		main_layout->addWidget(heightfactor_slider);

		///////////////////
		/// TESSELATION ///
		///////////////////

		QLabel* tesslevel_text = new QLabel("Tesselation level", this);
		main_layout->addWidget(tesslevel_text);

		tesslevel_slider = new QSlider(Qt::Horizontal, this);
		tesslevel_slider->setMinimum(1);
		tesslevel_slider->setMaximum(tesselation_slider_maximum);
		tesslevel_slider->setValue(48);
		connect(tesslevel_slider, SIGNAL(valueChanged(int)), this, SLOT(setTesselationLevel(int)));
		main_layout->addWidget(tesslevel_slider);
	}

	// SSAO Controls
	{
		QLabel* SSAOctrl_text = new QLabel("SSAO radius", this);
		SSAOctrl_text->setFont(font);
		main_layout->addWidget(SSAOctrl_text);

		QSlider* radius_slider = new QSlider(Qt::Horizontal, this);
		radius_slider->setMinimum(0);
		radius_slider->setMaximum(unit_slider_maximum);
		radius_slider->setValue(0.25f * unit_slider_maximum);
		connect(radius_slider, SIGNAL(valueChanged(int)), this, SLOT(setSSAORadius(int)));
		main_layout->addWidget(radius_slider);
	}

	//////////////////////////////
	/// Interpolation controls ///
	//////////////////////////////
	{
		QLabel *interpCtrl_text = new QLabel("Interpolation Control", this);
		font = interpCtrl_text->font();
		font.setBold(true);
		interpCtrl_text->setFont(font);
		main_layout->addWidget(interpCtrl_text);

		/////////////////////
		///// 1 : COLOR	/////
		/////////////////////

		QHBoxLayout *color_layout = new QHBoxLayout;
		color_layout->setAlignment(Qt::AlignLeft);
		
		QCheckBox *color_cbox = new QCheckBox();
		color_cbox->setChecked(true);
		connect(color_cbox, SIGNAL(stateChanged(int)), this, SLOT(colorChecked(int)));
		color_layout->addWidget(color_cbox);

		QLabel *color_text = new QLabel("1 : Color", this);
		color_layout->addWidget(color_text);

		main_layout->addLayout(color_layout);

		interpolation_slider_color = new QSlider(Qt::Horizontal, this);
		interpolation_slider_color->setMinimum(0);
		interpolation_slider_color->setMaximum(unit_slider_maximum);
		interpolation_slider_color->setValue(0);
		connect(interpolation_slider_color, SIGNAL(valueChanged(int)), this, SLOT(setInterpolationColor(int)));
		main_layout->addWidget(interpolation_slider_color);

		/////////////////////
		//// 2 : HEIGHT /////
		/////////////////////

		QHBoxLayout *height_layout = new QHBoxLayout;
		height_layout->setAlignment(Qt::AlignLeft);

		QCheckBox *height_cbox = new QCheckBox();
		height_cbox->setChecked(true);
		connect(height_cbox, SIGNAL(stateChanged(int)), this, SLOT(heightChecked(int)));
		height_layout->addWidget(height_cbox);

		QLabel* height_text = new QLabel("2 : Height", this);
		height_layout->addWidget(height_text);

		main_layout->addLayout(height_layout);

		interpolation_slider_height = new QSlider(Qt::Horizontal, this);
		interpolation_slider_height->setMinimum(0);
		interpolation_slider_height->setMaximum(unit_slider_maximum);
		interpolation_slider_height->setValue(0);
		connect(interpolation_slider_height, SIGNAL(valueChanged(int)), this, SLOT(setInterpolationHeight(int)));
		main_layout->addWidget(interpolation_slider_height);

		/////////////////////
		///// 3 : METAL /////
		/////////////////////

		QHBoxLayout *metal_layout = new QHBoxLayout;
		metal_layout->setAlignment(Qt::AlignLeft);

		QCheckBox *metal_cbox = new QCheckBox();
		metal_cbox->setChecked(true);
		connect(metal_cbox, SIGNAL(stateChanged(int)), this, SLOT(metallicChecked(int)));
		metal_layout->addWidget(metal_cbox);

		QLabel *metallic_text = new QLabel("3 : Metallic", this);
		metal_layout->addWidget(metallic_text);

		main_layout->addLayout(metal_layout);

		interpolation_slider_metallic = new QSlider(Qt::Horizontal, this);
		interpolation_slider_metallic->setMinimum(0);
		interpolation_slider_metallic->setMaximum(unit_slider_maximum);
		interpolation_slider_metallic->setValue(0);
		connect(interpolation_slider_metallic, SIGNAL(valueChanged(int)), this, SLOT(setInterpolationMetallic(int)));
		main_layout->addWidget(interpolation_slider_metallic);

		/////////////////////
		//// 4 : NORMAL /////
		/////////////////////

		QHBoxLayout *normal_layout = new QHBoxLayout;
		normal_layout->setAlignment(Qt::AlignLeft);

		QCheckBox *normal_cbox = new QCheckBox();
		normal_cbox->setChecked(true);
		connect(normal_cbox, SIGNAL(stateChanged(int)), this, SLOT(normalChecked(int)));
		normal_layout->addWidget(normal_cbox);

		QLabel *normal_text = new QLabel("4 : Normal", this);
		normal_layout->addWidget(normal_text);

		main_layout->addLayout(normal_layout);

		interpolation_slider_normal = new QSlider(Qt::Horizontal, this);
		interpolation_slider_normal->setMinimum(0);
		interpolation_slider_normal->setMaximum(unit_slider_maximum);
		interpolation_slider_normal->setValue(0);
		connect(interpolation_slider_normal, SIGNAL(valueChanged(int)), this, SLOT(setInterpolationNormal(int)));
		main_layout->addWidget(interpolation_slider_normal);

		/////////////////////
		/// 5 : ROUGHNESS ///
		/////////////////////

		QHBoxLayout *roughness_layout = new QHBoxLayout;
		roughness_layout->setAlignment(Qt::AlignLeft);

		QCheckBox *roughness_cbox = new QCheckBox();
		roughness_cbox->setChecked(true);
		connect(roughness_cbox, SIGNAL(stateChanged(int)), this, SLOT(roughnessChecked(int)));
		roughness_layout->addWidget(roughness_cbox);

		QLabel *roughness_text = new QLabel("5 : Roughness", this);
		roughness_layout->addWidget(roughness_text);

		main_layout->addLayout(roughness_layout);

		interpolation_slider_roughness = new QSlider(Qt::Horizontal, this);
		interpolation_slider_roughness->setMinimum(0);
		interpolation_slider_roughness->setMaximum(unit_slider_maximum);
		interpolation_slider_roughness->setValue(0);
		connect(interpolation_slider_roughness, SIGNAL(valueChanged(int)), this, SLOT(setInterpolationRoughness(int)));

		main_layout->addWidget(interpolation_slider_roughness);

		////////////////////
		/// GLOBAL VALUE ///
		////////////////////

		QLabel* interp_text = new QLabel("Global Interpolation value", this);
		main_layout->addWidget(interp_text);

		global_interpolation_slider = new QSlider(Qt::Horizontal, this);
		global_interpolation_slider->setMinimum(0);
		global_interpolation_slider->setMaximum(unit_slider_maximum);
		global_interpolation_slider->setValue(0);
		connect(global_interpolation_slider, SIGNAL(valueChanged(int)), this, SLOT(setGlobalInterpolation(int)));

		main_layout->addWidget(global_interpolation_slider);
	}
}

void ControlPanel::setTesselationLevel(int s)
{
	emit TessLevelChanged(s);
}
void ControlPanel::setHeightFactorValue(int s)
{
	emit HeightFactorChanged(s / 10.0f); //ranges from 0 to 1
}
void ControlPanel::setSSAORadius(int s)
{
    emit radiusChanged(0.2f * s / unit_slider_maximum); //ranges from 0 to 1
}

void ControlPanel::PlaneSelected()
{
	emit meshIsSelected(0);
}
void ControlPanel::SphereSelected()
{
	emit meshIsSelected(1);
}

void ControlPanel::setInterpolationColor(int s)
{
    emit ColorChanged(s / unit_slider_maximum);
}
void ControlPanel::setInterpolationRoughness(int s)
{
	emit RoughnessChanged(s / unit_slider_maximum);
}
void ControlPanel::setInterpolationMetallic(int s)
{
	emit MetallicChanged(s / unit_slider_maximum);
}
void ControlPanel::setInterpolationNormal(int s)
{
	emit NormalChanged(s / unit_slider_maximum);
}
void ControlPanel::setInterpolationHeight(int s)
{
	emit HeightChanged(s / unit_slider_maximum);
}

void ControlPanel::setGlobalInterpolation(int s)
{
	emit interpolationChanged(s / unit_slider_maximum);
}

void ControlPanel::colorChecked(int is_checked)
{
	emit setColorStatus(bool(is_checked));
}
void ControlPanel::metallicChecked(int is_checked)
{
	emit setMetallicStatus(bool(is_checked));
}
void ControlPanel::roughnessChecked(int is_checked)
{
	emit setRoughnessStatus(bool(is_checked));
}
void ControlPanel::normalChecked(int is_checked)
{
	emit setNormalStatus(bool(is_checked));
}
void ControlPanel::heightChecked(int is_checked)
{
	emit setHeightStatus(bool(is_checked));
}

void ControlPanel::colorHasChanged(float newVal) {
	interpolation_slider_color->setValue(newVal * unit_slider_maximum);
}
void ControlPanel::metallicHasChanged(float newVal) {
	interpolation_slider_metallic->setValue(newVal * unit_slider_maximum);
}
void ControlPanel::roughnessHasChanged(float newVal) {
	interpolation_slider_roughness->setValue(newVal * unit_slider_maximum);
}
void ControlPanel::normalHasChanged(float newVal) {
	interpolation_slider_normal->setValue(newVal * unit_slider_maximum);
}
void ControlPanel::heightHasChanged(float newVal) {
	interpolation_slider_height->setValue(newVal * unit_slider_maximum);
}
void ControlPanel::globalInterpolationChanged(float newVal) {
	global_interpolation_slider->setValue(newVal / unit_slider_maximum);
}

void ControlPanel::tessLevelChanged(float newVal) {
	tesslevel_slider->setValue(newVal);
}