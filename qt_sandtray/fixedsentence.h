#ifndef FIXEDSENTENCE_H
#define FIXEDSENTENCE_H

#include <QGraphicsItem>
#include <QtGui>

#include "gamedata.h"

class FixedSentence : public QGraphicsItem
{
public:
    FixedSentence(QString sFile, GameData &currData, int idIn, int iSentPosIn);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    QString extractImageProps(QFileInfo fiIm);

    GameData* mGameData;
    int myListSlot;
    int myOverallPosition;

    QPixmap mpxImage;
    int miImageHeight;
    int miImageWidth;
};

#endif // FIXEDSENTENCE_H
