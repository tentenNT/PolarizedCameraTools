#include <ArenaApi.h>
#include <SaveApi.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

// [...]

GenApi::INodeMap *pNodeMap = pDevice->GetNodeMap();
gcstring windowName = Arena::GetNodeValue(pNodeMap, "DeviceModelName") + " (" + Arena::GetNodeValue(pNodeMap, "DeviceSerialNumber") + ")";

Arena::IImage *pImage;
cv::Mat img;

int framesPerSecond = (int)Arena::GetNodeValue(pNodeMap, "AcquisitionFrameRate");
int numberOfSeconds = 10;

GenApi::CFloatPtr pExposureTimeNode = pNodeMap->GetNode("ExposureTime");
const int64_t getImageTimeout_ms = static_cast(pExposureTimeNode->GetMax() / 1000 * 2);

std::cout << "Starting stream." << std::endl;
pDevice->StartStream();

for (int i = 0; i < framesPerSecond * numberOfSeconds;  i++)
{
    pImage = pDevice->GetImage(getImageTimeout_ms);
    img = cv::Mat((int)pImage->GetHeight(), (int)pImage->GetWidth(), CV_8UC1, (void *)pImage->GetData());

    cv::imshow(windowName.c_str(), img);
    cv::waitKey(1);
    pDevice->RequeueBuffer(pImage);
}
cv::destroyAllWindows();

// Stop stream
std::cout << "Stopping stream." << std::endl;
pDevice->StopStream();
