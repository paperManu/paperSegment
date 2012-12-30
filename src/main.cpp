#include <iostream>
#include <thread>
#include <chrono>

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
    bool lShow = false;

    if(argc > 1)
    {
        for(int i=1; i<argc; i++)
        {
            if(strcmp(argv[i], "--record") == 0)
                lRecording = true;
            else if(strcmp(argv[i], "--show") == 0)
                lShow = true;
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
    lZSegment.setFGSmoothing(3);

    gmm lBGGmm, lFGGmm;
    lBGGmm.setClusterCount(3);
    lBGGmm.setEMMinLikelihood(0.01f);
    lBGGmm.setMaxEMLoop(30);
    lBGGmm.setMaxCost(100);

    lFGGmm.setClusterCount(3);
    lFGGmm.setEMMinLikelihood(0.01f);
    lFGGmm.setMaxEMLoop(100);
    lFGGmm.setMaxCost(100);

    seed lSeed;
    lSeed.setMinimumSize(128);
    lSeed.setDilatationSize(8);

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

    int frameNumber = 0;

    long int grabDuration, presegmentDuration, gmmDuration, segDuration, totalDuration;

    while(1)
    {
        grabDuration = presegmentDuration = gmmDuration = segDuration = totalDuration = 0;

        auto startTime = chrono::high_resolution_clock::now();

        lRGB = lKinect->getRGB();
        lDepth = lKinect->getDepthmap();

        auto grabTime = chrono::high_resolution_clock::now();
        grabDuration = chrono::duration_cast<chrono::microseconds>(grabTime - startTime).count();

        if(lSeedNbr < 90)
        {
            lZSegment.feedStdDevEval(lDepth);
            lSeedNbr++;
        }
        else if(lInitBG == false)
        {
            //lZSegment.computeStdDev();
            lZSegment.setStdDev(16);
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

            if (lShow)
                cv::imshow("seed", lPresegment);

            auto presegmentTime = chrono::high_resolution_clock::now();
            presegmentDuration = chrono::duration_cast<chrono::microseconds>(presegmentTime - grabTime).count();

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
                cv::Mat lBGCosts, lFGCosts;
                thread firstGMM([&] ()
                {
                    lBGGmm.setRgbImg(lRGB);
                    lBGGmm.calcGmm(lSeeds[0].background);
                    lBGCosts = lBGGmm.getCosts(lSeeds[0].unknown);
                } );
                
                thread secondGMM([&] ()
                {
                    lFGGmm.setRgbImg(lRGB);
                    lFGGmm.calcGmm(lSeeds[0].foreground);
                    lFGCosts = lFGGmm.getCosts(lSeeds[0].unknown);
                } );

                firstGMM.join();
                secondGMM.join();

                auto gmmTime = chrono::high_resolution_clock::now();
                gmmDuration = chrono::duration_cast<chrono::microseconds>(gmmTime - presegmentTime).count();

                lColorSegment.setCosts(lRGB, lBGCosts, lFGCosts, lSeeds[0].background+lSeeds[0].mask, lSeeds[0].foreground,
                                       lSeeds[0].x_min, lSeeds[0].x_max, 480-lSeeds[0].y_max, 480-lSeeds[0].y_min);

                if(lColorSegment.getSegment(lSegment))
                {
                    if (lShow)
                    {
                        cv::flip(lSegment, lSegment, 0);
                        cv::imshow("segment!", lSegment);
                    }

                    auto segTime = chrono::high_resolution_clock::now();
                    segDuration = chrono::duration_cast<chrono::microseconds>(segTime - gmmTime).count();

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

        auto endTime = chrono::high_resolution_clock::now();
        totalDuration = chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
        int size = 0;
        float ratio = 0.f;
        lColorSegment.getInfos(size, ratio);

        std::cout << frameNumber << " " << grabDuration/1000 << " " << presegmentDuration/1000
            << " " << gmmDuration/1000 << " " << segDuration/1000 << " " << totalDuration/1000
            << " " << size << " " << ratio << std::endl << std::flush;

        if (lShow)
        {
            lDepth *= 32;
            cv::imshow("depth", lDepth);
        }

        char lKey = cvWaitKey(5);
        if(lKey == 'q')
            break;
        else if(lKey == 'c')
        {
            lCalibrate = !lCalibrate;
        }

        //usleep(10000);
        frameNumber++;
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

