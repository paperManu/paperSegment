#include <thread>
#include <atomic>

#include "gmm.h"
#include "math.h"

#define __THREAD_COUNT__ 4

using namespace std;

/***********************/
gmm::gmm()
    :mIsGmm(false),
      mClusterCount(3),
      mEMLikelihood(1e-1),
      mMaxEMLoop(10),
      mMaxCost(10)
{
}

/***********************/
gmm::~gmm()
{
}

/***********************/
void gmm::setClusterCount(int pCount)
{
    if(pCount < 1)
        return;

    mClusterCount = pCount;
}

/***********************/
void gmm::setEMMinLikelihood(float pLikelihood)
{
    if(pLikelihood <= 0.f)
        return;

    mEMLikelihood = pLikelihood;
}

/***********************/
void gmm::setMaxEMLoop(unsigned int pLoop)
{
    mMaxEMLoop = max(1u, pLoop);
}

/***********************/
void gmm::setMaxCost(int pCost)
{
    if(pCost <= 0)
        return;

    mMaxCost = pCost;
}

/***********************/
void gmm::setRgbImg(cv::Mat &pImg)
{
    if(pImg.rows == 0 || pImg.cols == 0)
        return;

    // On veut du RGB !
    if(pImg.type() != CV_8UC3)
        return;

    mColorImg = pImg.clone();

    // Conversion en HSV
    cv::cvtColor(mColorImg, mColorImg, CV_BGR2HSV);

    // La mixture n'est plus la bonne
    mIsGmm = false;
}

