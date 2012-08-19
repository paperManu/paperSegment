#include "zsegment.h"

/*************************/
zSegment::zSegment()
    :mMax(2000)
{
    // La map de profondeur est en 16 bits maxi (supporté)
    // On va donc créer un tableau de 16384 valeurs pour contenir
    // la map d'écart type
    mStdDev = cv::Mat::zeros(1, 16384, CV_32F);

    mStructElemErode = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                            cv::Size(9, 9),
                                            cv::Point(5, 5));
}

/*************************/
zSegment::~zSegment()
{
}

/*************************/
void zSegment::setMax(unsigned int pMax)
{
    mMax = (float)pMax;
}

/*************************/
void zSegment::setFGSmoothing(unsigned int pSmooth)
{
    mStructElemErode = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                            cv::Size(pSmooth*2+1, pSmooth*2+1),
                                            cv::Point(pSmooth+1, pSmooth+1));
}

/*************************/
void zSegment::feedStdDevEval(cv::Mat &pImg)
{
    if(pImg.rows == 0 || pImg.cols == 0)
        return;

    if(pImg.type() != CV_16UC1)
        return;

    // La première image de la liste défini la résolution
    // et le nombre de canaux
    if(mStdDevSources.size() == (size_t)0)
    {
        mStdDevRes.x = pImg.cols;
        mStdDevRes.y = pImg.rows;
    }

    cv::Mat lConverted;
    pImg.convertTo(lConverted, CV_32FC1);
    mStdDevSources.push_back(lConverted);
}

/*************************/
void zSegment::computeStdDev()
{
    // TODO: il y a un souci dans le calcul de l'écart type, due sans doute
    // aux pixels mal définis. Il faudrait les mettre de coté avant de calculer
    // l'écart-type ...

    // On vérifie qu'il existe une liste d'images
    float lNbrImg = (float)mStdDevSources.size();
    if(lNbrImg == 0.f)
    {
        mIsStdDev = false;
        return;
    }

    mStdDev.setTo(0);

    // Calcul de la moyenne pour chaque pixel
    cv::Mat lMean = cv::Mat::zeros(mStdDevSources[0].rows, mStdDevSources[0].cols, CV_32FC1);
    for(int i=0; i<(int)lNbrImg; i++)
    {
        lMean += mStdDevSources[i];
    }
    lMean = lMean/lNbrImg;

    // Calcul de l'écart type pour chaque valeur
    cv::Mat lSquareDiff = cv::Mat::zeros(lMean.rows, lMean.cols, CV_32FC1);
    for(int i=0; i<(int)lNbrImg; i++)
    {
        cv::Mat lTmp = cv::Mat::zeros(lMean.rows, lMean.cols, CV_32FC1);

        lTmp = mStdDevSources[i]-lMean;
        cv::pow(lTmp, 2., lTmp);
        lSquareDiff += lTmp;
    }

    // Et on somme sur l'indice correspondant, puis on divise par le nombre de pixel concerné
    cv::Mat lStdDevNbr = cv::Mat::zeros(mStdDev.rows, mStdDev.cols, CV_32FC1);

    cv::MatConstIterator_<float> lMeanIt = lMean.begin<float>();
    cv::MatConstIterator_<float> lSquareIt = lSquareDiff.begin<float>();
    cv::MatConstIterator_<float> lEnd = lSquareDiff.end<float>();

    for(; lSquareIt<lEnd; lSquareIt++, lMeanIt++)
    {
        int lIndex = round(*lMeanIt);
        mStdDev.at<float>(lIndex) += *lSquareIt;
        lStdDevNbr.at<float>(lIndex) += lNbrImg;
    }

    cv::divide(mStdDev, lStdDevNbr, mStdDev);
    cv::sqrt(mStdDev, mStdDev);

    // Maintenant, on comble les zones "vides"
    cv::MatIterator_<float> lIt = mStdDev.begin<float>();
    for(; lIt < mStdDev.end<float>(); lIt++)
    {
        if(*lIt != 0)
            continue;

        float lStartValue = *(lIt-1);

        // Recherche de la prochaine valeur non nulle
        int lNbr = 0;
        cv::MatConstIterator_<float> lNext = lIt;
        for(; lNext < mStdDev.end<float>(); lNext++)
        {
            if(*lNext == 0)
            {
                lNbr++;
                continue;
            }
            else
                break;
        }

        // On rempli linéairement
        int lIndex = 1;
        for(; lIt<lNext; lIt++)
        {
            if(lNext < mStdDev.end<float>())
            {
                *lIt += lStartValue + (*lNext - lStartValue)*lIndex/lNbr;
                lIndex++;
            }
            else
            {
                *lIt = lStartValue;
            }
        }
    }

    mIsStdDev = true;

    // Et on vide la liste des images
    mStdDevSources.clear();
}

/*************************/
void zSegment::setStdDev(int pValue)
{
    // On n'aura pas besoin de ces captures
    mStdDevSources.clear();

    if(pValue < 0)
    {
        mIsStdDev = false;
        return;
    }

    mStdDev.setTo(pValue);
    mIsStdDev = true;
}

