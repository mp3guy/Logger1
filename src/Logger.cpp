/*
 * Logger.cpp
 *
 *  Created on: 15 Jun 2012
 *      Author: thomas
 */

#include "Logger.h"

Logger::Logger()
 : lastWritten(-1),
   writeThread(0)
{
    std::string deviceId = "#1";

    depth_compress_buf_size = 640 * 480 * sizeof(int16_t) * 4;
    depth_compress_buf = (uint8_t*)malloc(depth_compress_buf_size);

    encodedImage = 0;

    writing.assignValue(false);

    latestDepthIndex.assignValue(-1);
    latestImageIndex.assignValue(-1);

    for(int i = 0; i < 10; i++)
    {
        uint8_t * newImage = (uint8_t *)calloc(640 * 480 * 3, sizeof(uint8_t));
        imageBuffers[i] = std::pair<uint8_t *, int64_t>(newImage, 0);
    }

    for(int i = 0; i < 10; i++)
    {
        uint8_t * newDepth = (uint8_t *)calloc(640 * 480 * 2, sizeof(uint8_t));
        uint8_t * newImage = (uint8_t *)calloc(640 * 480 * 3, sizeof(uint8_t));
        frameBuffers[i] = std::pair<std::pair<uint8_t *, uint8_t *>, int64_t>(std::pair<uint8_t *, uint8_t *>(newDepth, newImage), 0);
    }

    setupDevice(deviceId);
}

Logger::~Logger()
{
    if(m_device)
    {
        m_device->stopDepthStream();
        m_device->stopImageStream();
    }

    free(depth_compress_buf);

    writing.assignValue(false);

    writeThread->join();

    if(encodedImage != 0)
    {
        cvReleaseMat(&encodedImage);
    }

    for(int i = 0; i < 10; i++)
    {
        free(imageBuffers[i].first);
    }

    for(int i = 0; i < 10; i++)
    {
        free(frameBuffers[i].first.first);
        free(frameBuffers[i].first.second);
    }
}


void Logger::setupDevice(const std::string & deviceId)
{
    m_device = boost::shared_ptr<openni_wrapper::OpenNIDevice > ((openni_wrapper::OpenNIDevice*)NULL);

    openni_wrapper::OpenNIDriver & driver = openni_wrapper::OpenNIDriver::getInstance();

    do
    {
        driver.updateDeviceList();

        if(driver.getNumberDevices() == 0)
        {
            std::cout << "No devices connected.... waiting for devices to be connected" << std::endl;
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            continue;
        }

        std::cout << boost::format("Number devices connected: %d") % driver.getNumberDevices() << std::endl;
        for(unsigned deviceIdx = 0; deviceIdx < driver.getNumberDevices(); ++deviceIdx)
        {
            std::cout << boost::format("  %u. device on bus %03i:%02i is a %s (%03X) from %s (%03X) with serial id \'%s\'")
                         % (deviceIdx + 1)
                         % (int) driver.getBus(deviceIdx)
                         % (int) driver.getAddress(deviceIdx)
                         % std::string(driver.getProductName(deviceIdx))
                         % driver.getProductID(deviceIdx)
                         % std::string(driver.getVendorName(deviceIdx))
                         % driver.getVendorID(deviceIdx)
                         % std::string(driver.getSerialNumber(deviceIdx))
                         << std::endl;
        }

        try
        {
            if(deviceId[0] == '#')
            {
                unsigned int index = boost::lexical_cast<unsigned int>(deviceId.substr(1));
                std::cout << boost::format("searching for device with index = %d") % index << std::endl;
                m_device = driver.getDeviceByIndex(index - 1);
                break;
            }
        }
        catch (const openni_wrapper::OpenNIException& exception)
        {
            if(!m_device)
            {
                std::cout << boost::format("No matching device found.... waiting for devices. Reason: %s") % exception.what() << std::endl;
				boost::this_thread::sleep(boost::posix_time::seconds(1));
                continue;
            }
            else
            {
                std::cout << boost::format("could not retrieve device. Reason %s") % exception.what() << std::endl;
                exit(-1);
            }
        }
    } while(!m_device);

    std::cout << boost::format("Opened '%s' on bus %i:%i with serial number '%s'")
                 % m_device->getProductName()
                 % (int) m_device->getBus()
                 % (int) m_device->getAddress()
                 % m_device->getSerialNumber()
                 << std::endl;

    m_device->registerImageCallback(&Logger::imageCallback, *this);
    m_device->registerDepthCallback(&Logger::depthCallback, *this);

    m_device->depth_generator_.GetAlternativeViewPointCap().SetViewPoint(m_device->image_generator_);

    m_device->startImageStream();
    m_device->startDepthStream();
    startSynchronization();
}

