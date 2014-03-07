#include "main.h"

int main(int argc, char **argv)
{
    Logger * logger = new Logger;

    QApplication app(argc, argv);
    MainWindow * window = new MainWindow(logger);
    window->show();

    return app.exec();
}

MainWindow::MainWindow(Logger * logger)
 : logger(logger),
   depthImage(640, 480, QImage::Format_RGB888),
   rgbImage(640, 480, QImage::Format_RGB888),
   recording(false),
   lastDrawn(-1)
{
    this->setMaximumSize(1280, 600);
    this->setMinimumSize(1280, 600);

    QVBoxLayout * wrapperLayout = new QVBoxLayout;

    QHBoxLayout * mainLayout = new QHBoxLayout;
    QHBoxLayout * fileLayout = new QHBoxLayout;
    QHBoxLayout * buttonLayout = new QHBoxLayout;

    wrapperLayout->addLayout(mainLayout);

    depthLabel = new QLabel(this);
    depthLabel->setPixmap(QPixmap::fromImage(depthImage));
    mainLayout->addWidget(depthLabel);

    imageLabel = new QLabel(this);
    imageLabel->setPixmap(QPixmap::fromImage(rgbImage));
    mainLayout->addWidget(imageLabel);

    wrapperLayout->addLayout(fileLayout);

    QLabel * logLabel = new QLabel("Log file: ", this);
    logLabel->setMaximumWidth(logLabel->fontMetrics().boundingRect(logLabel->text()).width());
    fileLayout->addWidget(logLabel);

    logFile = new QLabel(this);
    logFile->setTextInteractionFlags(Qt::TextSelectableByMouse);
    logFile->setStyleSheet("border: 1px solid grey");
    fileLayout->addWidget(logFile);

    browseButton = new QPushButton("Browse", this);
    browseButton->setMaximumWidth(browseButton->fontMetrics().boundingRect(browseButton->text()).width() + 10);
    connect(browseButton, SIGNAL(clicked()), this, SLOT(fileBrowse()));
    fileLayout->addWidget(browseButton);

    dateNameButton = new QPushButton("Date filename", this);
    dateNameButton->setMaximumWidth(dateNameButton->fontMetrics().boundingRect(dateNameButton->text()).width() + 10);
    connect(dateNameButton, SIGNAL(clicked()), this, SLOT(dateFilename()));
    fileLayout->addWidget(dateNameButton);

    wrapperLayout->addLayout(buttonLayout);

    startStop = new QPushButton("Record", this);
    connect(startStop, SIGNAL(clicked()), this, SLOT(recordToggle()));
    buttonLayout->addWidget(startStop);

    QPushButton * quitButton = new QPushButton("Quit", this);
    connect(quitButton, SIGNAL(clicked()), this, SLOT(quit()));
    buttonLayout->addWidget(quitButton);

    setLayout(wrapperLayout);

    startStop->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    quitButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QFont currentFont = startStop->font();
    currentFont.setPointSize(currentFont.pointSize() + 8);

    startStop->setFont(currentFont);
    quitButton->setFont(currentFont);

    painter = new QPainter(&depthImage);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerCallback()));
    timer->start(15);

#ifdef unix
    char * homeDir = getenv("HOME");
    logFolder.append(homeDir);
    logFolder.append("/");
#else
    char * homeDrive = getenv("HOMEDRIVE");
    char * homeDir = getenv("HOMEPATH");
    logFolder.append(homeDrive);
    logFolder.append("\\");
    logFolder.append(homeDir);
    logFolder.append("\\");
#endif

    logFolder.append("Kinect_Logs");

    boost::filesystem::path p(logFolder.c_str());
    boost::filesystem::create_directory(p);

    logFile->setText(QString::fromStdString(getNextFilename()));
}

MainWindow::~MainWindow()
{
    timer->stop();
    delete logger;
}

std::string MainWindow::getNextFilename()
{
    static char const* const fmt = "%Y-%m-%d";
    std::ostringstream ss;

    ss.imbue(std::locale(std::cout.getloc(), new boost::gregorian::date_facet(fmt)));
    ss << boost::gregorian::day_clock::universal_day();

    std::string dateFilename;

    if(!lastFilename.length())
    {
        dateFilename = ss.str();
    }
    else
    {
        dateFilename = lastFilename;
    }

    std::string currentFile;

    int currentNum = 0;

    while(true)
    {
        std::stringstream strs;
        strs << logFolder;
#ifdef unix
        strs << "/";
#else
        strs << "\\";
#endif
        strs << dateFilename << ".";
        strs << std::setfill('0') << std::setw(2) << currentNum;
        strs << ".klg";

        if(!boost::filesystem::exists(strs.str().c_str()))
        {
            return strs.str();
        }

        currentNum++;
    }

    return "";
}

