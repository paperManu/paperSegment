#include "colorsegment.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

/**********************/
colorSegment::colorSegment()
    :mShaderValid(false),
      mSigmaCam(1e-1),
      mMaxSmoothCost(20),
      mCudaDatabuffer(NULL)
{
    mInputCounter = 0;
    mLabelCounter = 0;

    mImgSize[0] = 640;
    mImgSize[1] = 480;

    mFBOSize[0] = mImgSize[0]*2;
    mFBOSize[1] = mImgSize[1]*2;


    // Initialisation des buffers permettant de récupérer
    // le résultat du rendu GL
    for(int i=0; i<3; i++)
    {
        mCPUData[i] = cv::Mat(mImgSize[1]*2, mImgSize[0]*2, CV_16UC1);
    }

    // Vérification qu'on a bien du CUDA sous le capot
    initNPP();

    // Allocation des labels
    mLabels = cv::Mat::zeros(mFBOSize[1], mFBOSize[0], CV_8UC1);

    mXMin = 0;
    mXMax = mFBOSize[0];
    mYMin = 0;
    mYMax = mFBOSize[1];
}

/**********************/
colorSegment::~colorSegment()
{
    if(mIsRunning)
    {
        mIsRunning = false;
        mMainLoop->join();
    }

    if(mCudaDatabuffer)
        nppiFree(mCudaDatabuffer);
}

/**********************/
bool colorSegment::init()
{
    mMainLoop = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&colorSegment::mainLoop, this)));

    if(mMainLoop)
        return true;
    else
        return false;
}

/**********************/
bool colorSegment::init(unsigned int pWidth, unsigned int pHeight)
{
    mImgSize[0] = pWidth;
    mImgSize[1] = pHeight;

    mDataCosts = cv::Mat(pHeight, pWidth, CV_16UC1);
    mImg = cv::Mat(pHeight, pWidth, CV_8UC3);

    return init();
}

/**********************/
void colorSegment::stop()
{

}

/**********************/
void colorSegment::setMaxSmoothCost(unsigned int pCost)
{
    mMaxSmoothCost = pCost;
}

/**********************/
bool colorSegment::setCosts(cv::Mat &pImg, cv::Mat &pBGCosts, cv::Mat &pFGCosts, cv::Mat pBG, cv::Mat pFG,
                            unsigned int pXMin, unsigned int pXMax, unsigned int pYMin, unsigned int pYMax)
{
    // On vérifie que toutes ces données ont le bon format
    bool lValid = true;
    lValid &= checkMatrix(pImg, CV_8UC3);
    lValid &= checkMatrix(pBGCosts, CV_16UC1);
    lValid &= checkMatrix(pFGCosts, CV_16UC1);
    lValid &= checkMatrix(pBG, CV_8UC1);
    lValid &= checkMatrix(pFG, CV_8UC1);

    if(!lValid)
        return false;

    // On calcul notre matrice des coûts, combinaison des coûts de données
    // du FG, BG, et des zones déjà fixée par pBG et pFG
    // Du fait de l'utilisation en opengl plus tard, on décalera le zéro à 32767
    // (OpenGL n'est pas fan des valeurs négatives)
    cv::Mat lCosts = cv::Mat(mImgSize[1], mImgSize[0], CV_16UC2);

    cv::MatConstIterator_<ushort> lBGIt = pBGCosts.begin<ushort>();
    cv::MatConstIterator_<ushort> lFGIt = pFGCosts.begin<ushort>();
    cv::MatConstIterator_<uchar> lBGMaskIt = pBG.begin<uchar>();
    cv::MatConstIterator_<uchar> lFGMaskIt = pFG.begin<uchar>();
    cv::MatIterator_<cv::Vec2w> lCostsIt = lCosts.begin<cv::Vec2w>();

    for(; lCostsIt<lCosts.end<cv::Vec2w>(); lCostsIt++, lBGIt++, lFGIt++, lBGMaskIt++, lFGMaskIt++)
    {
        (*lCostsIt)[0] = 0;
        (*lCostsIt)[1] = 0;

        // Si on est dans un des masques, on le note comme tel
        // avec une valeur facilement repérable !
        if(*lFGMaskIt == 255)
            (*lCostsIt)[0] = 65535;
        else if(*lBGMaskIt == 255)
            (*lCostsIt)[1] = 65535;
        // Sinon, on copie juste les valeurs des coûts
        else
        {
            (*lCostsIt)[0] = *lFGIt;
            (*lCostsIt)[1] = *lBGIt;
        }
    }

    // On fini par copier tout ça dans les matrices qui seront effectivement
    // utilisées pour initialiser les textures GL
    mImgMutex.lock();
    cv::flip(lCosts, mDataCosts, 0);
    cv::flip(pImg, mImg, 0);

    if(pXMin >= 0 && pXMin < (unsigned int)mImgSize[0]-1)
        mXMin = pXMin;
    if(pXMax < (unsigned int)mImgSize[0] && pXMax > pXMin)
        mXMax = pXMax;

    if(pYMin >= 0 && pYMin < (unsigned int)mImgSize[1]-1)
        mYMin = pYMin;
    if(pYMax < (unsigned int)mImgSize[1] && pYMax > pYMin)
        mYMax = pYMax;
    mImgMutex.unlock();

    // Opération atomique, on signale que les textures ont changé
    mInputCounter++;

    return true;
}

