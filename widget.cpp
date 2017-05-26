#include "widget.h"

Widget::Widget() {
    int size = qMin(qApp->desktop()->width(), qApp->desktop()->height()) * 0.7;

    resize(size, size);
    setMinimumSize(size / 2, size / 2);

    setAcceptDrops(true);

    QThread *thread = new QThread;

    worker = new Worker;
    worker->loadPattern(lastPatternFileName = "default.json");

    connect(this, SIGNAL(randomize()), worker, SLOT(randomize()));
    connect(this, SIGNAL(togglePause()), worker, SLOT(togglePause()));
    connect(this, SIGNAL(loadPattern(QString)), worker, SLOT(loadPattern(QString)));
    connect(worker, SIGNAL(renderFinished(QImage)), this, SLOT(updatePixmap(QImage)));
    connect(thread, SIGNAL(started()), worker, SLOT(run()));

    worker->moveToThread(thread);

    thread->start();

    connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
    timer.start(16);
}

Widget::~Widget() {
}

void Widget::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasFormat("text/uri-list"))
        e->acceptProposedAction();
}

void Widget::dropEvent(QDropEvent *e) {
    emit loadPattern(lastPatternFileName = e->mimeData()->urls().first().toLocalFile());

    activateWindow();
    raise();
}

void Widget::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
    case Qt::Key_Escape:
        isFullScreen() ? showNormal() : qApp->quit();
        break;

    case Qt::Key_F11:
        isFullScreen() ? showNormal() : showFullScreen();
        break;

    case Qt::Key_R:
        emit randomize();
        break;

    case Qt::Key_Space:
        emit togglePause();
        break;

    case Qt::Key_F5:
        emit loadPattern(lastPatternFileName);
        break;
    }
}

void Widget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), Qt::lightGray);

    if (pixmap.isNull())
        return;

    double wr = (double) width() / height();
    double pr = (double) pixmap.width() / pixmap.height();

    QSize newSize;

    if (wr < pr)
        newSize = pixmap.size() * width() / pixmap.width();
    else
        newSize = pixmap.size() * height() / pixmap.height();

    p.drawPixmap(rect().center() - QPoint(newSize.width(), newSize.height()) / 2, pixmap.scaled(newSize + QSize(1, 1)));
}

void Widget::updatePixmap(const QImage &image) {
    pixmap = QPixmap::fromImage(image);
}
