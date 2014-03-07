/*
 * main.h
 *
 *  Created on: 21 Jul 2012
 *      Author: thomas
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <QImage>
#include <QApplication>
#include <QString>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <qmessagebox.h>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QTextEdit>
#include <QComboBox>
#include <QMouseEvent>
#include <qlineedit.h>
#include <QPushButton>
#include <QFileDialog>
#include <QPainter>

#include <locale>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <iomanip>
#include <iostream>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem.hpp>

#include "Logger.h"

class MainWindow : public QWidget
{
    Q_OBJECT;

    public:
        MainWindow(Logger * logger);
        virtual ~MainWindow();

    private slots:
        void timerCallback();
        void recordToggle();
        void quit();
        void fileBrowse();
        void dateFilename();

    private:
        Logger * logger;
        QImage depthImage;
        QImage rgbImage;
        bool recording;
        QPushButton * startStop;
        QPushButton * browseButton;
        QPushButton * dateNameButton;
        QLabel * logFile;
        unsigned short depthBuffer[640 * 480 * 2];
        QLabel * depthLabel;
        QLabel * imageLabel;
        QTimer * timer;
        cv::Mat1b tmp;
        QPainter * painter;
        int lastDrawn;

        std::vector<int64_t> frameStats;
        int64_t lastFrameTime;

        std::string logFolder;
        std::string lastFilename;
        std::string getNextFilename();
};

#endif /* MAIN_H_ */
