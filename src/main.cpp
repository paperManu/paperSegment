#include <iostream>
#include "opencv2/opencv.hpp"
#include "boost/lexical_cast.hpp"

#include "kinect.h"
#include "zsegment.h"
#include "gmm.h"
#include "seed.h"
#include "colorsegment.h"

using namespace std;

int main(int argc, char** argv)
{
    bool lRecording = false;

    if(argc > 1)
    {
        for(int i=1; i<argc; i++)
        {
            if(strcmp(argv[i], "--record") == 0)
                lRecording = true;
        }
    }

    cerr << "Starting..." << endl;

    // Lancement du kinect
    Freenect::Freenect lFreenect;
    kinect* lKinect = NULL;
    try
    {
        lKinect = &lFreenect.createDevice<kinect>(0);
        if(lKinect == NULL)
            return 1;
    }
    catch(...)
    {
        cerr << "Problem while allocating Kinect device." << endl;
        return 1;
    }

    lKinect->setCalibration("./calibration_stereo.xml");

    lKinect->startVideo();
    lKinect->startDepth();

    lKinect->setVideoFormat(FREENECT_VIDEO_RGB);
    lKinect->setDepthFormat(FREENECT_DEPTH_11BIT);

    //lKinect->setRecording(lRecording);

    // Allocation des différents objets
    zSegment lZSegment;
    lZSegment.setMax(2000);
    lZSegment.setFGSmoothing(5);

    gmm lGmm;
    lGmm.setClusterCount(3);
    lGmm.setEMMinLikelihood(0.01f);
    lGmm.setMaxCost(100);

    seed lSeed;
    lSeed.setMinimumSize(128);
    lSeed.setDilatationSize(16);

    colorSegment lColorSegment;
    lColorSegment.init(640, 480);
    lColorSegment.setMaxSmoothCost(50);

    cv::Mat lRGB;
    cv::Mat lDepth;
    cv::Mat lSegment;

    bool lCalibrate = false;
    bool lInitBG = false;
    bool lIsBG = false;
    int lSeedNbr = 0;
    int lBGNbr = 0;

    cerr << "Capturing..." << endl;
    while(1)
    {
        lRGB = lKinect->getRGB();
        lDepth = lKinect->getDepthmap();

        if(lSeedNbr < 90)
        {
            lZSegment.feedStdDevEval(lDepth);
            lSeedNbr++;
        }
        else if(lInitBG == false)
        {
            //lZSegment.computeStdDev();
            lZSegment.setStdDev(5);
            lInitBG = true;
        }

        if(lInitBG == true && lBGNbr < 60)
        {
            lZSegment.feedBackground(lDepth);
            lBGNbr++;
        }
        else if(lInitBG == true && !lIsBG)
        {
            lZSegment.computeBackground();
            lIsBG = true;

            std::cerr << "Background initialized." << std::endl;
        }

        if(lIsBG == true)
        {
            lZSegment.feedImage(lDepth);
            lSeed.setRoughSegment(lZSegment.getBackground(),
                                  lZSegment.getForeground(),
                                  lZSegment.getUnknown());

            cv::Mat lPresegment = lZSegment.getBackground() + lZSegment.getForeground()/2;
            cv::imshow("seed", lPresegment);

            if(lRecording)
            {
                std::string lName = "./grab/preSegment_";
                lName += boost::lexical_cast<std::string>(time(NULL));
                lName += std::string(".png");
                cv::imwrite(lName, lPresegment);
            }

            std::vector<seedObject> lSeeds = lSeed.getSeeds();
            if(lSeeds.size() > 0)
            {
                // Calcul de la mixture de gaussienne pour la
                // plus grosse graîne
                lGmm.setRgbImg(lRGB);
                lGmm.calcGmm(lSeeds[0].background);
                cv::Mat lBGCosts = lGmm.getCosts(lSeeds[0].unknown);
                lGmm.calcGmm(lSeeds[0].foreground);
                cv::Mat lFGCosts = lGmm.getCosts(lSeeds[0].unknown);

                lColorSegment.setCosts(lRGB, lBGCosts, lFGCosts, lSeeds[0].background+lSeeds[0].mask, lSeeds[0].foreground,
                                       lSeeds[0].x_min, lSeeds[0].x_max, 480-lSeeds[0].y_max, 480-lSeeds[0].y_min);

                if(lColorSegment.getSegment(lSegment))
                {
                    cv::flip(lSegment, lSegment, 0);
                    cv::imshow("segment!", lSegment);

                    if(lRecording)
                    {
                        std::string lName = "./grab/segment_";
                        lName += boost::lexical_cast<std::string>(time(NULL));
                        lName += std::string(".png");
                        cv::imwrite(lName, lSegment);
                    }
                }
            }
        }

        lDepth *= 32;
        cv::imshow("depth", lDepth);

        char lKey = cvWaitKey(5);
        if(lKey == 'q')
            break;
        else if(lKey == 'c')
        {
            lCalibrate = !lCalibrate;
        }
        usleep(10000);
    }

    cerr << "Stopping..." << endl;

    cv::destroyAllWindows();

    try
    {
        lKinect->stopDepth();
        lKinect->stopVideo();
    }
    catch(...)
    {
        cerr << "Problem while closing Kinect." << endl;
    }

    return 0;
}