void Logger::startSynchronization()
{
    if(m_device->isSynchronizationSupported() &&
       !m_device->isSynchronized() &&
       m_device->getImageOutputMode().nFPS == m_device->getDepthOutputMode().nFPS &&
       m_device->isImageStreamRunning() &&
       m_device->isDepthStreamRunning())
    {
        m_device->setSynchronization(true);
    }
}

void Logger::stopSynchronization()
{
    if(m_device->isSynchronizationSupported() && m_device->isSynchronized())
    {
        m_device->setSynchronization(false);
    }
}

void Logger::encodeJpeg(cv::Vec<unsigned char, 3> * rgb_data)
{
    cv::Mat3b rgb(480, 640, rgb_data, 1920);

    IplImage * img = new IplImage(rgb);

    int jpeg_params[] = {CV_IMWRITE_JPEG_QUALITY, 90, 0};

    if(encodedImage != 0)
    {
        cvReleaseMat(&encodedImage);
    }

    encodedImage = cvEncodeImage(".jpg", img, jpeg_params);

    delete img;
}

void Logger::imageCallback(boost::shared_ptr<openni_wrapper::Image> image, void * cookie)
{
	boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration duration(time.time_of_day());
	m_lastImageTime = duration.total_microseconds();

    int bufferIndex = (latestImageIndex.getValue() + 1) % 10;

    image->fillRGB(image->getWidth(), image->getHeight(), reinterpret_cast<unsigned char*>(imageBuffers[bufferIndex].first), 640 * 3);

    imageBuffers[bufferIndex].second = m_lastImageTime;

    latestImageIndex++;
}

void Logger::depthCallback(boost::shared_ptr<openni_wrapper::DepthImage> depth_image, void * cookie)
{
	boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration duration(time.time_of_day());
	m_lastDepthTime = duration.total_microseconds();
    
	int bufferIndex = (latestDepthIndex.getValue() + 1) % 10;

    depth_image->fillDepthImageRaw(depth_image->getWidth(), depth_image->getHeight(), reinterpret_cast<unsigned short *>(frameBuffers[bufferIndex].first.first), 640 * 2);

    frameBuffers[bufferIndex].second = m_lastDepthTime;

    int lastImageVal = latestImageIndex.getValue();

    if(lastImageVal == -1)
    {
        return;
    }

    lastImageVal %= 10;

    memcpy(frameBuffers[bufferIndex].first.second, imageBuffers[lastImageVal].first, 640 * 480 * 3);

    latestDepthIndex++;
}

void Logger::startWriting(std::string filename)
{
    assert(!writeThread && !writing.getValue());

    this->filename = filename;

    writing.assignValue(true);

    writeThread = new boost::thread(boost::bind(&Logger::writeData,
                                               this));
}

void Logger::stopWriting()
{
    assert(writeThread && writing.getValue());

    writing.assignValue(false);

    writeThread->join();

    writeThread = 0;
}

void Logger::writeData()
{
    /**
     * int32_t at file beginning for frame count
     */
    FILE * logFile = fopen(filename.c_str(), "wb+");

    int32_t numFrames = 0;

    fwrite(&numFrames, sizeof(int32_t), 1, logFile);

    while(writing.getValueWait(1))
    {
        int lastDepth = latestDepthIndex.getValue();

        if(lastDepth == -1)
        {
            continue;
        }

        int bufferIndex = lastDepth % 10;

        if(bufferIndex == lastWritten)
        {
            continue;
        }

        unsigned long compressed_size = depth_compress_buf_size;
        boost::thread_group threads;

        threads.add_thread(new boost::thread(compress2,
                                             depth_compress_buf,
                                             &compressed_size,
                                             (const Bytef*)frameBuffers[bufferIndex].first.first,
                                             640 * 480 * sizeof(short),
                                             Z_BEST_SPEED));

        threads.add_thread(new boost::thread(boost::bind(&Logger::encodeJpeg,
                                                         this,
                                                         (cv::Vec<unsigned char, 3> *)frameBuffers[bufferIndex].first.second)));

        threads.join_all();

        int32_t depthSize = compressed_size;
        int32_t imageSize = encodedImage->width;

        /**
         * Format is:
         * int64_t: timestamp
         * int32_t: depthSize
         * int32_t: imageSize
         * depthSize * unsigned char: depth_compress_buf
         * imageSize * unsigned char: encodedImage->data.ptr
         */

        fwrite(&frameBuffers[bufferIndex].second, sizeof(int64_t), 1, logFile);
        fwrite(&depthSize, sizeof(int32_t), 1, logFile);
        fwrite(&imageSize, sizeof(int32_t), 1, logFile);
        fwrite(depth_compress_buf, depthSize, 1, logFile);
        fwrite(encodedImage->data.ptr, imageSize, 1, logFile);

        numFrames++;

        lastWritten = bufferIndex;
    }

    fseek(logFile, 0, SEEK_SET);
    fwrite(&numFrames, sizeof(int32_t), 1, logFile);

    fflush(logFile);
    fclose(logFile);
}