/*************************/
void zSegment::feedBackground(cv::Mat &pImg)
{
    if(pImg.rows == 0 || pImg.cols == 0)
        return;

    if(pImg.type() != CV_16UC1)
        return;

    // La première image de la liste défini la résolution
    // et le nombre de canaux
    if(mBackgroundSources.size() == 0)
    {
        mResolution.x = pImg.cols;
        mResolution.y = pImg.rows;
    }

    cv::Mat lConverted;
    pImg.convertTo(lConverted, CV_32FC1);
    mBackgroundSources.push_back(lConverted);
}

/*************************/
void zSegment::computeBackground()
{
    // On vérifie qu'il existe une liste d'images
    float lNbrImg = (float)mBackgroundSources.size();
    if(lNbrImg == 0.f)
    {
        mIsBackground = false;
        return;
    }

    // Et on fait une simple moyenne, en créant un masque des zones peu fiables
    cv::Mat lBit;

    mBackground = cv::Mat::zeros(mBackgroundSources[0].rows, mBackgroundSources[0].cols, CV_32FC1);
    mBgMask = cv::Mat::zeros(mBackgroundSources[0].rows, mBackgroundSources[0].cols, CV_8UC1);
    for(std::vector<cv::Mat>::iterator it=mBackgroundSources.begin();
        it<mBackgroundSources.end(); it++)
    {
        mBackground += *it;

        cv::compare(*it, mMax, lBit, cv::CMP_GT);
        cv::bitwise_or(mBgMask, lBit, mBgMask);
    }
    cv::bitwise_not(mBgMask, mBgMask);

    mBackground = mBackground/lNbrImg;

    mIsBackground = true;
    mBackgroundSources.clear();

    // On définit dès maintenant les dimensions des matrices de sortie
    mSegmentBG = cv::Mat::zeros(mBackground.rows, mBackground.cols, CV_8UC1);
    mSegmentFG = mSegmentBG.clone();
    mSegmentUnknown = mSegmentBG.clone();
}

/*************************/
bool zSegment::feedImage(cv::Mat &pImg)
{
    // On vérifie les dimensions de l'image
    if(pImg.cols == 0 || pImg.rows == 0)
        return false;

    // et son type
    if(pImg.type() != CV_16UC1)
        return false;

    cv::Mat lConverted;
    pImg.convertTo(lConverted, CV_32FC1);

    // Masque des pixels du FG non définis
    cv::Mat lFgMask;
    cv::compare(pImg, mMax, lFgMask, cv::CMP_GT);
    cv::bitwise_not(lFgMask, lFgMask);

    // On met les sorties à zéro
    mSegmentBG.setTo(0);
    mSegmentFG.setTo(0);
    mSegmentUnknown.setTo(255);

    // Calcul de la différence absolue entre l'image et le BG
    cv::Mat lAbsDiff = cv::Mat::zeros(mBackground.rows, mBackground.cols, CV_32FC1);
    cv::absdiff(lConverted, mBackground, lAbsDiff);

    // On parcourt l'image pour comparer la diff absolue à sigma
    cv::MatIterator_<uchar> lBGIt = mSegmentBG.begin<uchar>();
    cv::MatIterator_<uchar> lFGIt = mSegmentFG.begin<uchar>();

    cv::MatConstIterator_<ushort> lImgIt = pImg.begin<ushort>();
    cv::MatConstIterator_<float> lAbsDiffIt = lAbsDiff.begin<float>();
    for(; lImgIt<pImg.end<ushort>();
        lImgIt++, lAbsDiffIt++, lBGIt++, lFGIt++)
    {
        // si diff < 2*sigma
        if(*lAbsDiffIt <= 2*mStdDev.at<float>(*lImgIt))
            *lBGIt = 255;
        // si 3*sigma < diff
        else if(*lAbsDiffIt > 3*mStdDev.at<float>(*lImgIt))
            *lFGIt = 255;
    }

    // On élimine les pixels dont le BG et le FG ne sont pas défini correctement
    cv::bitwise_and(lFgMask, mBgMask, lFgMask);
    cv::bitwise_and(mSegmentBG, lFgMask, mSegmentBG);
    cv::bitwise_and(mSegmentFG, lFgMask, mSegmentFG);

    // On va enfin éroder la graîne du FG pour éliminer les faux positifs
    cv::Mat lErode = cv::Mat(mSegmentFG.rows, mSegmentFG.cols, CV_8UC1);
    cv::erode(mSegmentFG, lErode, mStructElemErode);
    lErode.copyTo(mSegmentFG);

    // La zone inconnue est composée de ce qui n'est ni BG, ni FG
    mSegmentUnknown = mSegmentUnknown - (mSegmentBG + mSegmentFG);

    return true;
}

/*************************/
cv::Mat zSegment::getBackground()
{
    return mSegmentBG.clone();
}

/*************************/
cv::Mat zSegment::getForeground()
{
    return mSegmentFG.clone();
}

/*************************/
cv::Mat zSegment::getUnknown()
{
    return mSegmentUnknown.clone();
}

/*************************/
cv::Mat zSegment::convertToMeters(cv::Mat &pImg)
{
    cv::Mat lMeters = cv::Mat::zeros(pImg.rows, pImg.cols, CV_32FC1);

    cv::MatIterator_<ushort> lImgIt = pImg.begin<ushort>();
    cv::MatIterator_<float> lMetersIt = lMeters.begin<float>();

    for(; lImgIt < pImg.end<ushort>(); lImgIt++, lMetersIt++)
    {
        *lMetersIt = 0.1236*tanf((float)(*lImgIt)/2842.5 + 1.1863);
    }

    return lMeters.clone();
}