/***********************/
void gmm::calcGmm(cv::Mat &pMask)
{
    if(mColorImg.rows == 0 || mColorImg.cols == 0)
        return;

    if(pMask.rows != mColorImg.rows || pMask.cols != mColorImg.cols)
        return;

    // L'image doit être convertie en matrice Nx1
    cv::Mat lKMeanSource = cv::Mat::zeros(mColorImg.rows*mColorImg.cols, 2, CV_32F);

    cv::MatIterator_<float> lKMeanIt = lKMeanSource.begin<float>();
    cv::MatConstIterator_<cv::Vec3b> lImgIt = mColorImg.begin<cv::Vec3b>();
    cv::MatConstIterator_<uchar> lMaskIt = pMask.begin<uchar>();
    cv::MatConstIterator_<cv::Vec3b> lEnd = mColorImg.end<cv::Vec3b>();

    int lMaskPixels = 0;
    for(; lImgIt < lEnd; lImgIt++, lMaskIt++)
    {
        // Si la zone n'est pas masquée
        if(*lMaskIt > 0)
        {
            *lKMeanIt = (*lImgIt)[0]*2.f;
            *(lKMeanIt+1) = (*lImgIt)[1]/2.55f;

            //std::cerr << *lKMeanIt << " " << *(lKMeanIt+1) << std::endl;

            lMaskPixels++;
            lKMeanIt += 2;
        }
    }
    lKMeanSource.resize((size_t)lMaskPixels);

    // Recherche des clusters, step 1 : kmeans
    cv::Mat lMu;
    cv::Mat lKMeanLabels;

    cv::TermCriteria lCriteria;
    lCriteria.maxCount = 5;
    lCriteria.epsilon = 0.5f;

    cv::kmeans(lKMeanSource, mClusterCount, lKMeanLabels, lCriteria, 2, cv::KMEANS_PP_CENTERS, lMu);

//    cerr << "KMean : " << lMaskPixels << " samples." << endl;
//    cerr << "Mu_H / Mu_S" << endl;
//    for(int i=0; i<mClusterCount; i++)
//    {
//        cerr << lMu.at<float>(i, 0) << " " << lMu.at<float>(i, 1) << endl;
//    }

    // Algo EM sur 2 dimensions
    // Celui intégré à OpenCV ne bosse que sur 1 dimension ...
    // On va d'abord rechercher les écarts type (sigma) correspondant aux centrods calculs par le kmean
    // ainsi que le poids de chacun
    cv::Mat lSigma(mClusterCount, 2, CV_32F);
    cv::Mat lWeight(mClusterCount, 1, CV_32F);

    // Ces pointeurs de threads sont réutilisés tout au long de cette méthode
    std::thread* threads[__THREAD_COUNT__];

    for (int t = 0; t < __THREAD_COUNT__; ++t)
    {
        threads[t] = new std::thread([&, t] ()
        {
            for(int i=t; i<mClusterCount; i+=__THREAD_COUNT__) // Pour chaque centroid
            {
                lSigma.at<float>(i, 0) = 0.f;
                lSigma.at<float>(i, 1) = 0.f;
                atomic<int> lNumber;
                lNumber = 0;

                for(int index=0; index<lMaskPixels; index++)
                {
                    if(lKMeanLabels.at<int>(index) == i)
                    {
                        lSigma.at<float>(i, 0) += (lKMeanSource.at<float>(index, 0)-lMu.at<float>(i, 0))*(lKMeanSource.at<float>(index, 0)-lMu.at<float>(i, 0));
                        lSigma.at<float>(i, 1) += (lKMeanSource.at<float>(index, 1)-lMu.at<float>(i, 1))*(lKMeanSource.at<float>(index, 1)-lMu.at<float>(i, 1));
                        lNumber++;
                    }
                }

                if(lNumber > 0)
                {
                    lSigma.at<float>(i, 0) /= (float)lNumber;
                    lSigma.at<float>(i, 1) /= (float)lNumber;
                }

                lWeight.at<float>(i) = (float)lNumber/(float)(lMaskPixels);
            }
        } );
    }
    for (int t = 0; t < __THREAD_COUNT__; ++t)
    {
        threads[t]->join();
        delete threads[t];
    }

    // On calcule la vraisemblance initiale de cette GM
    float lLikelihood;
    lLikelihood = getLikelihood(lKMeanSource, lMu, lSigma, lWeight);

    // Boucle principale
    // On limite le nombre de tours ...
    unsigned int lCounter = 0;
    bool lContinue = true;
    while(lContinue && lCounter < mMaxEMLoop)
    {
        lCounter++;

        // E-step
        cv::Mat lGamma(lMaskPixels, mClusterCount, CV_32F);

        for (int t = 0; t < __THREAD_COUNT__; ++t)
        {
            threads[t] = new thread([&, t] ()
            {
                for(int index=t; index<lMaskPixels; index+=__THREAD_COUNT__)
                {
                    float lSum = numeric_limits<float>::min();

                    for(int i=0; i<mClusterCount; i++)
                    {
                        lSum += lWeight.at<float>(i)*getGaussian2DValueAt(lKMeanSource.at<float>(index, 0), lKMeanSource.at<float>(index, 1),
                                                                            lMu.at<float>(i, 0), lMu.at<float>(i, 1),
                                                                            lSigma.at<float>(i, 0), lSigma.at<float>(i, 1));
                    }

                    for(int i=0; i<mClusterCount; i++)
                    {
                        lGamma.at<float>(index, i) = 1.f/lSum * lWeight.at<float>(i)*getGaussian2DValueAt(lKMeanSource.at<float>(index, 0), lKMeanSource.at<float>(index, 1),
                                                                            lMu.at<float>(i, 0), lMu.at<float>(i, 1),
                                                                            lSigma.at<float>(i, 0), lSigma.at<float>(i, 1));
                    }
                }
            } );
        }
        for (int t = 0; t < __THREAD_COUNT__; ++t)
        {
            threads[t]->join();
            delete threads[t];
        }

        cv::Mat lN(mClusterCount, 1, CV_32F);
        for(int i=0; i<mClusterCount; i++)
        {
            lN.at<float>(i) = 0.f;
            for(int index=0; index<lMaskPixels; index++)
            {
                lN.at<float>(i) += lGamma.at<float>(index, i);
            }
        }

        // M-step
        cv::Mat lWeightNew(mClusterCount, 1, CV_32F);
        cv::Mat lMuNew(mClusterCount, 2, CV_32F);
        cv::Mat lSigmaNew(mClusterCount, 2, CV_32F);

        for (int t = 0; t < __THREAD_COUNT__; ++t)
        {
            threads[t] = new thread([&, t] ()
            {
                for(int i=t; i<mClusterCount; i+=__THREAD_COUNT__)
                {
                    lWeightNew.at<float>(i) = lN.at<float>(i)/(float)lMaskPixels;
                    lMuNew.at<float>(i, 0) = 0.f;
                    lMuNew.at<float>(i, 1) = 0.f;

                    for(int index=0; index<lMaskPixels; index++)
                    {
                        lMuNew.at<float>(i, 0) += lGamma.at<float>(index, i)*lKMeanSource.at<float>(index, 0);
                        lMuNew.at<float>(i, 1) += lGamma.at<float>(index, i)*lKMeanSource.at<float>(index, 1);
                    }
                    if(lN.at<float>(i) == 0)
                    {
                        lMuNew.at<float>(i, 0) = 0.f;
                        lMuNew.at<float>(i, 1) = 0.f;
                    }
                    else
                    {
                        lMuNew.at<float>(i, 0) /= lN.at<float>(i);
                        lMuNew.at<float>(i, 1) /= lN.at<float>(i);
                    }
                }
            } );
        }
        for (int t = 0; t < __THREAD_COUNT__; ++t)
        {
            threads[t]->join();
            delete threads[t];
        }

        for (int t = 0; t < __THREAD_COUNT__; ++t)
        {
            threads[t] = new thread([&, t] ()
            {
                for(int i=0; i<mClusterCount; i++)
                {
                    lSigmaNew.at<float>(i, 0) = 0.f;
                    lSigmaNew.at<float>(i, 1) = 0.f;

                    for(int index=0; index<lMaskPixels; index++)
                    {
                        lSigmaNew.at<float>(i, 0) += lGamma.at<float>(index, i)*(lKMeanSource.at<float>(index, 0)-lMuNew.at<float>(i, 0))*(lKMeanSource.at<float>(index, 0)-lMuNew.at<float>(i, 0));
                        lSigmaNew.at<float>(i, 1) += lGamma.at<float>(index, i)*(lKMeanSource.at<float>(index, 1)-lMuNew.at<float>(i, 1))*(lKMeanSource.at<float>(index, 1)-lMuNew.at<float>(i, 1));
                    }
                    if(lN.at<float>(i) == 0)
                    {
                        lSigmaNew.at<float>(i, 0) = 0.f;
                        lSigmaNew.at<float>(i, 1) = 0.f;
                    }
                    else
                    {
                        lSigmaNew.at<float>(i, 0) /= lN.at<float>(i);
                        lSigmaNew.at<float>(i, 1) /= lN.at<float>(i);
                    }
                }
            } );
        }
        for (int t = 0; t < __THREAD_COUNT__; ++t)
        {
            threads[t]->join();
            delete threads[t];
        }

        // Vérification de la convergence
        float lLikelihoodNew = getLikelihood(lKMeanSource, lMuNew, lSigmaNew, lWeightNew);

        lMu = lMuNew;
        lSigma = lSigmaNew;
        lWeight = lWeightNew;

        if(abs(lLikelihood - lLikelihoodNew) < mEMLikelihood)
        {
            lContinue = false;
        }

        lLikelihood = lLikelihoodNew;
    }

    // Stockage de la GMM
    if(mGmm.size() < (size_t)mClusterCount)
        mGmm.resize(mClusterCount);

//    cerr << endl << "GMM : Mu_H / Mu_S / Sigma_H / Sigma_S / Weight" << endl;

    for(int i=0; i<mClusterCount; i++)
    {
        mGmm[i].mu[0] = lMu.at<float>(i, 0);
        mGmm[i].mu[1] = lMu.at<float>(i, 1);
        mGmm[i].sigma[0] = lSigma.at<float>(i, 0);
        mGmm[i].sigma[1] = lSigma.at<float>(i, 1);
        mGmm[i].weight = lWeight.at<float>(i);

//        cerr << mGmm[i].mu[0] << " " << mGmm[i].mu[1]
//             << " " << mGmm[i].sigma[0] << " " << mGmm[i].sigma[1]
//             << " " << mGmm[i].weight << endl;
    }

    mIsGmm = true;
}

