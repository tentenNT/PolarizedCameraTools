#pragma once
#include "ArenaApi.h"
#include "polarized.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define WIDTH                                   2448
#define HEIGHT                                  2048
#define WAITTIME                                1

using namespace Arena;

// 偏光データを再生
void RunOpencvPolarizedVideo(IDevice* pDevice, GenApi::INodeMap* pNodeMap, Polarized pol_chunk, std::string choice = "I_max");

// ストリームの開始，終了まではここで行う
void AcquireImage( IDevice* pDevice );

void ChoiceImage(cv::Mat& img, const std::string &choice);

void CaptureImage(const cv::Mat& img, IImage* pImage, const int& key, const std::string& choice, int& count);
