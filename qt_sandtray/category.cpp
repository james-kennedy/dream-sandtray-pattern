#include "category.h"

// construct the image data and add it to the gamedata currently used
Category::Category(QString sFile, GameData &currData, int idIn)
{
    mGameData = &currData;
    myListSlot = idIn;

    QImage qiImage;
    qiImage.load(sFile);
    mpxImage = QPixmap::fromImage(qiImage);
    miImageHeight = mpxImage.height();
    miImageWidth = mpxImage.width();

    QFileInfo fiImage(sFile);
    fiImage.fileName();

    // create image struct and add it to the gamedata list of images
    GameData::CategoryDetails myDetails;
    myDetails.catId = myListSlot;
    myDetails.catName = fiImage.fileName().mid(8,1).toUpper();  // first 3 are 'cat', next letter is name
    myDetails.qsCatSize = QSize(miImageWidth, miImageHeight);
    myDetails.catProps = extractImageProps(fiImage);
    myDetails.qpfCatPosition = scenePos();
    myDetails.currOverlap = false;
    myDetails.bShowFeedback = false;
    myDetails.bCorrectFeedback = false;
    myDetails.iFeedbackStart = 0;

    mGameData->addCatDetails(myDetails);

    setZValue(-1000);                                           // place at the back, so dragImages always go on top
}

// get the image properties from the filename - use underscore delimiter
QString Category::extractImageProps(QFileInfo fiIm)
{
    QRegExp rx("(\\_|\\.)"); //RegEx for '.' or '_'
    QStringList splitString = fiIm.fileName().split(rx);

    splitString.removeAt(0);                    // remove cat/id at start
    splitString.removeAt(splitString.count() - 1);  // remove file type at end

    return splitString.join("_");
}

// get bounding rectange for image - works from top left, so to make image centre of the box
// we need to multiply the image width and height by -0.5 to give the top left point
QRectF Category::boundingRect() const
{
    return QRectF(miImageWidth * -0.5, miImageHeight * -0.5, miImageWidth, miImageHeight);
}

// draws the category on screen - update to change appearance
void Category::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // make slightly transparent when there is an overlap
    if (mGameData->getCurrOverlap(myListSlot))
        painter->setOpacity(0.2);
    else
        painter->setOpacity(1.0);

    painter->drawPixmap(QPointF(miImageWidth * -0.5, miImageHeight * -0.5), mpxImage);

    // if something has just been categorised, then overlay the feedback if turned on
    if (mGameData->getShowFeedback())
    {
        if (mGameData->getShowCategoryFeedback(myListSlot))
        {
            // only want to show for a short time
            int iStart = mGameData->getFeedbackStart(myListSlot);
            int iTimeNow = QDateTime::currentMSecsSinceEpoch();
            int iDiff = iTimeNow - iStart;

            // the start is defaulted to 0 - ignore if this is called before the value is updated
            if (iStart != 0)
            {
                if (iDiff > 2000)
                {
                    //qDebug() << "feedback timeout";
                    //mGameData->setForceScreenUpdate(false);
                    mGameData->setShowCategoryFeedback(myListSlot, false);
                    setZValue(-1000);
                }
                else
                {
                    setZValue(100);
                    QPixmap* pixOverlay;
                    if (mGameData->getIsFeedbackCorrect(myListSlot))
                        pixOverlay = new QPixmap(mGameData->getCorrectFeedback());
                    else
                        pixOverlay = new QPixmap(mGameData->getIncorrectFeedback());

                    float flHScale = pixOverlay->height() / float(miImageHeight);               // scale the image feedback so we can use any size going in
                    float flWScale = pixOverlay->width() / float(miImageWidth);

                    if (flHScale > flWScale)
                       *pixOverlay = pixOverlay->scaledToHeight(miImageHeight);
                    else
                       *pixOverlay = pixOverlay->scaledToWidth(miImageWidth);

                    float flTransparency = 1;
                    if (iDiff == 0)
                        iDiff = 1;  // prevent division by 0

                    if (iDiff <= 500)
                        flTransparency = float(iDiff) / 500;

                    if (iDiff >= 1500)
                        flTransparency = float((2000 - iDiff)) / 500;

                    painter->setOpacity(flTransparency);                                    // apply transparency to our overlay
                    painter->drawPixmap(QPointF(miImageWidth * -0.5, miImageHeight * -0.5), *pixOverlay);       // draw the overlay - use centre as category centre
                    delete pixOverlay;
                }
            }
        }
    }
}
