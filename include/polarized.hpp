#pragma once

#include "ArenaApi.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define WIDTH                                   2448
#define HEIGHT                                  2048
#define TIMEOUT                                 2000

using namespace Arena;

struct Polarized{
    //偏光情報へのポインタ
    void *pDeg0, *pDeg45, *pDeg90, *pDeg135;
    //偏光情報をcv::Mat型にしたもの
    cv::Mat deg0_mat, deg45_mat, deg90_mat, deg135_mat;
    //偏光パラメータの計算に使用
    cv::Mat deg0_mat_f, deg45_mat_f, deg90_mat_f, deg135_mat_f, C_1, C_2, R, I_a, I_b;
    //偏光のパラメータ
    cv::Mat I_max, I_min, rho, theta, theta_c3;
    cv::Mat s_1, s_2, s_3;

    int rows, cols;
    
    // コンストラクタ
    Polarized(IDevice* pDevice, void* pDeg0, void* pDeg45, void* pDeg90, void* pDeg135)
    : pDeg0(pDeg0), pDeg45(pDeg45), pDeg90(pDeg90), pDeg135(pDeg135){
        IImage* pImage;
        pImage = pDevice->GetImage(TIMEOUT);
        rows = (int)pImage->GetHeight() / 2;
        cols = (int)pImage->GetWidth() / 2;
        pDevice->RequeueBuffer(pImage); // いらないかも
        std::cout << rows << "\n" << cols << std::endl;
    }

    void GetPolarizedData(IImage* pImage);

    // ポインタからcv::Matへ変換
    void TranslatePointerToMatrix(IImage* pImage);

    // I_max, I_minを計算
    void CalculateIntensity();

    // rho（DoLP, 偏光度）を計算
    void CalculateDoLP();

    // theta（AoLP，偏光角）を計算
    void CalculateAoLP();

    void ConvertAoLPmonoToHSV();
};

void ApplyWhiteBalance(cv::Mat& img);
