/* Classe permettant de créer, à partir d'une matrice définissant les probas
 * liés aux données, un graphe de type MRF puis de le résoudre par une méthode
 * de graph-cut (par l'intermédiaire de la lib NPP).
 * Et tout ça, ça fait de la segmentation d'image !
 */

#ifndef COLORSEGMENT_H
#define COLORSEGMENT_H

#define GLFW_INCLUDE_GL3
#define GLFW_NO_GLU
#define GL3_PROTOTYPES

#include <iostream>
#include <chrono>

#include "opencv2/opencv.hpp"
#include "GL/glfw.h"
#include "glm/glm.hpp"
#include "npp.h"
#include "boost/thread.hpp"
#include "tbb/atomic.h"

#define __CUDA_RUNTIME_H__
#include "cuda.h"
#include "helper_cuda.h"

class colorSegment
{
public:
    colorSegment();
    ~colorSegment();

    // Initialisation générale, création des textures,
    // création du thread de rendu ...
    bool init();
    bool init(unsigned int pWidth, unsigned int pHeight); // avec les dimensions des textures en paramètres

    // Arrêt de la segmentation
    void stop();

    // Spécification du coût maximum de lissage
    void setMaxSmoothCost(unsigned int pCost);

    // Attribution de nouveaux coûts de données, de la nouvelle image,
    // avec un masque pour les éventuelle données fixées
    bool setCosts(cv::Mat &pImg, cv::Mat &pBGCosts, cv::Mat &pFGCosts, cv::Mat pBG = cv::Mat(), cv::Mat pFG = cv::Mat(),
                  unsigned int pXMin=0, unsigned int pXMax=640, unsigned int pYMin=0, unsigned int pYMax=480);

    // Récupération de la segmentation
    bool getSegment(cv::Mat &pSegment);

    // Renvoie des infos sur la zone segmentée
    void getInfos(int &size, float &ratio);

private:
    /***********/
    // Attributs
    /***********/
    // Variables intéressants à récupérer pour stats
    int mGraphcutSize;
    float mGraphcutRatio;

    bool mIsRunning;

    // Calcul des coûts de lissage
    float mSigmaCam;
    int mMaxSmoothCost;
    int mCannyCost;

    // Caractériques des images
    int mImgSize[2];

    // Limites pour la segmentation
    unsigned int mXMin, mXMax, mYMin, mYMax;
    // Les mêmes limites, mais thread safe
    unsigned int mXMin_t, mXMax_t, mYMin_t, mYMax_t;

    // Stockage de l'image à segmenter
    cv::Mat mImg;
    // Stockage des coûts de données, fournis
    cv::Mat mDataCosts;
    // Stockage des labels calculés
    cv::Mat mLabels;

    // Thread
    boost::shared_ptr<boost::thread> mMainLoop;
    boost::mutex mImgMutex;
    boost::mutex mLabelMutex;
    tbb::atomic<unsigned> mInputCounter;
    tbb::atomic<unsigned> mLabelCounter;

    // Données OpenGL
    GLuint mVertexArray;
    GLuint mVertexBuffer[2];
    GLuint mCostTextures[2];
    GLuint mCameraTexture;

    GLuint mVertexShader;
    GLuint mFragmentShader;
    GLuint mShaderProgram;
    bool mShaderValid;

    GLint mMVPMatLocation;
    GLint mResolutionLocation;
    GLint mPassLocation;

    int mFBOSize[2];
    GLuint mFBO;
    GLuint mFBOTexture[3]; // 3 textures, parce que gauche=droite et haut=bas pour les coûts de lissage

    // Espace pour récupérer les textures du FBO sur le cpu
    cv::Mat mCPUData[3];
    Npp16u* mCudaDatabuffer;
    int mCudaDatabufferStep;

    // Données CUDA
    Npp8u* mCudaGCBuffer;
    Npp8u* mCudaLabels;
    NppiGraphcutState* mCudaGraphcutState;

    int mCudaLabelsStep;

    // Des PBO, pour l'interop entre CUDA et GL (vu que ça marche pas avec les textures ...
    GLuint mPBO[3];

    struct cudaGraphicsResource* mCudaResources[3];

    /**********/
    // Méthodes
    /**********/
    // Calcul des coûts de lissage
    cv::Mat smoothCostsColor(cv::Mat &pImg);

    // Initialisation des données OpenGL
    bool initGL();

    // Initialisation de CUDA
    bool initNPP();
    bool initCUDA();

    // Boucle principale GL + CUDA
    void mainLoop();

    // Rendu GL
    void drawGL();

    // Calcul Npp;
    void computeCuda();

    // Préparation des shaders
    char* readFile(const char* pFile);
    bool compileShader();
    bool verifyShader(GLuint pShader);
    bool verifyProgram(GLuint pProgram);

    // Préparation du FBO, et de la géométrie
    bool prepareFBO();
    void prepareGeometry();
    void prepareTexture();
    void preparePBO();

    // Mise à jour des textures
    void updateTextures(cv::Mat pImg, cv::Mat pCosts);

    // Vérification des dimensions et du type d'une matrice opencv
    bool checkMatrix(cv::Mat &pMat, int pType);
};

#endif // COLORSEGMENT_H
