#include "general_image_processing.hpp"

void ApplyWhiteBalance(cv::Mat& img){
    //CV_8UC3のままで計算したら飽和して出力がおかしくなった
    //どうもuchar型で計算するのはよろしくないことが起こることが多いようだ
    img.convertTo(img, CV_32FC3);
    img.forEach<cv::Point3_<float>>([](cv::Point3_<float> &p, const int* position) -> void{
        p.x *= BLUE_GAIN;
        p.y *= GREEN_GAIN;
        p.z *= RED_GAIN;
    });
    img.convertTo(img, CV_8UC3);
}

void RawdataToColor(cv::Mat &img){
    img.convertTo(img, CV_8UC1);
    cv::cvtColor(img, img, cv::COLOR_BayerBG2BGR);
    ApplyWhiteBalance(img);
}