/***********************/
std::vector<gaussian2D> gmm::getMixture()
{
    return mGmm;
}

/***********************/
cv::Mat gmm::getProbs(cv::Mat &pMask)
{
    cv::Mat lProbs;

    if(pMask.rows != mColorImg.rows || pMask.cols != mColorImg.cols)
        return lProbs;

    if(pMask.type() != CV_8UC1)
        return lProbs;

    if(!mIsGmm)
        return lProbs;

    lProbs = cv::Mat::zeros(mColorImg.rows, mColorImg.cols, CV_32FC1);

    // On calcule le coût de correspondance de chaque pixel au modèle
    // Ce coût est inversement proportionnel à la proba de correspondance
    cv::MatConstIterator_<cv::Vec3b> lImgIt = mColorImg.begin<cv::Vec3b>();
    cv::MatConstIterator_<uchar> lMaskIt = pMask.begin<uchar>();
    cv::MatIterator_<float> lProbsIt = lProbs.begin<float>();

    for(; lImgIt<mColorImg.end<cv::Vec3b>(); lImgIt++, lMaskIt++, lProbsIt++)
    {
        float lProba = 0.f;

        // Si le pixel est masqué, on passe au suivant
        if(*lMaskIt == 0)
            continue;

        // Sinon
        for(int i=0; i<mClusterCount; i++)
        {
            lProba += mGmm[i].weight*getGaussian2DValueAt((*lImgIt)[0]*2.f, (*lImgIt)[1]/2.55f, mGmm[i].mu[0], mGmm[i].mu[1], mGmm[i].sigma[0], mGmm[i].sigma[1]);
        }

        lProba = max(lProba, numeric_limits<float>::min());

        *lProbsIt = lProba;
    }

    return lProbs;
}

