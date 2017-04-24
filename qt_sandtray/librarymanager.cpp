#include "librarymanager.h"

LibraryManager::LibraryManager(QGraphicsScene &sceneIn, GameData &dataIn)
{
    mMainScene = &sceneIn;               // access to the scene to directly add images
    mGameData = &dataIn;                 // take the gameData structure to populate/pass on
    msLibRoot = mGameData->getLibraryPath();
    mGameData->setLibraryId(-1);        // init at -1 as we call nextLibrary() to bring up a new lib, making start lib0
    miLastRight = 0;

    if (!mGameData->getLibraryTestReserved())
        mGameData->setLibraryLimit(QDir(msLibRoot).entryList(QDir::Dirs | QDir::NoDotAndDotDot).count());

    QObject::connect(mGameData, SIGNAL(newGame()), this, SLOT(nextLibrary()));
    QObject::connect(mGameData, SIGNAL(resetGame()), this, SLOT(reloadCurrentLibrary()));
}

LibraryManager::~LibraryManager()
{
}

// go to next library and loop round if we reach library limit
void LibraryManager::nextLibrary()
{
    int iLibrary = mGameData->getLibraryId();
    iLibrary++;

    if (iLibrary >= mGameData->getLibraryLimit())
        iLibrary = 0;

    mGameData->setLibraryId(iLibrary);

    loadLibrary();
}

void LibraryManager::reloadCurrentLibrary()
{
    loadLibrary();
}

// loads the library specified in iCurrLib - creates the category and image objects and adds them to the scene
void LibraryManager::loadLibrary()
{
    int iCurrLib = mGameData->getLibraryId();
    mGameData->setCollisionCheck(false);     // turn off collision checking - causes a crash if on whilst library changing

    QStringList libFolders = QDir(msLibRoot).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (libFolders.count() < 1)
    {
        qDebug() << "No library folders found.";
    }
    else
    {
        QDir libDir(msLibRoot + libFolders.at(iCurrLib));

        // pull in the images from the correct folder - only get png or jpg files
        if (libDir.exists())
        {          
            QStringList nameFilter;
            nameFilter << "*.png" << "*.jpg";

            // for the DREAM game, we often intentionally don't want categories, so remove check
            //int iTotalCats = getNumberOfCategoriesInLibrary(libDir, nameFilter);
            //
            //if (iTotalCats < 1)
            //{
            //    qDebug() << "No categories found in library. Please add at least 1 image file starting 'cat'.";
            //}
            //else
            //{
                mMainScene->clear();             // clear scene, also destroys objects
                mGameData->clearImageDetails();  // clear out our data structures
                mGameData->clearSentDetails();
                mGameData->clearCatDetails();
                miLastRight = 0;

                QString sLibProps = extractLibraryProps(libFolders.at(iCurrLib));
                mGameData->setLibraryProperties(sLibProps);

                if (mGameData->getShowButtons())
                {
                    // add new library button
                    QFileInfo fiNewButton(mGameData->getNewLibButton());
                    if (fiNewButton.isFile())
                    {
                        LibraryButton *newButton = new LibraryButton(fiNewButton.absoluteFilePath(), *mGameData, true);
                        QPointF qpfNewLib = getButtonPosition(fiNewButton.absoluteFilePath(), true);
                        newButton->setPos(qpfNewLib);
                        mMainScene->addItem(newButton);
                    }

                    // add reset library button
                    QFileInfo fiResetButton(mGameData->getResetLibButton());
                    if (fiResetButton.isFile())
                    {
                        LibraryButton *resetButton = new LibraryButton(fiResetButton.absoluteFilePath(), *mGameData, false);
                        QPointF qpfResetLib = getButtonPosition(fiResetButton.absoluteFilePath(), false);
                        resetButton->setPos(qpfResetLib);
                        mMainScene->addItem(resetButton);
                    }
                }        

                int iImgCounter = 0, iCatCounter = 0, iSentCounter = 0, iFullCounter = 0;

                // load all categories first so when we add the images, we know where they are and don't overlap
                foreach(QFileInfo fiImage, libDir.entryInfoList(nameFilter, QDir::Files))
                {
                    if (fiImage.isFile())
                    {
                        if (fiImage.fileName().mid(0,3).toUpper() == "SEN")
                        {
                            // verify that the order has been preserved
                            int iThisSentencePos = fiImage.fileName().mid(3,1).toInt();

                            if (iThisSentencePos != iFullCounter)
                            {
                                // throw an error, message box etc!
                            }
                            else
                            {
                                if (fiImage.fileName().mid(4,3).toUpper() == "BLA")
                                {
                                    // new category - adds itself to the gameData structure, but we get it's position from here
                                    Category *newCat = new Category(fiImage.absoluteFilePath(), *mGameData, iCatCounter);
                                    GameData::CategoryDetails thisCat = mGameData->getCatDetails()[iCatCounter];
                                    QPointF qpfPos = getCategoryPosition(miLastRight, thisCat.qsCatSize.width(), thisCat.qsCatSize.height());
                                    newCat->setPos(qpfPos);
                                    mMainScene->addItem(newCat);
                                    mGameData->setCatPos(iCatCounter, qpfPos);

                                    iCatCounter++;
                                }
                                else
                                {
                                    // same as above, but for new object type which is like category, but with no real detail
                                    // new category - adds itself to the gameData structure, but we get it's position from here
                                    FixedSentence *newSent = new FixedSentence(fiImage.absoluteFilePath(), *mGameData, iSentCounter, iFullCounter);
                                    GameData::SentenceDetails thisSent = mGameData->getSentDetails()[iSentCounter];
                                    QPointF qpfPos = getCategoryPosition(miLastRight, thisSent.qsSentSize.width(), thisSent.qsSentSize.height());
                                    newSent->setPos(qpfPos);
                                    mMainScene->addItem(newSent);
                                    mGameData->setSentPos(iSentCounter, qpfPos);

                                    iSentCounter++;
                                }

                                iFullCounter++;
                            }
                        }
                    }
                }

                foreach(QFileInfo fiImage, libDir.entryInfoList(nameFilter, QDir::Files))
                {
                    if (fiImage.isFile())
                    {
                        if (fiImage.fileName().mid(0,3).toUpper() != "SEN")
                        {
                            // new image for categorising - adds itself to the gameData structure
                            DragImage *newImage = new DragImage(fiImage.absoluteFilePath(), *mGameData, iImgCounter);
                            GameData::ImageDetails thisIm = mGameData->getImageDetails()[iImgCounter];
                            QPointF qpfPos = getImagePosition(thisIm.qsImageSize.width(), thisIm.qsImageSize.height(), iImgCounter);
                            newImage->setPos(qpfPos);
                            mMainScene->addItem(newImage);
                            mGameData->setImagePositionById(iImgCounter, qpfPos);

                            iImgCounter++;
                        }
                    }
                }

                // if we are showing 1 at a time then give it index 0
                if (mGameData->getOneAtATime())
                {
                    mGameData->setShuffleOrder();
                    mGameData->setCurrOneToShow(0);
                }

                emit libraryLoaded();
                // now all images are in, restart the collision detection
                mGameData->setSingleScreenUpdate();
                mGameData->setCollisionCheck(true);
            //}
        }
        else
        {
            qDebug() << "Lib folders found, but index given was wrong.";
        }
    }
}

