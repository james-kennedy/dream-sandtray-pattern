#include "refreshscreen.h"

// this is small class used as a thread to force the screen to update at times we specify
// it's lined to gameData on signals/slots to force the start and stop from around the game
RefreshScreen::RefreshScreen(QGraphicsScene &sceneIn, GameData &currData)
{
    mainScene = &sceneIn;
    mGameData = &currData;

    iRun = 0;
}

void RefreshScreen::start()
{
    iRun = 0;

    while(iRun > 0)
    {
        qDebug() << "I'm updating";
        mainScene->update();
    }
}

void RefreshScreen::startUpdate()
{
    QMutexLocker locker(&mutex);
    { iRun += 1; }
    qDebug() << iRun;

    while(iRun > 0)
    {
        mainScene->update();
    }
}

void RefreshScreen::singleUpdate()
{
     mainScene->update();
}

void RefreshScreen::stopUpdate()
{
    QMutexLocker locker(&mutex);
    iRun -= 1;
    qDebug() << iRun;
}

void RefreshScreen::killThread()
{
    iRun = 0;

    emit finished();    // kills the thread
}
