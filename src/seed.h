/* Classe permettant de créer, à partir d'une segmentation approchée BG / FG,
 * un ensemble de couples objets du FG / BG environnant servant de graîne
 * à une segmentation plus précise
 */

#ifndef SEED_H
#define SEED_H

#include "opencv2/opencv.hpp"

struct seedObject
{
    unsigned int size;
    cv::Mat background; // ce qui est assuré d'être dans le BG
    cv::Mat foreground; // ce qui est assuré d'être dans le FG
    cv::Mat unknown; // ce qui nécessite d'être précisé
    cv::Mat mask; // ce dont on ne tiendra pas compte

    unsigned int x_min;
    unsigned int x_max;
    unsigned int y_min;
    unsigned int y_max;
};

class seed
{
public:
    seed();

    // Spécifie la segmentation approchée
    bool setRoughSegment(const cv::Mat &pBG, const cv::Mat &pFG, const cv::Mat &pUnknown);

    // Choix de la taille minimale d'un objet
    void setMinimumSize(unsigned int pSize);

    // Choix de la taille de la dilatation
    void setDilatationSize(unsigned int pSize);

    // Renvoie l'ensemble des couples FG/BG correspondant à chaque objet
    std::vector<seedObject> getSeeds();

private:
    /***********/
    // Attributs
    /***********/
    // Surface minimale d'un blob
    unsigned int mMinSize;

    // Elément structurant pour la dilatation
    cv::Mat mStructElemDilate;
    // ... et la dimension définissant celui-ci
    unsigned int mStructElemSize;

    std::vector<seedObject> mSeeds;

    /**********/
    // Méthodes
    /**********/
    static bool cmpArea(const seedObject &pObj1, const seedObject &pObj2);
};

#endif // SEED_H
