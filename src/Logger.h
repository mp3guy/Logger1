/*
 * Logger.h
 *
 *  Created on: 15 Jun 2012
 *      Author: thomas
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <zlib.h>

#include <limits>
#include <cassert>
#include <iostream>

#include <opencv2/opencv.hpp>

#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/condition_variable.hpp>

#include "OpenNI/openni_device.h"
#include "OpenNI/openni_driver.h"
#include "OpenNI/openni_exception.h"
#include "OpenNI/openni_depth_image.h"
#include "OpenNI/openni_image.h"

#include "ThreadMutexObject.h"

class Logger
{
    public:
        Logger();
        virtual ~Logger();

        void startWriting(std::string filename);
        void stopWriting();

        std::pair<std::pair<uint8_t *, uint8_t *>, int64_t> frameBuffers[10];
        ThreadMutexObject<int> latestDepthIndex;

    private:
        std::pair<uint8_t *, int64_t> imageBuffers[10];
        ThreadMutexObject<int> latestImageIndex;

        boost::shared_ptr<openni_wrapper::OpenNIDevice> m_device;
        int64_t m_lastImageTime;
        int64_t m_lastDepthTime;
        int depth_compress_buf_size;
        uint8_t * depth_compress_buf;
        CvMat * encodedImage;

        int lastWritten;
        boost::thread * writeThread;
        ThreadMutexObject<bool> writing;
        std::string filename;

        void setupDevice(const std::string & deviceId);
        void startSynchronization();
        void stopSynchronization();

        void encodeJpeg(cv::Vec<unsigned char, 3> * rgb_data);
        void imageCallback(boost::shared_ptr<openni_wrapper::Image> image, void * cookie);
        void depthCallback(boost::shared_ptr<openni_wrapper::DepthImage> depth_image, void * cookie);

        void writeData();
};

#endif /* LOGGER_H_ */
