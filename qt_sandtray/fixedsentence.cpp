#include "fixedsentence.h"

FixedSentence::FixedSentence(QString sFile, GameData &currData, int idIn, int iSentPosIn)
{
    mGameData = &currData;
    myListSlot = idIn;
    myOverallPosition = iSentPosIn;

    QImage qiImage;
    qiImage.load(sFile);
    mpxImage = QPixmap::fromImage(qiImage);
    miImageHeight = mpxImage.height();
    miImageWidth = mpxImage.width();

    QFileInfo fiImage(sFile);
    fiImage.fileName();

    GameData::SentenceDetails myDetails;
    myDetails.iSentId = myListSlot;
    myDetails.iSentPos = myOverallPosition;
    myDetails.qsSentSize = QSize(miImageWidth, miImageHeight);
    myDetails.sSentProps = extractImageProps(fiImage);
    myDetails.qpfSentPosition = scenePos();

    mGameData->addSentDetails(myDetails);

    setZValue(-1000);                                           // place at the back, so dragImages always go on top
}

// get the image properties from the filename - use underscore delimiter
QString FixedSentence::extractImageProps(QFileInfo fiIm)
{
    QRegExp rx("(\\_|\\.)"); //RegEx for '.' or '_'
    QStringList splitString = fiIm.fileName().split(rx);

    splitString.removeAt(0);                        // remove cat/id at start
    splitString.removeAt(splitString.count() - 1);  // remove file type at end

    return splitString.join("_");
}

// get bounding rectange for image - works from top left, so to make image centre of the box
// we need to multiply the image width and height by -0.5 to give the top left point
QRectF FixedSentence::boundingRect() const
{
    return QRectF(miImageWidth * -0.5, miImageHeight * -0.5, miImageWidth, miImageHeight);
}

// draws the sentence element on screen - update to change appearance
void FixedSentence::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->drawPixmap(QPointF(miImageWidth * -0.5, miImageHeight * -0.5), mpxImage);
}
