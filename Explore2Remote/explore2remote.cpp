#include "explore2remote.h"
#include <QGraphicsPixmapItem>
#include <algorithm>

using namespace explore2;
using namespace remote;
using namespace remote_utils;

static const size_t MAX_FPS_SAMPLES = 200;

namespace texture {;

void update_texture(QTreeWidgetItem* item, const TextureData& texture)
{
	item->setText(0, QString("%1").arg(texture.handle));
	item->setData(0, Qt::UserRole, texture.handle);
	item->setData(0, Qt::UserRole + 1, texture.id);

	item->setText(1, texture.name.c_str());

	item->setText(2, QString("%1").arg(texture.type));
	item->setData(2, Qt::UserRole, texture.type);

	item->setText(3, QString("%1").arg(texture.width));
	item->setData(3, Qt::UserRole, texture.width);

	item->setText(4, QString("%1").arg(texture.height));
	item->setData(4, Qt::UserRole, texture.height);

	item->setText(5, QString("%1").arg(texture.depth));
	item->setData(5, Qt::UserRole, texture.depth);

	item->setText(6, QString("%1").arg(texture.value_type));
	item->setData(6, Qt::UserRole, texture.value_type);
}

unsigned int get_texture_id(QTreeWidgetItem* item)
{
	return item->data(0, Qt::UserRole + 1).toUInt();
}

unsigned int get_texture_type(QTreeWidgetItem* item)
{
	return item->data(2, Qt::UserRole).toUInt();
}

unsigned int get_texture_width(QTreeWidgetItem* item)
{
	return item->data(3, Qt::UserRole).toUInt();
}

unsigned int get_texture_height(QTreeWidgetItem* item)
{
	return item->data(4, Qt::UserRole).toUInt();
}

unsigned int get_texture_depth(QTreeWidgetItem* item)
{
	return item->data(5, Qt::UserRole).toUInt();
}

unsigned int get_texture_value_type(QTreeWidgetItem* item)
{
	return item->data(6, Qt::UserRole).toUInt();
}

}

Explore2Remote::Explore2Remote(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags),
	_fpsPlot(new QwtPlotCurve("FPS")),
	_frameTimePlot(new QwtPlotCurve("Frame MS")),
	_totalFrameTime(0),
	_textureScene(new QGraphicsScene(-2000, -2000, 4000, 4000))
{
	ui.setupUi(this);

	ui.textureGraphicsView->setScene(_textureScene);

	//ui.qwtPlotFPS->enableAxis(0, false);
	//ui.qwtPlotFPS->enableAxis(1);
	ui.qwtPlotFPS->setAxisTitle(0, "fps");
	//ui.qwtPlotFPS->setAxisAutoScale(1);
	//ui.qwtPlotFPS->enableAxis(2);
	ui.qwtPlotFPS->setAxisTitle(2, "time");

	//ui.qwtPlotFrameMS->enableAxis(0, false);
	//ui.qwtPlotFrameMS->enableAxis(1);
	ui.qwtPlotFrameMS->setAxisTitle(0, "ms");
	//ui.qwtPlotFrameMS->setAxisAutoScale(1);
	//ui.qwtPlotFrameMS->enableAxis(2);
	ui.qwtPlotFrameMS->setAxisTitle(2, "time");

	_frameTimePlot->setStyle(QwtPlotCurve::Steps);
	//_frameTimePlot->setAxis(2, 1);
	_frameTimePlot->attach(ui.qwtPlotFrameMS);
	//_fpsPlot->setAxis(2, 1);
	_fpsPlot->attach(ui.qwtPlotFPS);

	RemoteDebug::create_instance(RemoteDebug::Debugger_t());

	remote_utils::register_event<std::string>(std::bind(&Explore2Remote::string_event, this, std::placeholders::_1));
	remote_utils::register_event<remote_utils::FrameTimeEvent>(std::bind(&Explore2Remote::fps_event, this, std::placeholders::_1));
	remote_utils::register_event<remote_utils::ChunkLODStatsEvent>(std::bind(&Explore2Remote::chunk_stats_event, this, std::placeholders::_1));
	remote_utils::register_event<remote_utils::TextureList>(std::bind(&Explore2Remote::texture_list_event, this, std::placeholders::_1));
	remote_utils::register_event<remote_utils::TexturePixels>(std::bind(&Explore2Remote::texture_pixels_event, this, std::placeholders::_1));
	remote_utils::register_event<remote_utils::TextureRequest>([](remote_utils::TextureRequest&){});
	remote_utils::register_event<remote_utils::Command>([](remote_utils::Command&){});

	_eventTimer.setInterval(10);
	_eventTimer.setSingleShot(false);
	
	QObject::connect(&_eventTimer, SIGNAL(timeout()), this, SLOT(handle_events()));

	QObject::connect(ui.updateButton, &QPushButton::clicked, [&]() {
		RemoteDebug::get_instance()->send_event(remote_utils::TextureRequest());
	});	
	
	QObject::connect(ui.textureTreeWidget, &QTreeWidget::itemDoubleClicked, [&](QTreeWidgetItem* item, int column) {
		RemoteDebug::get_instance()->send_event(remote_utils::TextureRequest(texture::get_texture_id(item)));
	});

	QObject::connect(ui.textureLayerSlider, &QSlider::valueChanged, [&](int val) {
		set_texture_level(val);
	});

	QObject::connect(ui.textureScaleModeGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [&](int) {
		set_texture_level(ui.textureLayerSlider->value());
	});

	QObject::connect(ui.actionReload_Effects, &QAction::triggered, [&](bool) {
		RemoteDebug::get_instance()->send_event(remote_utils::Command(ui.actionReload_Effects->text().toLocal8Bit().data()));
	});

	_eventTimer.start();
}

