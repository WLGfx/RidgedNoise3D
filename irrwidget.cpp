#include <QWidget>

#include "irrwidget.h"

using namespace irr;

IrrWidget::IrrWidget(QObject *parent)
    : QObject{parent}
{
    // initialise the Irrlicht widget in the UI

    SIrrlichtCreationParameters params;

    QWidget *parentWidget = (QWidget*)parent;
    params.WindowId = (void*)parentWidget->winId();
    params.WindowSize.set(parentWidget->size().width(), parentWidget->size().height());
    params.DriverType = video::EDT_OPENGL;
    params.HandleSRGB = true;
    params.Vsync = true;
    params.AntiAlias = true;

    device = createDeviceEx(params);
    driver = device->getVideoDriver();
    smgr = device->getSceneManager();

    device->setResizable(true);
    smgr->clear();

    init_default_scene();

    update_timer.setInterval(1);
    connect(&update_timer, SIGNAL(timeout()),
            this, SLOT(update()));
    update_timer.start();

    irrnoise = new IrrNoise(parent, device);
}

IrrWidget::~IrrWidget() {
    updating = 1; // pause renderer
    update_timer.stop();

    delete irrnoise;

    if (device) {
        device->drop();
        device = 0;
    }
}

void IrrWidget::init_default_scene() {
    if (default_scene_node) {
        default_scene_node->remove();
        default_scene_node = 0;
    }

    default_scene_node = smgr->addEmptySceneNode();

    default_camera = smgr->addCameraSceneNode(default_scene_node);
    default_camera->setTarget(core::vector3df(0, 0, 0));
    scene::ISceneNodeAnimator *anim = smgr->createFlyCircleAnimator(
                core::vector3df(0, 50, 0),
                100.0f,
                0.0002f);
    default_camera->addAnimator(anim);

    default_light = smgr->addLightSceneNode(default_scene_node,
                            core::vector3df(0, 40, -40),
                            video::SColorf(1.0f, 1.0f, 1.0f),
                            20.0f);

    driver->setFog(video::SColor(0, 0, 0, 0),
                   video::EFT_FOG_LINEAR,
                   0,180);

    smgr->addCubeSceneNode(10, default_scene_node);
}

void IrrWidget::update() {
    if (updating) return;

    updating = 1;

    if (!device || !device->run()) {
        //updating = 0;
        return;
    }

    // set dimensions to widget size
    QWidget *parent_widget = (QWidget*)parent();
    QSize size = parent_widget->size();
    driver->OnResize(core::dimension2du(size.width(), size.height()));

    // update camera and light (default scene)
    default_camera->setAspectRatio((float)size.width() / size.height());
    default_light->setPosition(default_camera->getPosition());

    irrnoise->update();

    driver->beginScene(true, true, video::SColor(80, 0, 0, 0));
    smgr->drawAll();
    driver->endScene();

    irrnoise->update_end();

    updating = 0;
}
