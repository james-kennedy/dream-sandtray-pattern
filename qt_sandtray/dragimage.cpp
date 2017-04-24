#include "dragimage.h"

// construct the image data and add it to the gamedata currently used
DragImage::DragImage(QString sFile, GameData &currData, int idIn)
{
    mGameData = &currData;
    myListSlot = idIn;

    QImage qiImage;
    qiImage.load(sFile);
    mpxImage = QPixmap::fromImage(qiImage);
    miImageHeight = mpxImage.height();
    miImageWidth = mpxImage.width();
    mScaledSize = QSize(0,0);
    mflDistanceMoved = 0;
    mqiStartImageMove = 0;

    QFileInfo fiImage(sFile);

    // create image struct and add it to the gamedata list of images
    GameData::ImageDetails myDetails;
    myDetails.imageId = myListSlot;
    myDetails.imageProps = extractImageProps(fiImage);
    myDetails.catBelonged = fiImage.fileName().mid(0, 1).toUpper(); // first character is the category
    myDetails.qpfImagePosition = QPointF(1000, 1000);               // default off screen
    myDetails.qsImageSize = QSize(miImageWidth, miImageHeight);
    myDetails.imageOwned = false;
    myDetails.imageActive = true;
    myDetails.bRobotMoving = false;
    myDetails.bRobotLastOwner = false;
    myDetails.catPlaced = -1;
    myDetails.iCatOverlap = -1;

    mGameData->addImageDetails(myDetails);
}

// get the image properties from the filename - use underscore delimiter
QString DragImage::extractImageProps(QFileInfo fiIm)
{
    QRegExp rx("(\\_|\\.)"); //RegEx for '.' or '_'
    QStringList splitString = fiIm.fileName().split(rx);

    splitString.removeAt(0);                        // remove cat/id at start
    splitString.removeAt(splitString.count() - 1);  // remove file type at end

    return splitString.join("_");
}

// get bounding rectange for image - works from top left, so to make image centre of the box
// we need to multiply the image width and height by -0.5 to give the top left point
QRectF DragImage::boundingRect() const
{
    return QRectF(miImageWidth * -0.5, miImageHeight * -0.5, miImageWidth, miImageHeight);
}

// draws the image on screen - update to change appearance
void DragImage::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // make sure that the one we are showing is always on top if doing one-at-a-time
    if (myListSlot == mGameData->getCurrOneToShow())
        setZValue(50);

    // only paint here if we aren't doing one-at-a-time, or it is active, we are doing one-at-a-time and it is the current one to show, or one-at-a-time, but categorised
    if ((mGameData->getImageActive(myListSlot) && myListSlot == mGameData->getCurrOneToShow()) || !mGameData->getImageActive(myListSlot) || !mGameData->getOneAtATime())
        painter->drawPixmap(QPointF(miImageWidth * -0.5, miImageHeight * -0.5), mpxImage);

    if (mGameData->getImageOwned(myListSlot))
    {
        QPixmap* pixOverlay = new QPixmap(miImageWidth, miImageHeight);                         // create pixmap overlay the size of the image
        pixOverlay->fill(Qt::gray);                                                             // set the overlay colour
        pixOverlay->setMask(mpxImage.createMaskFromColor(Qt::transparent));                     // don't colour parts of the image which are transparent
        painter->setOpacity(0.4);                                                               // apply transparency to our overlay
        painter->drawPixmap(QPointF(miImageWidth * -0.5, miImageHeight * -0.5), *pixOverlay);   // draw the overlay
        delete pixOverlay;                                                                      // destroy the item to prevent mem leak
    }

    if (!mGameData->getImageActive(myListSlot))
    {
        setLockPositionAndScale();                          // inactive so paint in place
    }
    else if (mGameData->getRobotMoving(myListSlot))
    {
        if (mGameData->getRobotReadyToMove())
            setRobotMovePosition();                         // robot moving, so update position manually
    }
}

// move the image and update the game structure for the new pos - scale here too (optional var)
void DragImage::updatePositionOfImage(QPointF qpfPosition, float flScale)
{
    prepareGeometryChange();
    setPos(qpfPosition);

    if (flScale > 0)
        setScale(flScale);

    mGameData->setImagePositionById(myListSlot, qpfPosition);
}

// when pressed declare it as owned so others can't access, also set the start position so we can measure move amount
void DragImage::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if (mGameData->getImageActive(myListSlot) && !mGameData->getImageOwned(myListSlot) && !mGameData->getRobotMoving(myListSlot) && !mGameData->getRobotLocked())
    {
        if (!mGameData->getOneAtATime() || myListSlot == mGameData->getCurrOneToShow())
        {
            mGameData->setImageOwned(myListSlot, true);
            mGameData->setRobotOwned(myListSlot, false);
            qpfPreviousPosition = event->scenePos();
            updatePositionOfImage(qpfPreviousPosition);

            mqiStartImageMove = QDateTime::currentMSecsSinceEpoch();    // reset start of move params
            mflDistanceMoved = 0;

            // if this isn't the first move, then update delay
            qint64 qiLastMoveEnd = mGameData->getLastMoveEnd();

            if (qiLastMoveEnd != 0)
            {
                float flDelay = float((mqiStartImageMove - qiLastMoveEnd))  / 1000;
                mGameData->setNewDelay(flDelay);
            }

            setZValue(10);
        }
    }
}

