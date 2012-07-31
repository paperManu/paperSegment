/* Classe créant, à partir d'une image et d'un masque, une mixture de gaussienne
 * modélisant les zones non masquées. Celles-ci sont créées dans l'espace HSV
 * Le masque est une matrice en CV_16U, les images doivent être fournies en CV_8UC3
 * et de préférence en RGB.
 * Elle peut en outre retourner une matrice des probabilités selon un masque donné.
 */

#ifndef GMM_H
#define GMM_H

#include "opencv2/opencv.hpp"

struct gaussian2D
{
    float sigma[2];
    float mu[2];
    float weight;
};

class gmm
{
public:
    gmm();
    ~gmm();

    // Paramètrages
    void setClusterCount(int pCount);
    void setEMMinLikelihood(float pLikelihood);
    void setMaxEMLoop(unsigned int pLoop);
    void setMaxCost(int pCost);

    // Spécifie l'image RGB sur laquelle on travaille
    void setRgbImg(cv::Mat &pImg);

    // Spécifie le masque à utiliser pour créer le modèle
    void calcGmm(cv::Mat &pMask);

    // Renvoie la mixture de gaussienne
    std::vector<gaussian2D> getMixture();

    // Renvoie la matrice des proba du masque donné en param
    cv::Mat getProbs(cv::Mat &pMask);

    // Renvoie la matrice des coûts (log opposé) liés aux probas,
    // selon le modèle créé avec calcGmm()
    cv::Mat getCosts(cv::Mat &pMask);

private:
    /************/
    // Attribute
    /************/
    bool mIsGmm;

    cv::Mat mColorImg;
    std::vector<gaussian2D> mGmm;

    int mClusterCount;
    float mEMLikelihood;
    unsigned int mMaxEMLoop;

    int mMaxCost;

    /***********/
    // Méthodes
    /***********/
    float getLikelihood(cv::Mat &pData, cv::Mat &pMu, cv::Mat &pSigma, cv::Mat &pWeight);
    float getGaussian2DValueAt(float pX, float pY, float pMuX, float pMuY, float pSigmaX, float pSigmaY);
};

#endif // GMM_H
