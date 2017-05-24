#include "worker.h"

Worker::Worker(QImage &image)
    : image(image) {
}

void Worker::clearTransforms() {
    transforms.clear();
}

void Worker::addTransform(QMatrix4x4 t) {
    transforms << t;
}

void Worker::setBufferWidth(int bufferWidth) {
    this->bufferWidth = bufferWidth;
}

void Worker::setBufferHeight(int bufferHeight) {
    this->bufferHeight = bufferHeight;
}

void Worker::setMod(int mod) {
    this->mod = mod;
}

void Worker::setSleepInterval(int sleepInterval) {
    this->sleepInterval = sleepInterval;
}

void Worker::setOperation(Worker::Operation operation) {
    this->operation = operation;
}

void Worker::setIndexMode(Worker::IndexMode indexMode) {
    this->indexMode = indexMode;
}

void Worker::initialize() {
    qsrand(QTime::currentTime().msec());

    image = QImage(bufferWidth, bufferHeight, QImage::Format_RGB32);

    buffer[0] = new byte[bufferWidth * bufferHeight];
    buffer[1] = new byte[bufferWidth * bufferHeight];

    randomize();
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

    emit renderFinished();
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