// on release permit access again, use final position to update move amount
void DragImage::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if (mGameData->getImageActive(myListSlot) && !mGameData->getRobotMoving(myListSlot) && !mGameData->getRobotLocked())
    {
        if (!mGameData->getOneAtATime() || myListSlot == mGameData->getCurrOneToShow())
        {
            mGameData->setImageOwned(myListSlot, false);
            updatePositionOfImage(event->scenePos());

            qint64 qiTimeNow = QDateTime::currentMSecsSinceEpoch();
            float flMoveTime;
            flMoveTime = float((qiTimeNow - mqiStartImageMove)) / 1000;

            mGameData->setLastMoveEnd(qiTimeNow);                   // used for delay calcs
            mGameData->setNewTime(flMoveTime);                      // add to time list
            mGameData->setNewDistance(mflDistanceMoved);            // add to distance list - calculated in mouseMoveEvent
            mGameData->setNewSpeed(mflDistanceMoved / flMoveTime);  // speed in pixels per second

            setZValue(0);
        }
    }
}

// continually update the scenePos in order to redraw the image, keep gamedata updated too
void DragImage::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    if (mGameData->getImageActive(myListSlot) && !mGameData->getRobotMoving(myListSlot) && !mGameData->getRobotLocked())
    {
        if (!mGameData->getOneAtATime() || myListSlot == mGameData->getCurrOneToShow())
        {
            QPointF qpfCurrentPos = event->scenePos();
            updatePositionOfImage(qpfCurrentPos);

            // to get accurate measure of distance moved, update distance every event
            int iXMove = qpfCurrentPos.x() - qpfPreviousPosition.x();
            int iYMove = qpfCurrentPos.y() - qpfPreviousPosition.y();
            mflDistanceMoved += sqrt(double((iXMove * iXMove) + (iYMove * iYMove)));

            qpfPreviousPosition = qpfCurrentPos;    // update for next move
        }
    }
}

void DragImage::setLockPositionAndScale()
{
    // get lock position for category - need category info
    int iCatPlaced = mGameData->getCatPlaced(myListSlot);
    GameData::CategoryDetails myCat = mGameData->getCatDetails()[iCatPlaced];

    int iXPos = 0, iYPos = 0;
    iXPos = myCat.qpfCatPosition.x();
    iYPos = myCat.qpfCatPosition.y();

    updatePositionOfImage(QPointF(iXPos, iYPos), 1);

    setZValue(-1000);
}

// loop through the category image list and find the position for this image
int DragImage::getMyPositionInCategory(int iCatPlaced)
{
    QList<int> inCatList = mGameData->getInsideCategory(iCatPlaced);

    for (int iCount = 0; iCount < inCatList.count(); iCount++)
    {
        if (inCatList[iCount] == myListSlot)
            return iCount;      // exit with the value now
    }

    return 0;                   // not there, so return 0
}

void DragImage::setRobotMovePosition()
{
    qint64 iTimeStart = mGameData->getRobotMoveStart();
    qint64 iTimeNow = QDateTime::currentMSecsSinceEpoch();
    int iTotalTime = mGameData->getRobotMoveTimeMs(myListSlot);
    float flPercent = (float(iTimeNow) - float(iTimeStart)) / float(iTotalTime);

    if (flPercent >= 1)
    {
        flPercent = 1;                                  // don't let it overshoot
        mGameData->setRobotMoving(myListSlot, false);   // now stop the move
        mGameData->setRobotReadyToMove(false);          // prevent move for next loop
        mGameData->setImageOwned(myListSlot, false);    // release item for categorisation/user access
        //mGameData->setForceScreenUpdate(false);         // assumes robot can only move 1 at a time
    }

    QPointF qpfNewPosition = mGameData->getPointOnRobotMove(myListSlot, flPercent);

    // bring inside bounds of screen if it goes out - otherwise we lose it!
    QSize iScreen = mGameData->getScreenSize();
    int iScreenX = iScreen.width() / 2;
    int iScreenY = iScreen.height() / 2;

    if (qpfNewPosition.x() > iScreenX) qpfNewPosition.setX(iScreenX);
    if (qpfNewPosition.x() < -iScreenX) qpfNewPosition.setX(-iScreenX);
    if (qpfNewPosition.y() > iScreenY) qpfNewPosition.setY(iScreenY);
    if (qpfNewPosition.y() < -iScreenY) qpfNewPosition.setY(-iScreenY);

    //qDebug() << "Robot move %: " << flPercent;
    //qDebug() << "Robot move to: " << qpfNewPosition;
    updatePositionOfImage(qpfNewPosition);
}