/**********************/
bool colorSegment::getSegment(cv::Mat &pSegment)
{
    cv::Mat lLabel;

    static int lPreviousCounter = std::numeric_limits<int>::max();
    int lCurrentCounter;

    lCurrentCounter = mLabelCounter;
    if(lCurrentCounter != lPreviousCounter)
    {
        // On récupère la segmentation actuelle
        mLabelMutex.lock();
        lLabel = mLabels.clone();
        mLabelMutex.unlock();

        // On la reformate pour enlever les noeuds en trop
        pSegment = cv::Mat(mImgSize[1], mImgSize[0], CV_8UC1);

        for(int y=0; y<mFBOSize[1]; y+=2)
        {
            for(int x=0; x<mFBOSize[0]; x+=2)
            {
                pSegment.at<char>(y/2, x/2) = lLabel.at<char>(y, x)*255;
            }
        }

        return true;
    }

    return false;
}

/**********************/
cv::Mat colorSegment::smoothCostsColor(cv::Mat &pImg)
{
    cv::Mat lCosts;

    if(!checkMatrix(pImg, CV_8UC3))
        return lCosts;

    // Conversion de l'image en HSV
    cv::Mat lHSV;
    cv::cvtColor(pImg, lHSV, CV_BGR2HSV);

    // Recherche des contours (Canny edge detector)
    cv::Mat lGray;
    cv::Mat lEdges;

    cv::cvtColor(pImg, lGray, CV_BGR2GRAY);
    cv::Canny(lGray, lEdges, 30.f, 50.f);

    // Calcul des coûts. On a besoin de 3 itérateurs :
    // celui sur le pixel en cours, le pixel à droite, le pixel en bas
    // Les coûts sur les bords doivent être nuls, on les annulera plus tard
    lCosts = cv::Mat::zeros(pImg.rows, pImg.cols, CV_16UC2);
    cv::MatIterator_<cv::Vec2w> lCostsIt = lCosts.begin<cv::Vec2w>();
    cv::MatConstIterator_<cv::Vec3b> lPixIt = lHSV.begin<cv::Vec3b>();
    cv::MatConstIterator_<cv::Vec3b> lPixHIt = lPixIt++;
    cv::MatConstIterator_<cv::Vec3b> lPixVIt = lPixIt + pImg.cols;
    cv::MatConstIterator_<uchar> lGrayIt = lGray.begin<uchar>();
    cv::MatConstIterator_<uchar> lGrayHIt = lGrayIt++;
    cv::MatConstIterator_<uchar> lGrayVIt = lGrayIt + pImg.cols;

    cv::MatConstIterator_<cv::Vec3b> lEnd = lHSV.end<cv::Vec3b>();

    for(; lPixIt < lEnd; lPixIt++, lCostsIt++, lPixHIt++, lPixVIt++
        , lGrayIt++, lGrayHIt++, lGrayVIt++)
    {
        // On vérifie que l'itérateur vertical ne sort pas de l'image
        if(lPixVIt >= lEnd)
        {
            lPixVIt = lHSV.begin<cv::Vec3b>();
            lGrayVIt = lGray.begin<uchar>();
        }

        cv::Vec3f lP, lQ, lR; // Nos trois pixels convertis dans des formats plus traditionnels
        float lX, lY;
        lX = (float)(*lPixIt)[0] * 2.f;
        lY = (float)(*lPixIt)[1] / 255.f;
        lP[0] = lY*cos(lX*M_PI/180.f);
        lP[1] = lY*sin(lX*M_PI/180.f);
        lP[2] = (float)(*lPixIt)[2] / 255.f;

        lX = (float)(*lPixHIt)[0] * 2.f;
        lY = (float)(*lPixHIt)[1] / 255.f;
        lQ[0] = lY*cos(lX*M_PI/180.f);
        lQ[1] = lY*sin(lX*M_PI/180.f);
        lQ[2] = (float)(*lPixHIt)[2] / 255.f;

        lX = (float)(*lPixVIt)[0] * 2.f;
        lY = (float)(*lPixVIt)[1] / 255.f;
        lR[0] = lY*cos(lX*M_PI/180.f);
        lR[1] = lY*sin(lX*M_PI/180.f);
        lR[2] = (float)(*lPixVIt)[2] / 255.f;

        (*lCostsIt)[0] = expf(-((lP[0]-lQ[0])*(lP[0]-lQ[0])+(lP[1]-lQ[1])*(lP[1]-lQ[1])+(lP[2]-lQ[2])*(lP[2]-lQ[2]))/(2.f*mSigmaCam*mSigmaCam)) * mMaxSmoothCost;
        if(*lGrayIt == 0 && *lGrayHIt == 0)
            (*lCostsIt)[0] += mCannyCost;

        (*lCostsIt)[1] = expf(-((lP[0]-lR[0])*(lP[0]-lR[0])+(lP[1]-lR[1])*(lP[1]-lR[1])+(lP[2]-lR[2])*(lP[2]-lR[2]))/(2.f*mSigmaCam*mSigmaCam)) * mMaxSmoothCost;
        if(*lGrayIt == 0 && *lGrayVIt == 0)
            (*lCostsIt)[1] += mCannyCost;
    }

    return lCosts;
}

