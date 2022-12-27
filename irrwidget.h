#ifndef IRRWIDGET_H
#define IRRWIDGET_H

#include <QObject>
#include <QTimer>
#include <QAtomicInt>

#include <irrlicht.h>
#include "irrnoise.h"

class IrrWidget : public QObject
{
    Q_OBJECT
public:
    explicit IrrWidget(QObject *parent = nullptr);
    ~IrrWidget();

    // Irrlicht
private:
    irr::IrrlichtDevice *device = 0;

    irr::video::IVideoDriver *driver = 0;
    irr::scene::ISceneManager *smgr = 0;

    QTimer update_timer;
    QAtomicInt updating = 0;

    // Default scene setup
    void init_default_scene();
    irr::scene::ISceneNode *default_scene_node = 0;
    irr::scene::ICameraSceneNode *default_camera = 0;
    irr::scene::ILightSceneNode *default_light = 0;

    // main functions
public slots:
    void update();

    // Noise mesh
private:
    IrrNoise *irrnoise;
};

#endif // IRRWIDGET_H