/***********************/
cv::Mat gmm::getCosts(cv::Mat &pMask)
{
    cv::Mat lCosts;
    cv::Mat lProbs = getProbs(pMask);

    if(lProbs.rows == 0 || lProbs.cols == 0)
        return lCosts;

    lCosts = cv::Mat::zeros(lProbs.rows, lProbs.cols, CV_16UC1);

    cv::MatConstIterator_<float> lProbsIt = lProbs.begin<float>();
    cv::MatIterator_<short int> lCostsIt = lCosts.begin<short int>();

    for(; lProbsIt<lProbs.end<float>(); lProbsIt++, lCostsIt++)
    {
        if(*lProbsIt == 0.f)
            continue;

        *lCostsIt = (short int)abs(mMaxCost*(-log10f(*lProbsIt)));
    }

    return lCosts;
}

/***********************/
float gmm::getLikelihood(cv::Mat &pData, cv::Mat &pMu, cv::Mat &pSigma, cv::Mat &pWeight)
{
    atomic<float> lLikelihood;
    lLikelihood = 0.f;

    std::thread* threads[__THREAD_COUNT__];
    for (int t = 0; t < __THREAD_COUNT__; ++t)
    {
        threads[t] = new thread([&, t] ()
        {
            for(int index=t; index<pData.size[0]; index+=__THREAD_COUNT__)
            {
                float lLocalHood = 0.f;

                for(int i=0; i<mClusterCount; i++)
                {
                    lLocalHood += pWeight.at<float>(i)*getGaussian2DValueAt(pData.at<float>(index, 0), pData.at<float>(index, 1),
                                                                            pMu.at<float>(i, 0), pMu.at<float>(i, 1),
                                                                            pSigma.at<float>(i, 0), pSigma.at<float>(i, 1));
                }

                if(lLocalHood == 0.f)
                {
                    lLocalHood = numeric_limits<float>::min();
                }

                lLocalHood = log10f(lLocalHood);
                lLikelihood = lLikelihood + lLocalHood;
            }
        } );
    }
    for (int t = 0; t < __THREAD_COUNT__; ++t)
    {
        threads[t]->join();
        delete threads[t];
    }

    lLikelihood = lLikelihood / pData.size[0];

    return lLikelihood;
}

/***********************/
float gmm::getGaussian2DValueAt(float pX, float pY, float pMuX, float pMuY, float pSigmaX, float pSigmaY)
{
    if(pSigmaX == 0 || pSigmaY == 0)
    {
        return numeric_limits<float>::min();
    }

    float lValue = 1.f/(sqrtf(pSigmaX*pSigmaY)*2.f*M_PI) * exp(-(pX-pMuX)*(pX-pMuX)/(2*pSigmaX) - (pY-pMuY)*(pY-pMuY)/(2*pSigmaY));

    return lValue;
}
