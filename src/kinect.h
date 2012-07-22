#ifndef KINECT_H
#define KINECT_H

#include "libfreenect.hpp"
#include "libfreenect_sync.h"
#include "opencv2/opencv.hpp"
#include "boost/thread.hpp"

//using namespace boost::interprocess;

class kinect : public Freenect::FreenectDevice
{
public:
    /***********/
    // Méthodes
    /***********/
    kinect(freenect_context* pCtx, int pIndex);

    // Défini les déformations du couple rgb/z-cam à corriger
    bool setCalibration(const char* pFile);

    // Récupération de l'image RGB
    cv::Mat getRGB();

    // Récupération de la profondeur
    cv::Mat getDepthmap();

    // Callback lors de la capture d'une image rgb
    void VideoCallback(void *pRgb, uint32_t pTime);

    // Callback lors de la capture de la profondeur
    void DepthCallback(void *pDepth, uint32_t pTime);

private:
    /***********/
    // Attributs
    /***********/
    freenect_resolution mResolution;
    freenect_video_format mVideoFormat;
    freenect_depth_format mDepthFormat;

    boost::mutex mMutex;
    boost::thread mThread; // Thread de mise à jour des paramètres

    // Buffers divers
    cv::Mat mRgbFrontBuffer;
    cv::Mat mDepthFrontBuffer;

    // Matrices de rectification des images
    bool mIsCalibrated;
    cv::Mat mRectifyLeft1, mRectifyLeft2;
    cv::Mat mRectifyRight1, mRectifyRight2;

    // Décalage dans la reconstruction
    int mDepthShift;

    /**********/
    // Méthodes
    /**********/
};

#endif // KINECT_H
