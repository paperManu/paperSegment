#include "kinect.h"

using namespace std;
using namespace cv;
using namespace Freenect;

/**************************/
kinect::kinect(freenect_context *pCtx, int pIndex)
    :FreenectDevice(pCtx, pIndex),
      mResolution(FREENECT_RESOLUTION_MEDIUM),
      mVideoFormat(FREENECT_VIDEO_RGB),
      mDepthFormat(FREENECT_DEPTH_11BIT),
      mIsCalibrated(false),
      mDepthShift(8)
{
    mRgbFrontBuffer = cv::Mat::zeros(480, 640, CV_8UC3);
    mDepthFrontBuffer = cv::Mat::zeros(480, 640, CV_16UC1);
}

/**************************/
bool kinect::setCalibration(const char *pFile)
{
    cv::FileStorage lFile;
    lFile.open(pFile, cv::FileStorage::READ);

    if(!lFile.isOpened())
    {
        std::cerr << "Error while opening calibration file." << std::endl;
        mIsCalibrated = false;
        return false;
    }

    cv::Mat lLeftMat, lLeftDist;
    cv::Mat lRightMat, lRightDist;
    cv::Mat lRotation, lTranslation;
    cv::Size lSize(640, 480);

    lFile["Camera_Left_Matrix"] >> lLeftMat;
    lFile["Camera_Left_Distortion"] >> lLeftDist;
    lFile["Camera_Right_Matrix"] >> lRightMat;
    lFile["Camera_Right_Distortion"] >> lRightDist;
    lFile["Camera_Rotation"] >> lRotation;
    lFile["Camera_Translation"] >> lTranslation;

    cv::Mat lR1, lR2, lP1, lP2, lQ;

    try
    {
        cv::stereoRectify(lLeftMat, lLeftDist, lRightMat, lRightDist,
                          lSize, lRotation, lTranslation,
                          lR1, lR2, lP1, lP2, lQ, cv::CALIB_ZERO_DISPARITY);
        cv::initUndistortRectifyMap(lLeftMat, lLeftDist, lR1, lP1, lSize,
                                    CV_32FC1, mRectifyLeft1, mRectifyLeft2);
        cv::initUndistortRectifyMap(lRightMat, lRightDist, lR2, lP2, lSize,
                                    CV_32FC1, mRectifyRight1, mRectifyRight2);
    }
    catch(...)
    {
        std::cerr << "Error while computing projection maps." << endl;
    }

    mIsCalibrated = true;

    return true;
}

/**************************/
cv::Mat kinect::getRGB()
{
    cv::Mat lMat, lRemat;

    // Récupération de la dernière image capturée    
    mMutex.lock();
    lMat = mRgbFrontBuffer.clone();
    mMutex.unlock();

    // Correction de sa géométrie
    if(mIsCalibrated)
    {
        cv::remap(lMat, lRemat, mRectifyLeft1, mRectifyLeft2, cv::INTER_AREA);
        return lRemat;
    }
    else
    {
        return lMat;
    }
}

/**************************/
cv::Mat kinect::getDepthmap()
{
    cv::Mat lMat, lRemat;

    mMutex.lock();
    lMat = mDepthFrontBuffer.clone();
    mMutex.unlock();

    // Correction de sa géométrie
    if(mIsCalibrated)
    {
        cv::remap(lMat, lRemat, mRectifyRight1, mRectifyRight2, cv::INTER_NEAREST);
        // On décale tout de 8 pixels vers la gauche, du fait de la méthode
        // utilisée par le Kinect pour créer la map de profondeur
        lMat.setTo(65536);
        lRemat(cv::Rect(mDepthShift, 0, 640-mDepthShift, 480)).copyTo(lMat(cv::Rect(0,0, 640-mDepthShift, 480)));
        return lMat;
    }
    else
    {
        return lMat;
    }
}

/**************************/
void kinect::VideoCallback(void *pRgb, uint32_t pTime)
{    
    mMutex.lock();

    memcpy(mRgbFrontBuffer.data, pRgb, 640*480*3*sizeof(unsigned char));

    mMutex.unlock();
}

/**************************/
void kinect::DepthCallback(void *pDepth, uint32_t pTime)
{
    mMutex.lock();

    memcpy(mDepthFrontBuffer.data, pDepth, 640*480*sizeof(unsigned short));

    mMutex.unlock();
}