/**********************/
bool colorSegment::initGL()
{
    // Initialisation de tout ce qui est GL
    if(!glfwInit())
    {
        std::cerr << "Failed to create GL context." << std::endl;
        glfwTerminate();
        return false;
    }

    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 0);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if(!glfwOpenWindow(mImgSize[0], mImgSize[1], 0, 0, 0, 0, 0, 0, GLFW_WINDOW))
    {
        std::cerr << "Failed to crqeate GL window." << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetWindowTitle("colorSegment");
    glfwSwapInterval(1);
    glClearColor(0.f, 0.f, 0.f, 1.f);

    prepareGeometry();
    prepareTexture();
    prepareFBO();
    //preparePBO();

    mShaderValid = compileShader();
    if(!mShaderValid)
        return false;

    glViewport(0, 0, mImgSize[0], mImgSize[1]);

    return true;
}

/************************************/
bool colorSegment::initNPP()
{
    NppGpuComputeCapability lResult = nppGetGpuComputeCapability();

    switch(lResult)
    {
    case NPP_CUDA_1_0:
        std::cerr << "CUDA version 1.0 detected." << std::endl;
        break;
    case NPP_CUDA_1_1:
        std::cerr << "CUDA version 1.1 detected." << std::endl;
        break;
    case NPP_CUDA_1_2:
        std::cerr << "CUDA version 1.2 detected." << std::endl;
        break;
    case NPP_CUDA_1_3:
        std::cerr << "CUDA version 1.3 detected." << std::endl;
        break;
    case NPP_CUDA_2_0:
        std::cerr << "CUDA version 2.0 detected." << std::endl;
        break;
    case NPP_CUDA_2_1:
        std::cerr << "CUDA version 2.0 detected." << std::endl;
        break;
    case NPP_CUDA_3_0:
        std::cerr << "CUDA version 2.0 detected." << std::endl;
        break;
    case NPP_CUDA_UNKNOWN_VERSION:
        std::cerr << "CUDA unknown version detected." << std::endl;
        break;
    case NPP_CUDA_NOT_CAPABLE:
    default:
        std::cerr << "No CUDA device detected." << std::endl;
        return false;
    }

    cutilSafeCall(cudaSetDevice(cutGetMaxGflopsDeviceId()));

    return true;
}

/**********************/
bool colorSegment::initCUDA()
{
    // Sélection du device GL
//    cutilSafeCall(cudaGLSetGLDevice(cutGetMaxGflopsDeviceId()));

//    cutilSafeCall(cudaGraphicsGLRegisterBuffer(&mCudaResources[0], mPBO[0], cudaGraphicsMapFlagsReadOnly));
//    cutilSafeCall(cudaGraphicsGLRegisterBuffer(&mCudaResources[1], mPBO[1], cudaGraphicsMapFlagsReadOnly));
//    cutilSafeCall(cudaGraphicsGLRegisterBuffer(&mCudaResources[2], mPBO[2], cudaGraphicsMapFlagsReadOnly));

//    return true;
}

/**********************/
void colorSegment::mainLoop()
{
    static unsigned int lPreviousValue = std::numeric_limits<unsigned int>::max();
    unsigned int lCurrentValue;

    initGL();
    //initCUDA();

    mIsRunning = true;
    while(mIsRunning)
    {
        // Mise à jour des textures
        lCurrentValue = mInputCounter;
        if(lCurrentValue != lPreviousValue)
        {
            mImgMutex.lock();
            updateTextures(mImg, mDataCosts);
            mXMin_t = mXMin;
            mXMax_t = mXMax;
            mYMin_t = mYMin;
            mYMax_t = mYMax;
            mImgMutex.unlock();

            // Rendu OpenGL
            drawGL();

            // Cuda / Npp
            computeCuda();
        }
        lPreviousValue = lCurrentValue;

        if(glfwGetKey(GLFW_KEY_ESC) || !glfwGetWindowParam(GLFW_OPENED))
            mIsRunning = false;
    }

    glfwTerminate();
}

