#include "widget.h"

Widget::Widget() {
    int size = qMin(qApp->desktop()->width(), qApp->desktop()->height()) * 0.7;
    resize(size, size);
    setMinimumSize(size / 2, size / 2);

    QThread *thread = new QThread;

    worker = new Worker(image);

    loadPattern("pattern.json");
    worker->initialize();

    connect(this, SIGNAL(randomize()), worker, SLOT(randomize()));
    connect(this, SIGNAL(togglePause()), worker, SLOT(togglePause()));
    connect(worker, SIGNAL(renderFinished()), this, SLOT(updatePixmap()));
    connect(thread, SIGNAL(started()), worker, SLOT(run()));

    worker->moveToThread(thread);

    thread->start();

    connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
    timer.start(16);
}

Widget::~Widget() {
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

void Widget::updatePixmap() {
    pixmap = QPixmap::fromImage(image);
}

void Widget::loadPattern(const QString &fileName) {
    QFile file(fileName);
    file.open(QFile::ReadOnly);

    QJsonDocument doc(QJsonDocument::fromJson(file.readAll()));
    QJsonObject obj = doc.object();

    worker->setBufferWidth(obj["width"].toInt(200));
    worker->setBufferHeight(obj["height"].toInt(200));

    worker->setMod(obj["mod"].toInt(2));

    QString oper = obj["operation"].toString("==");

    if (oper == "==")
        worker->setOperation(Worker::Equal);
    else if (oper == "!=")
        worker->setOperation(Worker::NotEqual);

    QString index = obj["index"].toString("mirror");

    if (index == "mirror")
        worker->setIndexMode(Worker::Mirror);
    else if (index == "wrap")
        worker->setIndexMode(Worker::Wrap);

    worker->setSleepInterval(obj["sleep"].toInt(16));

    worker->clearTransforms();

    for (QJsonValue value : obj["transforms"].toArray()) {
        QMatrix4x4 matrix;

        QJsonArray matrixData = value.toArray();

        for (int i = 0; i < 3; i++) {
            QJsonArray row = matrixData[i].toArray();

            for (int j = 0; j < 3; j++) {
                matrix(i, j) = row[j].toDouble();
            }
        }

        worker->addTransform(matrix);
    }
}
