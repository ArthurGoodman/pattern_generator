#include "worker.h"

Worker::Worker() {
    qsrand(QTime::currentTime().msec());
}

void Worker::randomize() {
    swapBuffers();

    for (int x = 0; x < bufferWidth; x++)
        for (int y = 0; y < bufferHeight; y++)
            write(x, y, qrand() % mod);

    if (pause && running)
        render();
    else
        swapBuffers();
}

void Worker::run() {
    running = true;

    while (true) {
        qApp->processEvents();

        if (abort) {
            running = false;
            return;
        }

        if (!pause) {
            advance();
            render();
        }

        QThread::msleep(sleepInterval);
    }
}

void Worker::finish() {
    abort = true;
}

void Worker::togglePause() {
    pause = !pause;
}

void Worker::loadPattern(const QString &fileName) {
    QFile file(fileName);
    file.open(QFile::ReadOnly);

    QJsonDocument doc(QJsonDocument::fromJson(file.readAll()));
    QJsonObject obj = doc.object();

    bufferWidth = obj["width"].toInt(200);
    bufferHeight = obj["height"].toInt(200);

    mod = obj["mod"].toInt(2);

    QString str = obj["operation"].toString("==");

    if (str == "==")
        operation = Worker::Equal;
    else if (str == "!=")
        operation = Worker::NotEqual;

    str = obj["index"].toString("mirror");

    if (str == "mirror")
        indexMode = Worker::Mirror;
    else if (str == "wrap")
        indexMode = Worker::Wrap;

    sleepInterval = obj["sleep"].toInt(16);

    transforms.clear();

    for (QJsonValue value : obj["transforms"].toArray()) {
        QMatrix4x4 matrix;

        QJsonArray matrixData = value.toArray();

        for (int i = 0; i < 3; i++) {
            QJsonArray row = matrixData[i].toArray();

            for (int j = 0; j < 3; j++) {
                matrix(i, j) = row[j].toDouble();
            }
        }

        transforms << matrix;
    }

    initialize();
}

void Worker::initialize() {
    image = QImage(bufferWidth, bufferHeight, QImage::Format_RGB32);

    buffer[0] = new byte[bufferWidth * bufferHeight];
    buffer[1] = new byte[bufferWidth * bufferHeight];

    randomize();
}

void Worker::advance() {
    for (int x = 0; x < bufferWidth; x++)
        for (int y = 0; y < bufferHeight; y++) {
            int c = read(x, y);

            QVector3D p(x, y, 1);

            for (auto &t : transforms)
                p = t.map(p);

            if (c == read(p.x(), p.y()))
                write(x, y, operation == Equal ? c : qrand() % mod);
            else
                write(x, y, operation == NotEqual ? c : qrand() % mod);
        }
}

void Worker::render() {
    swapBuffers();

    uchar *bits = image.bits();

    for (int x = 0; x < bufferWidth; x++)
        for (int y = 0; y < bufferHeight; y++) {
            double f = static_cast<double>(mod - 1 - read(x, y)) / (mod - 1);
            uchar value = f * darkValue + (1 - f) * lightValue;

            bits[index(x, y) * 4 + 0] = value;
            bits[index(x, y) * 4 + 1] = value;
            bits[index(x, y) * 4 + 2] = value;
            bits[index(x, y) * 4 + 3] = 255;
        }

    emit renderFinished(image);
}

int Worker::index(int x, int y) {
    switch (indexMode) {
    case Mirror:
        if (x < 0)
            x = -x % bufferWidth;
        else if (x >= bufferWidth)
            x = bufferWidth - 1 - x % bufferWidth;

        if (y < 0)
            y = -y % bufferHeight;
        else if (y >= bufferHeight)
            y = bufferHeight - 1 - y % bufferHeight;

        break;

    case Wrap:
        if (x < 0)
            x = bufferWidth - (-x % bufferWidth);
        else if (x >= bufferWidth)
            x = x % bufferWidth;

        if (y < 0)
            y = bufferHeight - (-y % bufferHeight);
        else if (y >= bufferHeight)
            y = y % bufferHeight;

        break;
    }

    return y * bufferWidth + x;
}

byte Worker::read(int x, int y) {
    return buffer[currentBuffer][index(x, y)];
}

void Worker::write(int x, int y, byte v) {
    buffer[nextBufferIndex()][index(x, y)] = v;
}

int Worker::nextBufferIndex() {
    return (currentBuffer + 1) % 2;
}

void Worker::swapBuffers() {
    currentBuffer = nextBufferIndex();
}
