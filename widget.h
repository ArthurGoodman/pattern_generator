#pragma once

#include <QtWidgets>

#include "worker.h"

class Widget : public QWidget {
    Q_OBJECT

    QPixmap pixmap;
    QTimer timer;
    Worker *worker;

    QString lastPatternFileName;

public:
    Widget();

    virtual ~Widget();

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void paintEvent(QPaintEvent *);

private slots:
    void updatePixmap(const QImage &image);

signals:
    void randomize();
    void togglePause();
    void loadPattern(const QString &fileName);
};
