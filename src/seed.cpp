#include "seed.h"
#include "cvblob.h"

/******************/
seed::seed()
    :mMinSize(64)
{
    // Création de l'élément structurant pour les opérations de dilatation
    // à venir
    mStructElemDilate = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                            cv::Size(17, 17),
                                            cv::Point(8, 8));
}

/******************/
void seed::setMinimumSize(unsigned int pSize)
{
    mMinSize = pSize;
}

/******************/
void seed::setDilatationSize(unsigned int pSize)
{
    mStructElemSize = pSize;
    mStructElemDilate = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                            cv::Size(2*pSize+1, 2*pSize+1),
                                            cv::Point(pSize, pSize));
}

/******************/
bool seed::setRoughSegment(const cv::Mat &pBG, const cv::Mat &pFG, const cv::Mat &pUnknown)
{
    if(pBG.rows != pFG.rows || pBG.rows != pUnknown.rows
            || pBG.cols != pFG.cols || pBG.cols != pUnknown.cols)
        return false;

    // On commence en séparant les blobs dans pFG
    // Pour ça on utilise cvBlob
    cvb::CvBlobs lBlobs;

    IplImage lIpl(pFG);
    lIpl.depth = IPL_DEPTH_8U;

    IplImage *lFilteredLabelImg = cvCreateImage(cvGetSize(&lIpl), IPL_DEPTH_8U, 1);
    IplImage *lLabelImg = cvCreateImage(cvGetSize(&lIpl), IPL_DEPTH_LABEL, 1);

    cvb::cvLabel(&lIpl, lLabelImg, lBlobs);

    // On filtre les blobs trop petits
    cvb::cvFilterByArea(lBlobs, mMinSize, std::numeric_limits<int>::max());

    // On dessine tous les blobs restants, qui sont autant d'objets du FG,
    mSeeds.clear();

    for(cvb::CvBlobs::const_iterator lIt=lBlobs.begin(); lIt!=lBlobs.end(); lIt++)
    {
            cvb::CvBlobs lCurrentBlob;
            lCurrentBlob.insert(*lIt);

            cvb::cvFilterLabels(lLabelImg, lFilteredLabelImg, lCurrentBlob);

            // on met ça dans un nouvel élément de mSeeds
            seedObject lSeed;

            lSeed.foreground = cv::Mat(lFilteredLabelImg, true);
            lSeed.size = lIt->second->area;
            lSeed.x_min = lIt->second->minx;
            lSeed.x_max = lIt->second->maxx;
            lSeed.y_min = lIt->second->miny;
            lSeed.y_max = lIt->second->maxy;

            mSeeds.push_back(lSeed);
    }

    // Et on trie du plus gros au plus petit
    std::sort(mSeeds.begin(), mSeeds.end(), cmpArea);

    cvReleaseImage(&lFilteredLabelImg);
    cvReleaseImage(&lLabelImg);

    // Maintenant, on crée les graînes des BG
    cv::Mat lDilate = cv::Mat(pBG.rows, pBG.cols, CV_8UC1);

    for(std::vector<seedObject>::iterator it=mSeeds.begin(); it!=mSeeds.end(); it++)
    {
        // On commence par créer une dilatation de la graîne du FG,
        // ceci pour repérer les zones du BG environnant cette graîne
        cv::dilate((*it).foreground, lDilate, mStructElemDilate);

        // On ne conserve que ce qui n'est pas déjà dans le FG
        // Ceci désigne la zone sur laquelle nous allons segmenter
        lDilate = lDilate - (*it).foreground;
        (*it).unknown = lDilate.clone();

        // Finalement, tout ce qui n'est ni FG, ni dans la partie de unknown
        // qu'est lDilate sera notre BG. On n'en conserve cependant que la partie
        // environnante
        cv::dilate(lDilate, (*it).background, mStructElemDilate);
        (*it).background = (*it).background - ((*it).foreground + lDilate);

        // Et on produit le masque
        cv::bitwise_not((*it).background + (*it).foreground + lDilate, (*it).mask);

        // Bien entendu, tout ceci modifie les limites de nos blobs
        (*it).x_min = std::max((unsigned int)0, (*it).x_min-mStructElemSize);
        (*it).x_max = std::min((unsigned int)pFG.cols, (*it).x_max+mStructElemSize);
        (*it).y_min = std::max((unsigned int)0, (*it).y_min-mStructElemSize);
        (*it).y_max = std::min((unsigned int)pFG.rows, (*it).y_max+mStructElemSize);
    }

    return true;
}

/******************/
std::vector<seedObject> seed::getSeeds()
{
    return mSeeds;
}

/******************/
bool seed::cmpArea(const seedObject &pObj1, const seedObject &pObj2)
{
    return pObj1.size > pObj2.size;
}
