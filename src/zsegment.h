/* Classe permettant la segmentation d'images sur un canal, en comparant une image
 * à un temps donné à une image de référence, selon un écart-type donné
 * Elle est a priori plus adaptée à de la segmentation selon la distance,
 * et considère que l'on utilise des images en CV_16UC1
 */

#ifndef ZSEGMENT_H
#define ZSEGMENT_H

#include "opencv2/opencv.hpp"

class zSegment
{
public:
    zSegment();
    ~zSegment();

    // Spécifie un max, qui est la valeur au delà de laquelle on considère
    // que la reconstruction n'est pas fiable
    void setMax(unsigned int pMax);

    // Défini le lissage sur la segmentation du FG
    void setFGSmoothing(unsigned int pSmooth);

    // Evaluation de l'écart-type selon la distance
    // Nécessite plusieurs images successives d'une scène immobile
    void feedStdDevEval(cv::Mat &pImg);
    // A appeler pour calculer l'écart-type
    // Les images sources sont ensuite supprimées
    void computeStdDev();
    // Force l'écart type à une certaine valeur, pour toutes les profondeurs
    void setStdDev(int pValue);

    // Modélisation de l'arrière-plan
    // Spécifie les dimensions des images que l'on acceptera plus tard
    void feedBackground(cv::Mat &pImg);
    // Calcul l'arrière-plan (moyennage)
    void computeBackground();

    // Segmentation d'une image
    bool feedImage(cv::Mat &pImg);
    // Map de l'arrière-plan
    cv::Mat getBackground();
    // Map de l'avant-plan
    cv::Mat getForeground();
    // Map de la zone d'incertitude
    cv::Mat getUnknown();

private:
    /***********/
    // Attributs
    /***********/
    float mMax;

    bool mIsStdDev; // true si la map d'écart type existe
    bool mIsBackground; // true si le modèle de l'arrière-plan existe

    cv::Mat mStdDev; // écart-type selon la distance (matrice 1xM)
    cv::Mat mBackground; // Depthmap de l'arrière plan
    cv::Mat mBgMask; // Masque des zones non fiables du BG

    cv::Point2d mStdDevRes;
    cv::Point2d mResolution;

    std::vector<cv::Mat> mStdDevSources;
    std::vector<cv::Mat> mBackgroundSources;

    // Résultats de la segmentation
    cv::Mat mSegmentBG;
    cv::Mat mSegmentFG;
    cv::Mat mSegmentUnknown;

    cv::Mat mStructElemErode;

    /**********/
    // Méthodes
    /**********/
    // Converti une img de kinect en mètres
    cv::Mat convertToMeters(cv::Mat &pImg);
};

#endif // ZSEGMENT_H