// get the library properties from the foldername - use underscore delimiter
QString LibraryManager::extractLibraryProps(QString sLibDirectory)
{
    QStringList splitString = sLibDirectory.split("_");
    splitString.removeAt(0);        // remove lib UID from start
    return splitString.join("_");   // rejoin props to be unpacked by robot
}

// loops through the library path and counts the number of categories are present (file must start 'cat...')
int LibraryManager::getNumberOfCategoriesInLibrary(QDir libDir, QStringList nameFilter)
{
    int iNum = 0;

    foreach(QFileInfo fiImage, libDir.entryInfoList(nameFilter, QDir::Files))
    {
        if (fiImage.isFile())
        {
            if (fiImage.fileName().mid(0,3).toUpper() == "SEN")
            {
                iNum++;
            }
        }
    }

    return iNum;
}

// return X and Y position for a category based on how many total and which one this is; implemented for 1, 2 or 4 cats
QPointF LibraryManager::getCategoryPosition(int iLastRight, int iImWidth, int iImHeight)
{
   int iScreenWidth = mGameData->getScreenSize().width();
   int iScreenHeight = mGameData->getScreenSize().height();

   if (iLastRight == 0)
   {
       miLastRight = (-iScreenWidth / 2) + iImWidth;
       return QPointF((-iScreenWidth / 2) + (iImWidth / 2) + 4, ((iScreenHeight / 2) - (iImHeight / 2)));
   }
   else
   {
       QPointF qpfPosition = QPointF(iLastRight + (iImWidth / 2) + 4, ((iScreenHeight / 2) - (iImHeight / 2)));
       miLastRight += iImWidth;
       return qpfPosition;
   }
}

// pick a random position within the screen bounds taking into account the image size
// see if it overlaps with any category image - loop again if it does
QPointF LibraryManager::getImagePosition(int imWidth, int imHeight, int iCount)
{
    int iXPos, iYPos;

    iYPos = 0 - (imHeight / 2);

    if (iCount == 0)
        iXPos = 0 - (imWidth * 2) - 10;
    else if (iCount == 1)
        iXPos = 0 + (imWidth * 2) + 10;
    else if (iCount == 2)
        iXPos = 0 - (imWidth / 2) - 20;
    else
        iXPos = 0 + (imWidth / 2) + 20;

    return QPointF(iXPos, iYPos);
}

// return the position for the new and reset buttons based on the number of categories
QPointF LibraryManager::getButtonPosition(QString sFilePath, bool bNewLibrary)
{
    int iScreenWidth = mGameData->getScreenSize().width();
    int iScreenHeight = mGameData->getScreenSize().height();
    int iScreenL = - iScreenWidth / 2;
    int iScreenT = - iScreenHeight / 2;
    int iScreenB = iScreenHeight / 2;
    int iXPos, iYPos;

    QImage qiImage;
    qiImage.load(sFilePath);
    int iImageH = qiImage.height();
    int iImageW = qiImage.width();

    if (bNewLibrary)
        iYPos = iScreenB - (iImageH / 2);       // bottom of screen
    else
        iYPos = iScreenT + (iImageH / 2);       // top of screen

    iXPos = iScreenL + (iImageW / 2);           // left edge

    return QPointF(iXPos, iYPos);
}

int LibraryManager::getRandomNumber(int min, int max)
{
   return (qrand() % ((max + 1) - min)) + min;
}
