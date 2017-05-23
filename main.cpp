#include <QApplication>
#include <QtWidgets>

static const int bufferWidth = 200, bufferHeight = 200;
static const int lightValue = 230, darkValue = 40;
static const int sleepInterval = 16;
static const int windowSize = 20;
static const int mod = 2;

class Worker : public QObject {
    Q_OBJECT

    QImage &image;
    byte *buffer[2];
    int currentBuffer = 0;

    bool running = false;
    bool abort = false;
    bool pause = false;

public:
    Worker(QImage &image)
        : image(image) {
        qsrand(QTime::currentTime().msec());

        buffer[0] = new byte[bufferWidth * bufferHeight];
        buffer[1] = new byte[bufferWidth * bufferHeight];

        randomize();
    }

public slots:
    void randomize() {
        swapBuffers();

        for (int x = 0; x < bufferWidth; x++)
            for (int y = 0; y < bufferHeight; y++)
                write(x, y, qrand() % mod);

        if (pause && running)
            render();
        else
            swapBuffers();
    }

    void run() {
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

    void finish() {
        abort = true;
    }

    void togglePause() {
        pause = !pause;
    }

signals:
    void renderFinished();

private:
    //    QPoint translateForward(QPoint p) {
    //        return QPoint(p.x() % windowSize - windowSize / 2, p.y() % windowSize - windowSize / 2);
    //    }

    //    QPoint rotate(QPoint p) {
    //        double a = M_PI / 2;

    //        double sina = sin(a);
    //        double cosa = cos(a);

    //        return QPoint(cosa * p.x() - sina * p.y(), sina * p.x() + cosa * p.y());
    //    }

    //    QPoint translateBack(QPoint p) {
    //        return QPoint(p.x() + p.x() / windowSize * windowSize + windowSize / 2, p.y() + p.y() / windowSize * windowSize + windowSize / 2);
    //    }

    void advance() {
        for (int x = 0; x < bufferWidth; x++)
            for (int y = 0; y < bufferHeight; y++) {
                int c = read(x, y);

                if (c != read(x - windowSize, y - windowSize))
                    write(x, y, qrand() % mod);
                else
                    write(x, y, c);
            }
    }

    void render() {
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

    inline int index(int x, int y) {
        if (x < 0)
            x = -x % bufferWidth;
        else if (x >= bufferWidth)
            x = bufferWidth - 1 - x % bufferWidth;

        if (y < 0)
            y = -y % bufferHeight;
        else if (y >= bufferHeight)
            y = bufferHeight - 1 - y % bufferHeight;

        return y * bufferWidth + x;
    }

    inline byte read(int x, int y) {
        return buffer[currentBuffer][index(x, y)];
    }

    inline void write(int x, int y, byte v) {
        buffer[nextBufferIndex()][index(x, y)] = v;
    }

    inline int nextBufferIndex() {
        return (currentBuffer + 1) % 2;
    }

    inline void swapBuffers() {
        currentBuffer = nextBufferIndex();
    }
};

class Widget : public QWidget {
    Q_OBJECT

    QImage image;
    QPixmap pixmap;
    QTimer timer;

public:
    Widget() {
        int size = qMin(qApp->desktop()->width(), qApp->desktop()->height()) * 0.7;
        resize(size, size);
        setMinimumSize(size / 2, size / 2);

        image = QImage(bufferWidth, bufferHeight, QImage::Format_RGB32);

        QThread *thread = new QThread;

        Worker *worker = new Worker(image);

        connect(this, SIGNAL(randomize()), worker, SLOT(randomize()));
        connect(this, SIGNAL(togglePause()), worker, SLOT(togglePause()));
        connect(worker, SIGNAL(renderFinished()), this, SLOT(updatePixmap()));
        connect(thread, SIGNAL(started()), worker, SLOT(run()));

        worker->moveToThread(thread);

        thread->start();

        connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
        timer.start(16);
    }

    virtual ~Widget() {
    }

protected:
    void keyPressEvent(QKeyEvent *e) {
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

    void paintEvent(QPaintEvent *) {
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

private slots:
    void updatePixmap() {
        pixmap = QPixmap::fromImage(image);
    }

signals:
    void randomize();
    void togglePause();
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    Widget widget;
    widget.show();

    return app.exec();
}

#include "main.moc"