void MainWindow::dateFilename()
{
    lastFilename.clear();
    logFile->setText(QString::fromStdString(getNextFilename()));
}

void MainWindow::fileBrowse()
{
    QString message = "Log file selection";

    QString types = "All files (*)";

    QString fileName = QFileDialog::getSaveFileName(this, message, ".", types);

    if(!fileName.isEmpty())
    {
        if(!fileName.contains(".klg", Qt::CaseInsensitive))
        {
            fileName.append(".klg");
        }

#ifdef unix
        logFolder = fileName.toStdString().substr(0, fileName.toStdString().rfind("/"));
        lastFilename = fileName.toStdString().substr(fileName.toStdString().rfind("/") + 1, fileName.toStdString().rfind(".klg"));
#else
        logFolder = fileName.toStdString().substr(0, fileName.toStdString().rfind("\\"));
        lastFilename = fileName.toStdString().substr(fileName.toStdString().rfind("\\") + 1, fileName.toStdString().rfind(".klg"));
#endif

        lastFilename = lastFilename.substr(0, lastFilename.size() - 4);

        logFile->setText(QString::fromStdString(getNextFilename()));
    }
}

void MainWindow::recordToggle()
{
    if(!recording)
    {
        if(logFile->text().length() == 0)
        {
            QMessageBox::information(this, "Information", "You have not selected an output log file");
        }
        else
        {
            logger->startWriting(logFile->text().toStdString());
            startStop->setText("Stop");
            recording = true;
        }
    }
    else
    {
        logger->stopWriting();
        startStop->setText("Record");
        recording = false;
        logFile->setText(QString::fromStdString(getNextFilename()));
    }
}

void MainWindow::quit()
{
    if(QMessageBox::question(this, "Quit?", "Are you sure you want to quit?", "&No", "&Yes", QString::null, 0, 1 ))
    {
        if(recording)
        {
            recordToggle();
        }
        this->close();
    }
}

void MainWindow::timerCallback()
{
    int lastDepth = logger->latestDepthIndex.getValue();

    if(lastDepth == -1)
    {
        return;
    }

    int bufferIndex = lastDepth % 10;

    if(bufferIndex == lastDrawn)
    {
        return;
    }

    if(lastFrameTime == logger->frameBuffers[bufferIndex].second)
    {
        return;
    }

    memcpy(&depthBuffer[0], logger->frameBuffers[bufferIndex].first.first, 640 * 480 * 2);
    memcpy(rgbImage.bits(), logger->frameBuffers[bufferIndex].first.second, 640 * 480 * 3);

    cv::Mat1w depth(480, 640, (unsigned short *)&depthBuffer[0]);
    normalize(depth, tmp, 0, 255, cv::NORM_MINMAX, 0);

    cv::Mat3b depthImg(480, 640, (cv::Vec<unsigned char, 3> *)depthImage.bits());
    cv::cvtColor(tmp, depthImg, CV_GRAY2RGB);

    painter->setPen(recording ? Qt::red : Qt::green);
    painter->setFont(QFont("Arial", 30));
    painter->drawText(10, 50, recording ? "Recording" : "Viewing");

    frameStats.push_back(abs(logger->frameBuffers[bufferIndex].second - lastFrameTime));

    if(frameStats.size() > 15)
    {
        frameStats.erase(frameStats.begin());
    }

    int64_t speedSum = 0;

    for(unsigned int i = 0; i < frameStats.size(); i++)
    {
        speedSum += frameStats[i];
    }

    int64_t avgSpeed = (float)speedSum / (float)frameStats.size();

    float fps = 1.0f / ((float)avgSpeed / 1000000.0f);

    fps = floor(fps * 10.0f);

    fps /= 10.0f;

    std::stringstream str;
    str << fps << "fps";

    lastFrameTime = logger->frameBuffers[bufferIndex].second;

    painter->setFont(QFont("Arial", 24));
    painter->drawText(10, 455, QString::fromStdString(str.str()));

    depthLabel->setPixmap(QPixmap::fromImage(depthImage));
    imageLabel->setPixmap(QPixmap::fromImage(rgbImage));
}
