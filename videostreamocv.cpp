#include "videostreamocv.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <QDebug>
#include <QAtomicInt>
#include <QCoreApplication>
#include <QMap>

VideoStreamOCV::VideoStreamOCV(QObject *parent) :
    QObject(parent),
    m_stopStreaming(false)
{

}

VideoStreamOCV::~VideoStreamOCV() {
    qDebug() << "Closing video stream";
    if (cam->isOpened())
        cam->release();
}

void VideoStreamOCV::setCameraID(int cameraID)  {
    m_cameraID = cameraID;
}

void VideoStreamOCV::setBufferParameters(cv::Mat *buf, int bufferSize, QSemaphore *freeFramesS, QSemaphore *usedFramesS, QAtomicInt *acqFrameNum){
    buffer = buf;
    frameBufferSize = bufferSize;
    freeFrames = freeFramesS;
    usedFrames = usedFramesS;
    m_acqFrameNum = acqFrameNum;
}

void VideoStreamOCV::startStream()
{
    int idx = 0;
    cv::Mat frame;
    cam = new cv::VideoCapture;


    m_stopStreaming = false;
    cam->open(m_cameraID);
//    cam->set(cv::CAP_PROP_CONTRAST, 65535);
    if (cam->isOpened()) {
        m_isStreaming = true;
        forever {

            if (m_stopStreaming == true) {
                m_isStreaming = false;
                break;
            }
            cam->grab();
            //            freeFrames->acquire();
            cam->retrieve(frame);
            buffer[idx%frameBufferSize] = frame;
            m_acqFrameNum->operator++();
            idx++;
            usedFrames->release();

            // Get any new events
            QCoreApplication::processEvents(); // Is there a better way to do this. This is against best practices
            sendCommands(); // Send last of each control property events that arrived on this processEvent() call then removes it from queue
        }
        cam->release();
    }
    else {
        qDebug() << "Camera " << m_cameraID << " failed to open.";
    }
}

void VideoStreamOCV::stopSteam()
{
    m_stopStreaming = true;
}

void VideoStreamOCV::setPropertyI2C(unsigned int preamble, unsigned int data)
{
    // add newEvent to the queue for sending new settings to camera
    // overwrites data of previous preamble event that has not been sent to camera yet
    sendCommandQueue[preamble] = data;
}

void VideoStreamOCV::sendCommands()
{
    QList<unsigned int> keys = sendCommandQueue.keys();
    qDebug() << "New Loop";
    for (int i = 0; i < keys.size(); i++) {
        qDebug() << "preamble: 0x" << QString::number(keys[i],16) << ". data: 0x" << QString::number(sendCommandQueue[keys[i]], 16);
        cam->set(cv::CAP_PROP_SHARPNESS, keys[i]); // send preamble
        cam->set(cv::CAP_PROP_CONTRAST, sendCommandQueue[keys[i]]); // send data
        sendCommandQueue.remove(keys[i]);
    }

}