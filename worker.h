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
    QImage image;
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
    Worker();

public slots:
    void randomize();
    void run();
    void finish();
    void togglePause();
    void loadPattern(const QString &fileName);

signals:
    void renderFinished(const QImage &image);

private:
    void initialize();

    void advance();
    void render();

    int index(int x, int y);

    byte read(int x, int y);
    void write(int x, int y, byte v);
    int nextBufferIndex();
    void swapBuffers();
};
