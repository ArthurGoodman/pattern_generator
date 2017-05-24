#pragma once

#include <QtWidgets>

static const int lightValue = 230, darkValue = 40;

class Worker : public QObject {
    Q_OBJECT

public:
    enum Operation {
        Equal,
        NotEqual
    };

    enum IndexMode {
        Mirror,
        Wrap
    };

private:
    QImage &image;
    byte *buffer[2];
    int currentBuffer = 0;

    bool running = false;
    bool abort = false;
    bool pause = false;

    QVector<QMatrix4x4> transforms;
    int mod = 2, sleepInterval = 16, bufferWidth = 200, bufferHeight = 200;
    Operation operation = Equal;
    IndexMode indexMode = Mirror;

public:
    Worker(QImage &image);

    void clearTransforms();

    void addTransform(QMatrix4x4 t);

    void setBufferWidth(int bufferWidth);
    void setBufferHeight(int bufferHeight);
    void setMod(int mod);
    void setSleepInterval(int sleepInterval);
    void setOperation(Operation operation);
    void setIndexMode(IndexMode indexMode);

    void initialize();

public slots:
    void randomize();
    void run();
    void finish();
    void togglePause();

signals:
    void renderFinished();

private:
    void advance();
    void render();

    int index(int x, int y);

    byte read(int x, int y);
    void write(int x, int y, byte v);
    int nextBufferIndex();
    void swapBuffers();
};