Explore2Remote::~Explore2Remote()
{

}

void Explore2Remote::handle_events()
{
	while(RemoteDebug::get_instance()->process()) {}
}

void Explore2Remote::string_event(std::string& str)
{
	ui.listWidget->addItem(str.c_str());
}

void Explore2Remote::fps_event(remote_utils::FrameTimeEvent& frameTimes)
{
	double msPerFrame = frameTimes.frameTime / (float)frameTimes.frames;
	double fps = (float)frameTimes.frames / (frameTimes.frameTime / 1000.0f);

	_totalFrameTime += msPerFrame;
	_totalFrameTimeData.push_back((double)_totalFrameTime / 1000.0);
	_fpsData.push_back(fps);
	_frameTimeData.push_back(msPerFrame);
	if(_totalFrameTimeData.size() > ui.graphTimeWindow->value())
	{
		size_t diff = _totalFrameTimeData.size() - ui.graphTimeWindow->value();
		_totalFrameTimeData = std::vector<double>(_totalFrameTimeData.begin()+diff, _totalFrameTimeData.end());
		_fpsData = std::vector<double>(_fpsData.begin()+diff, _fpsData.end());
		_frameTimeData = std::vector<double>(_frameTimeData.begin()+1, _frameTimeData.end());
	}
	_fpsPlot->setSamples(&_totalFrameTimeData[0], &_fpsData[0], static_cast<int>(_totalFrameTimeData.size()));
	_frameTimePlot->setSamples(&_totalFrameTimeData[0], &_frameTimeData[0], static_cast<int>(_totalFrameTimeData.size()));
	ui.qwtPlotFPS->replot();
	ui.qwtPlotFrameMS->replot();
}

void Explore2Remote::chunk_stats_event(explore2::remote_utils::ChunkLODStatsEvent& chunkStats)
{
	ui.chunkState0Empty->setText(QString("%1").arg(chunkStats.states[0]));
	ui.chunkState1Prepared->setText(QString("%1").arg(chunkStats.states[1]));
	ui.chunkState2Shown->setText(QString("%1").arg(chunkStats.states[2]));
	ui.chunkState3ChildrenPrepared->setText(QString("%1").arg(chunkStats.states[3]));
	ui.chunkTotalShown->setText(QString("%1").arg(
		chunkStats.states[2] + chunkStats.states[3]));
	ui.chunkState4ChildrenShown->setText(QString("%1").arg(chunkStats.states[4]));
	ui.chunkTreeDepth->setText(QString("%1").arg(chunkStats.depth));
	ui.chunkOneWeightCount->setText(QString("%1").arg(chunkStats.oneWeightsCount));
	ui.chunkZeroWeightCount->setText(QString("%1").arg(chunkStats.zeroWeightsCount));
}

void Explore2Remote::texture_list_event(explore2::remote_utils::TextureList& textureList)
{
	ui.textureTreeWidget->clear();
	for(auto itr = textureList.textures.begin(); itr != textureList.textures.end(); ++itr)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem();
		texture::update_texture(item, *itr);
		ui.textureTreeWidget->addTopLevelItem(item);
	}
}

void Explore2Remote::texture_pixels_event(explore2::remote_utils::TexturePixels& texturePixels)
{
	_activeTexture = texturePixels;
	ui.textureLayerSlider->setRange(0, _activeTexture.depth - 1);
	set_texture_level(0);
}

unsigned char scale_pixel_value(float val, float low, float high)
{
	return static_cast<unsigned char>(std::min<float>(255.0f, std::max<float>(0, (val - low) / (high - low)) * 255.0f));
}

void Explore2Remote::set_texture_level( int level )
{
	_textureScene->clear();
	ui.layerLabel->setText(QString("%1").arg(level));

	if(level >= _activeTexture.depth)
		return ;

	float low, high;
	if(ui.scale0_255Button->isChecked())
	{
		low = 0; high = 255;
	}
	else if(ui.scale0_1Button->isChecked())
	{
		low = 0; high = 1;
	}
	else if(ui.scaleCustomButton->isChecked())
	{
		low = ui.customLowerDoubleSpinBox->value();
		high = ui.customUpperDoubleSpinBox->value();
	}
	else if(ui.scaleAutoButton->isChecked())
	{
		auto minMaxPair = std::minmax_element(_activeTexture.pixels.begin(), _activeTexture.pixels.end());
		low = *minMaxPair.first;
		high = *minMaxPair.second;
	}
	std::vector<unsigned char> bits;
	int pixelsPerLevel = _activeTexture.width * _activeTexture.height;
	//int offs = _activeTexture.width * _activeTexture.height * 4 * level;
	for(size_t idx = pixelsPerLevel * level; idx < pixelsPerLevel * (level + 1); ++idx)
	{
		bits.push_back(scale_pixel_value(_activeTexture.pixels[idx * 4 + 0], low, high));
		bits.push_back(scale_pixel_value(_activeTexture.pixels[idx * 4 + 1], low, high));
		bits.push_back(scale_pixel_value(_activeTexture.pixels[idx * 4 + 2], low, high));
		bits.push_back(_activeTexture.pixels[idx * 4 + 3] * 255);//scale_pixel_value(_activeTexture.pixels[idx * 4 + 3]));
	}

	QImage image(&bits[0], _activeTexture.width, _activeTexture.height, QImage::Format_RGBA8888);

	//QPixmap* texPixMap = new QPixmap(_activeTexture.width, _activeTexture.height);

	QGraphicsPixmapItem* item = _textureScene->addPixmap(QPixmap::fromImage(image));
	item->setPos((/*_textureScene->width()*/ - image.width()) / 2, (/*_textureScene->height()*/ - image.height()) / 2);
}