/**********************/
void colorSegment::drawGL()
{
    // On ne touche pas aux textures pendant le rendu
    // (non mais)
    // Envoi des données au shader
    // Matrice vue projection
    glm::mat4 lProjMatrix = glm::ortho(-1.f, 1.f, -1.f, 1.f);
    glUniformMatrix4fv(mMVPMatLocation, 1, GL_FALSE, glm::value_ptr(lProjMatrix));

    // Rendu dans le FBO
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

    GLfloat lImgSize[2];
    lImgSize[0] = (float)mImgSize[0];
    lImgSize[1] = (float)mImgSize[1];

    // Résolution
    glUniform2fv(mResolutionLocation, 1, lImgSize);

    glViewport(0, 0, mFBOSize[0], mFBOSize[1]);
    glUniform1i(mPassLocation, (GLint)1);
    GLenum lFBOBuf[] = {GL_COLOR_ATTACHMENT0,
                        GL_COLOR_ATTACHMENT1,
                        GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, lFBOBuf);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Copie des trois textures attachées dans les trois PBO
    /*glActiveTexture(GL_TEXTURE7);
    for(int i=0; i<3; i++)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO[i]);
        glBindTexture(GL_TEXTURE_2D, mFBOTexture[i]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }*/

    // Rendu de la fenêtre principale
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Résolution
    glUniform2iv(mResolutionLocation, 1, (GLint*)mImgSize);

    glViewport(0, 0, mImgSize[0], mImgSize[1]);
    glUniform1i(mPassLocation, (GLint)0);
    GLenum lBackbuffer[] = {GL_BACK};
    glDrawBuffers(1, lBackbuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers();

    // Copie des trois textures dans la mémoire cpu
    glActiveTexture(GL_TEXTURE7);
    for(int i=0; i<3; i++)
    {
        glBindTexture(GL_TEXTURE_2D, mFBOTexture[i]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_SHORT, mCPUData[i].data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

//    cv::imwrite("first.png", mCPUData[0]);
//    cv::imwrite("second.png", mCPUData[1]);
//    cv::imwrite("third.png", mCPUData[2]);
}

/**********************/
void colorSegment::computeCuda()
{
    NppiSize lSize;
    // On ne va faire la segmentation que sur une zone précise
    lSize.width = (mXMax_t - mXMin_t)*2;
    lSize.height = (mYMax_t - mYMin_t)*2;

    // Le décallage entre le début des données totales, et le début de la zone étudiée
    int lDeltaBuffer = mXMin_t*2 + mYMin_t*2*mFBOSize[0];

    if(mCudaDatabuffer == NULL)
    {
        mCudaDatabuffer = nppiMalloc_16u_C1(lSize.width, lSize.height, &mCudaDatabufferStep);
        if(mCudaDatabuffer != NULL)
        {
            std::cerr << "CUDA / NPP initialized." << std::endl;
        }
        else
        {
            std::cerr << "Problem detected with CUDA. Exiting." << std::endl;
            exit(1);
        }

        // Il s'agit juste de la phase d'initialisation, on ne veut pas segmenter pour l'instant
        return;
    }
    else
    {
        nppiFree(mCudaDatabuffer);
        mCudaDatabuffer = nppiMalloc_16u_C1(lSize.width, lSize.height, &mCudaDatabufferStep);
    }

    NppStatus lStatus;
    NppiSize lRoiSize;

    // On copie les résultats du rendu GL
    // Pour les différentiels de coût des terminaux, on les converti simplement,
    // en soustrayant 32768 (2^15) puisque ceux-ci peuvent être négatifs et cela a
    // été pris en compte dans le rendu GL
    int lTerminalsStep;
    Npp32s* lTerminals = nppiMalloc_32s_C1(lSize.width, lSize.height, &lTerminalsStep);
    cutilSafeCall(cudaMemcpy2D(mCudaDatabuffer, mCudaDatabufferStep, mCPUData[0].data+lDeltaBuffer*sizeof(ushort), mFBOSize[0]*sizeof(ushort),
                               lSize.width*sizeof(ushort), lSize.height, cudaMemcpyHostToDevice));
    lStatus = nppiConvert_16u32s_C1R(mCudaDatabuffer, mCudaDatabufferStep, lTerminals, lTerminalsStep, lSize);
    lStatus = nppiSubC_32s_C1IRSfs((Npp32s)32767, lTerminals, lTerminalsStep, lSize, 1);

    /*****
    // Debug
    cv::Mat lMat = cv::Mat::zeros(lSize.height, lSize.width, CV_16UC1);
    cutilSafeCall(cudaMemcpy2D(lMat.data, lSize.width*sizeof(ushort), mCudaDatabuffer, mCudaDatabufferStep, lSize.width*sizeof(ushort), lSize.height, cudaMemcpyDeviceToHost));
    cv::imwrite("terminals.png", lMat);
    /*****/

    // On converti simplement les coûts vers le bas
    int lDownStep;
    Npp32s* lDown = nppiMalloc_32s_C1(lSize.width, lSize.height, &lDownStep);
    cutilSafeCall(cudaMemcpy2D(mCudaDatabuffer, mCudaDatabufferStep, mCPUData[2].data+lDeltaBuffer*sizeof(ushort), mFBOSize[0]*sizeof(ushort),
                               lSize.width*sizeof(ushort), lSize.height, cudaMemcpyHostToDevice));
    // On met la dernière à zéro malgré tout, par sécurité
    lRoiSize.width = lSize.width;
    lRoiSize.height = 1;
    lStatus = nppiSet_16u_C1R(0, mCudaDatabuffer, mCudaDatabufferStep, lRoiSize);
    lStatus = nppiSet_16u_C1R(0, mCudaDatabuffer+mCudaDatabufferStep/2*(lSize.height-1), mCudaDatabufferStep, lRoiSize);
    lStatus = nppiConvert_16u32s_C1R(mCudaDatabuffer, mCudaDatabufferStep, lDown, lDownStep, lSize);

    /*****
    // Debug
    lMat = cv::Mat::zeros(lSize.height, lSize.width, CV_16UC1);
    cutilSafeCall(cudaMemcpy2D(lMat.data,  lSize.width*sizeof(ushort), mCudaDatabuffer, mCudaDatabufferStep, lSize.width*sizeof(ushort), lSize.height, cudaMemcpyDeviceToHost));
    cv::imwrite("down.png", lMat);
    /*****/

    // Pour les coûts vers le haut, il faut décaler les coûts précédents vers le bas
    int lBufferStep, lTmp1Step;
    Npp16u* lBuffer = nppiMalloc_16u_C1(lSize.width, lSize.height, &lBufferStep);
    Npp16u* lTmp1 = nppiMalloc_16u_C1(lSize.width, lSize.height, &lTmp1Step);

    NppiRect lRect;
    lRect.x = 0;
    lRect.y = 0;
    lRect.width = lSize.width;
    lRect.height = lSize.height;

    lStatus = nppiRotate_16u_C1R(mCudaDatabuffer, lSize, mCudaDatabufferStep, lRect, lBuffer, lBufferStep, lRect, 0, 0, -1, NPPI_INTER_NN);
    // On s'assure que la première ligne est à zéro
    lStatus = nppiMirror_16u_C1R(lBuffer, lBufferStep, lTmp1, lTmp1Step, lSize, NPP_HORIZONTAL_AXIS);
    lRoiSize.width = lSize.width;
    lRoiSize.height = 1;
    lStatus = nppiSet_16u_C1R(0, lTmp1, lTmp1Step, lRoiSize);
    lStatus = nppiSet_16u_C1R(0, lTmp1+lTmp1Step/2*(lSize.height-1), lTmp1Step, lRoiSize);
    lStatus = nppiMirror_16u_C1R(lTmp1, lTmp1Step, lBuffer, lBufferStep, lSize, NPP_HORIZONTAL_AXIS);

    int lUpStep;
    Npp32s* lUp = nppiMalloc_32s_C1(lSize.width, lSize.height, &lUpStep);
    lStatus = nppiConvert_16u32s_C1R(lBuffer, lBufferStep, lUp, lUpStep, lSize);

    /*****
    // Debug
    lMat = cv::Mat::zeros(lSize.height, lSize.width, CV_16UC1);
    cutilSafeCall(cudaMemcpy2D(lMat.data, lSize.width*sizeof(ushort), lBuffer, lBufferStep, lSize.width*sizeof(ushort), lSize.height, cudaMemcpyDeviceToHost));
    cv::imwrite("up.png", lMat);
    /*****/
    nppiFree(lBuffer);
    nppiFree(lTmp1);

    // Pour les coûts vers la droite et la gauche, ils doivent être transposés
    // Ne nous privons donc pas !
    int lTmp2Step;
    int lMaxSize = std::max(lSize.width, lSize.height);

    NppiSize lSquareSize;
    lSquareSize.width = lMaxSize;
    lSquareSize.height = lMaxSize;
    NppiRect lSquareRect;
    lSquareRect.x = 0;
    lSquareRect.y = 0;
    lSquareRect.width = lMaxSize;
    lSquareRect.height = lMaxSize;

    lBuffer = nppiMalloc_16u_C1(lMaxSize, lMaxSize, &lBufferStep);
    lTmp1 = nppiMalloc_16u_C1(lMaxSize, lMaxSize, &lTmp1Step);
    Npp16u* lTmp2 = nppiMalloc_16u_C1(lMaxSize, lMaxSize, &lTmp2Step);

    cutilSafeCall(cudaMemcpy2D(mCudaDatabuffer, mCudaDatabufferStep, mCPUData[1].data+lDeltaBuffer*sizeof(ushort), mFBOSize[0]*sizeof(ushort),
                               lSize.width*sizeof(ushort), lSize.height, cudaMemcpyHostToDevice));
    lStatus = nppiCopy_16u_C1R(mCudaDatabuffer, mCudaDatabufferStep, lBuffer, lBufferStep, lSize);
    lStatus = nppiMirror_16u_C1R(lBuffer, lBufferStep, lTmp1, lTmp1Step, lSize, NPP_VERTICAL_AXIS);
    // Le -1 de la ligne suivante est nécessaire pour conserver un graphe cohérent
    lStatus = nppiRotate_16u_C1R(lTmp1, lSquareSize, lTmp1Step, lSquareRect, lTmp2, lTmp2Step, lSquareRect, 90, 0, lSize.width-1, NPPI_INTER_NN);
    // On s'assure que les pixels à droite sont nuls
    lRoiSize.width = lSize.height;
    lRoiSize.height = 1;
    lStatus = nppiSet_16u_C1R(0, lTmp2, lTmp2Step, lRoiSize);
    lStatus = nppiSet_16u_C1R(0, lTmp2+lTmp2Step/2*(lSize.width-1), lTmp2Step, lRoiSize);
    // On obtient directement les coûts vers la droite
    NppiSize lTSize;
    lTSize.width = lSize.height;
    lTSize.height = lSize.width;

    int lRightStep;
    Npp32s* lRight = nppiMalloc_32s_C1(lSize.height, lSize.width, &lRightStep);
    lStatus = nppiConvert_16u32s_C1R(lTmp2, lTmp2Step, lRight, lRightStep, lTSize);

    /*****
    // Debug
    lMat = cv::Mat::zeros(lSize.width, lSize.height, CV_16UC1);
    cutilSafeCall(cudaMemcpy2D(lMat.data, lSize.height*sizeof(ushort), lTmp2, lTmp2Step, lSize.height*sizeof(ushort), lSize.width, cudaMemcpyDeviceToHost));
    cv::imwrite("right.png", lMat);
    /*****/

    // On décale vers la droite pour avoir les coûts vers la gauche
    lStatus = nppiRotate_16u_C1R(lTmp2, lSquareSize, lTmp2Step, lSquareRect, lTmp1, lTmp1Step, lSquareRect, 0, 0, -1, NPPI_INTER_NN);
    // On s'assure que les pixels à gauche sont nuls
    lRoiSize.width = lMaxSize;
    lRoiSize.height = 1;
    lStatus = nppiSet_16u_C1R(0, lTmp1, lTmp1Step, lRoiSize);
    lStatus = nppiSet_16u_C1R(0, lTmp1+(lTmp1Step/2)*(lSize.width-1), lTmp1Step, lRoiSize);
    // Et on converti le résultat
    int lLeftStep;
    Npp32s* lLeft = nppiMalloc_32s_C1(lSize.height, lSize.width, &lLeftStep);
    lStatus = nppiConvert_16u32s_C1R(lTmp1, lTmp1Step, lLeft, lLeftStep, lTSize);

    /*****
    // Debug
    lMat = cv::Mat::zeros(lSize.width, lSize.height, CV_16UC1);
    cutilSafeCall(cudaMemcpy2D(lMat.data, lSize.height*sizeof(ushort), lTmp1, lTmp1Step, lSize.height*sizeof(ushort), lSize.width, cudaMemcpyDeviceToHost));
    cv::imwrite("left.png", lMat);
    /*****/

    // On en profite pour allouer les données du graphcut
    int lGCBufferSize;
    nppiGraphcutGetSize(lSize, &lGCBufferSize);
    cudaMalloc(&mCudaGCBuffer, lGCBufferSize);
    nppiGraphcutInitAlloc(lSize, &mCudaGraphcutState, mCudaGCBuffer);

    mCudaLabels = nppiMalloc_8u_C1(lSize.width, lSize.height, &mCudaLabelsStep);

    // Graphcut !
    std::cerr << "Start graphcut ...";
    lStatus = nppiGraphcut_32s8u(lTerminals, lLeft, lRight, lUp, lDown, lDownStep, lLeftStep,
                                           lSize, mCudaLabels, mCudaLabelsStep, mCudaGraphcutState);

    if(lStatus < 0)
        std::cerr << "...a error has occured...";
    else if(lStatus > 0)
        std::cerr << "...a warning has occured...";

    std::cerr << "... ended." << std::endl;

    // Copie du résultat dans mLabels
    mLabelCounter++;
    mLabelMutex.lock();
    mLabels.setTo(1); // On met toutes les valeurs à 1, puis on copie juste la partie segmentée
    cudaMemcpy2D(mLabels.data+lDeltaBuffer, mFBOSize[0], mCudaLabels, mCudaLabelsStep, lSize.width, lSize.height, cudaMemcpyDeviceToHost);
    //cv::imwrite("segment.png", mLabels);
    mLabelMutex.unlock();

    nppiFree(mCudaGCBuffer);
    nppiFree(mCudaLabels);
    nppiFree(mCudaGraphcutState);

    nppiFree(lTerminals);
    nppiFree(lUp);
    nppiFree(lDown);
    nppiFree(lLeft);
    nppiFree(lRight);
    nppiFree(lBuffer);
    nppiFree(lTmp1);
    nppiFree(lTmp2);
}

/**********************/
char* colorSegment::readFile(const char *pFile)
{
    long lLength;
    char* lBuffer;

    std::cout << pFile << std::endl;

    FILE *lFile = fopen(pFile, "rb");
    if(!lFile)
    {
        std::cout << "Unable to find specified file: " << pFile << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    fseek(lFile, 0, SEEK_END);
    lLength = ftell(lFile);
    lBuffer = (char*)malloc(lLength+1);
    fseek(lFile, 0, SEEK_SET);
    fread(lBuffer, lLength, 1, lFile);
    fclose(lFile);
    lBuffer[lLength] = 0;

    std::cout << lBuffer << std::endl;

    return lBuffer;
}

/**********************/
bool colorSegment::compileShader()
{
    GLchar* lSrc;
    bool lResult;

    // Vertex shader
    mVertexShader = glCreateShader(GL_VERTEX_SHADER);
    lSrc = readFile("./vertex.vert");
    glShaderSource(mVertexShader, 1, (const GLchar**)&lSrc, 0);
    glCompileShader(mVertexShader);
    lResult = verifyShader(mVertexShader);
    if(!lResult)
    {
        return false;
    }
    free(lSrc);

    // Fragment shader
    mFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    lSrc = readFile("./fragment.frag");
    glShaderSource(mFragmentShader, 1, (const GLchar**)&lSrc, 0);
    glCompileShader(mFragmentShader);
    lResult = verifyShader(mFragmentShader);
    if(!lResult)
    {
        return false;
    }
    free(lSrc);

    // Création du programme
    mShaderProgram = glCreateProgram();
    glAttachShader(mShaderProgram, mVertexShader);
    glAttachShader(mShaderProgram, mFragmentShader);
    glBindAttribLocation(mShaderProgram, 0, "vVertex");
    glBindAttribLocation(mShaderProgram, 1, "vTexCoord");
    glLinkProgram(mShaderProgram);
    lResult = verifyProgram(mShaderProgram);
    if(!lResult)
    {
        return false;
    }

    glUseProgram(mShaderProgram);

    // Préparation des textures de coût
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mCostTextures[0]);
    GLint lTextureUniform = glGetUniformLocation(mShaderProgram, "vDatacost");
    glUniform1i(lTextureUniform, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mCostTextures[1]);
    lTextureUniform = glGetUniformLocation(mShaderProgram, "vSmoothcost");
    glUniform1i(lTextureUniform, 1);

    // Préparation de la texture caméra
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mCameraTexture);
    lTextureUniform = glGetUniformLocation(mShaderProgram, "vTexCamera");
    glUniform1i(lTextureUniform, 2);

    // Retour du FBO, pour debug
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mFBOTexture[0]);
    lTextureUniform = glGetUniformLocation(mShaderProgram, "vTexFBO");
    glUniform1i(lTextureUniform, 3);

    // Matrice de transformation
    mMVPMatLocation = glGetUniformLocation(mShaderProgram, "vMVP");
    // Résolution
    mResolutionLocation = glGetUniformLocation(mShaderProgram, "vResolution");
    // Passe (hw framebuffer ou FBO)
    mPassLocation = glGetUniformLocation(mShaderProgram, "vPass");

    return true;
}

/**********************/
bool colorSegment::verifyShader(GLuint pShader)
{
    GLint lIsCompiled;
    glGetShaderiv(pShader, GL_COMPILE_STATUS, &lIsCompiled);

    if(lIsCompiled == true)
    {
        std::cout << "Shader successfully compiled." << std::endl;
    }
    else
    {
        std::cout << "Failed to compile shader." << std::endl;
    }

    GLint lLength;
    char* lLogInfo;
    glGetShaderiv(pShader, GL_INFO_LOG_LENGTH, &lLength);
    lLogInfo = (char*)malloc(lLength);
    glGetShaderInfoLog(pShader, lLength, &lLength, lLogInfo);
    std::string lLogInfoStr = std::string(lLogInfo);
    free(lLogInfo);

    std::cout << lLogInfoStr << std::endl;
    std::cout << "-------" << std::endl;

    if(lIsCompiled == false)
    {
        return false;
    }
    return true;
}

/**********************/
bool colorSegment::verifyProgram(GLuint pProgram)
{
    GLint lIsLinked;
    glGetProgramiv(pProgram, GL_LINK_STATUS, &lIsLinked);

    if(lIsLinked == true)
    {
        std::cout << "Program successfully linked." << std::endl;
    }
    else
    {
        std::cout << "Failed to link program." << std::endl;
    }

    GLint lLength;
    char* lLogInfo;
    glGetProgramiv(pProgram, GL_INFO_LOG_LENGTH, &lLength);
    lLogInfo = (char*)malloc(lLength);
    glGetProgramInfoLog(pProgram, lLength, &lLength, lLogInfo);
    std::string lLogInfoStr = std::string(lLogInfo);
    free(lLogInfo);

    std::cout << lLogInfoStr << std::endl;
    std::cout << "-------" << std::endl;

    if(lIsLinked == false)
    {
        glfwTerminate();
        return false;
    }
    return true;
}

/**********************/
bool colorSegment::prepareFBO()
{
    glGenFramebuffers(1, &mFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

    for(int i=0; i<3; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, mFBOTexture[i], 0);
    }

    GLenum lFBOStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(lFBOStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Error while preparing the FBO." << std::endl;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

/**********************/
void colorSegment::prepareGeometry()
{
    GLfloat lPoints[] = {-1.f, -1.f, 0.f, 1.f,
                         -1.f, 1.f, 0.f, 1.f,
                         1.f, 1.f, 0.f, 1.f,
                         1.f, 1.f, 0.f, 1.f,
                         1.f, -1.f, 0.f, 1.f,
                         -1.f, -1.f, 0.f, 1.f};

    GLfloat lTex[] = {0.f, 0.f,
                      0.f, 1.f,
                      1.f, 1.f,
                      1.f, 1.f,
                      1.f, 0.f,
                      0.f, 0.f};

    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);

    // Coordonnées des vertices
    glGenBuffers(2, mVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, 6*4*sizeof(float), lPoints, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Coordonnées de texture
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, 6*2*sizeof(float), lTex, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
}

/**********************/
void colorSegment::prepareTexture()
{
    // Préparation des textures contenant les coûts
    glGenTextures(2, mCostTextures);

    // Texture du coût des données
    glBindTexture(GL_TEXTURE_2D, mCostTextures[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, mImgSize[0], mImgSize[1], 0, GL_RG, GL_UNSIGNED_SHORT, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Texture du coût de lissage
    glBindTexture(GL_TEXTURE_2D, mCostTextures[1]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, mImgSize[0], mImgSize[1], 0, GL_RG, GL_UNSIGNED_SHORT, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Préparation des textures du FBO, contenant les matrices qui
    // seront utilisées pour les NPP
    glGenTextures(3, mFBOTexture);

    for(int i=0; i<3; i++)
    {
        glBindTexture(GL_TEXTURE_2D, mFBOTexture[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, mImgSize[0]*2, mImgSize[1]*2, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Préparation de la texture qui contiendra l'image de la caméra
    glGenTextures(1, &mCameraTexture);
    glBindTexture(GL_TEXTURE_2D, mCameraTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, mImgSize[0], mImgSize[1], 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

}

/**********************/
void colorSegment::preparePBO()
{
    glGenBuffers(3, mPBO);

    for(int i=0; i<3; i++)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, mImgSize[0]*mImgSize[1]*2, NULL, GL_DYNAMIC_COPY);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
}

/**********************/
void colorSegment::updateTextures(cv::Mat pImg, cv::Mat pCosts)
{
    // Calcul des coûts de lissage
    cv::Mat lSmoothCosts = smoothCostsColor(pImg);

    // Upload des textures
    // Texture de coût lié aux données
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mCostTextures[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mImgSize[0], mImgSize[1], GL_RG, GL_UNSIGNED_SHORT, pCosts.data);
    // Texture de coût de lissage
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mCostTextures[1]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mImgSize[0], mImgSize[1], GL_RG, GL_UNSIGNED_SHORT, lSmoothCosts.data);
    // Texture de l'image RGB de la caméra
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mCameraTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mImgSize[0], mImgSize[1], GL_RGB, GL_UNSIGNED_BYTE, pImg.data);
}

/**********************/
bool colorSegment::checkMatrix(cv::Mat &pMat, int pType)
{
    if(pMat.rows != mImgSize[1] || pMat.cols != mImgSize[0])
        return false;

    if(pMat.type() != pType)
        return false;

    return true;
}
