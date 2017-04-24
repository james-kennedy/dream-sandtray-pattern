#ifndef LIBRARY_MANAGER_H
#define LIBRARY_MANAGER_H

#include <QtGui>
#include <QObject>

#include "dragimage.h"
#include "category.h"
#include "fixedsentence.h"
#include "librarybutton.h"

class LibraryManager : public QObject
{
    Q_OBJECT

public:
    LibraryManager(QGraphicsScene &sceneIn, GameData &dataIn);
   ~LibraryManager();

signals:
    void libraryLoaded();

public slots:
    void nextLibrary();
    void reloadCurrentLibrary();

private:
    void loadLibrary();
    QString extractLibraryProps(QString sLibDirectory);
    int getNumberOfCategoriesInLibrary(QDir libDir, QStringList nameFilter);
    QPointF getCategoryPosition(int iLastRight, int iImWidth, int iImHeight);
    QPointF getImagePosition(int imWidth, int imHeight, int iCount);
    QPointF getButtonPosition(QString sFilePath, bool bNewLibrary);
    int getRandomNumber(int min, int max);

    QGraphicsScene* mMainScene;
    GameData* mGameData;

    QString msLibRoot;
    int miLastRight;
};

#endif // LIBRARY_MANAGER_H
