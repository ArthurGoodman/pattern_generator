#include <QApplication>
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
    Worker(QImage &image)
        : image(image) {
    }

    void clearTransforms() {
        transforms.clear();
    }

    void addTransform(QMatrix4x4 t) {
        transforms << t;
    }

    void setBufferWidth(int bufferWidth) {
        this->bufferWidth = bufferWidth;
    }

    void setBufferHeight(int bufferHeight) {
        this->bufferHeight = bufferHeight;
    }

    void setMod(int mod) {
        this->mod = mod;
    }

    void setSleepInterval(int sleepInterval) {
        this->sleepInterval = sleepInterval;
    }

    void setOperation(Operation operation) {
        this->operation = operation;
    }

    void setIndexMode(IndexMode indexMode) {
        this->indexMode = indexMode;
    }

    void initialize() {
        qsrand(QTime::currentTime().msec());

        image = QImage(bufferWidth, bufferHeight, QImage::Format_RGB32);

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
    void advance() {
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
    Worker *worker;

public:
    Widget() {
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

private:
    void loadPattern(const QString &fileName) {
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
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    Widget widget;
    widget.show();

    return app.exec();
}

#include "main.moc"
