#pragma once

#include <QtWidgets>

#include "worker.h"

class Widget : public QWidget {
    Q_OBJECT

    QImage image;
    QPixmap pixmap;
    QTimer timer;
    Worker *worker;

public:
    Widget();

    virtual ~Widget();

protected:
    void keyPressEvent(QKeyEvent *e);
    void paintEvent(QPaintEvent *);

private slots:
    void updatePixmap();

signals:
    void randomize();
    void togglePause();

private:
    void loadPattern(const QString &fileName);
};
