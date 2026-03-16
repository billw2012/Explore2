#ifndef EXPLORE2REMOTE_H
#define EXPLORE2REMOTE_H

#include <QtWidgets/QMainWindow>
#include <QTimer>

#include <qwt_plot_curve.h>


#include <vector>

#include "Explore2Game/RemoteDebugInterface.h"

#include "ui_explore2remote.h"

class Explore2Remote : public QMainWindow
{
	Q_OBJECT

public:
	Explore2Remote(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~Explore2Remote();

private:
	void string_event(std::string& str);
	void fps_event(explore2::remote_utils::FrameTimeEvent& fps);
	void chunk_stats_event(explore2::remote_utils::ChunkLODStatsEvent& chunkStats);
	void texture_list_event(explore2::remote_utils::TextureList& textureList);
	void texture_pixels_event(explore2::remote_utils::TexturePixels& texturePixels);
	
	void set_texture_level(int level);

private slots:
	void handle_events();

private:
	Ui::Explore2RemoteClass ui;

	std::shared_ptr<QwtPlotCurve> _fpsPlot, _frameTimePlot;
	std::vector<double> _totalFrameTimeData, _fpsData, _frameTimeData;
	size_t _totalFrameTime;

	explore2::remote_utils::TexturePixels _activeTexture;

	QGraphicsScene* _textureScene;

	QTimer _eventTimer;
};

#endif // EXPLORE2REMOTE_H
