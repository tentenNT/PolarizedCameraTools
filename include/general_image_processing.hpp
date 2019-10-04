#include "ArenaApi.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define BLUE_GAIN                               2.87
#define GREEN_GAIN                              1.0 
#define RED_GAIN                                1.35

void ApplyWhiteBalance(cv::Mat& img);
void RawdataToColor(cv::Mat &img);
