#include "stdafx.h"
#include "ArenaApi.h"
#include "SaveApi.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#define WIDTH                                   2448
#define HEIGHT                                  2048
#define FILE_NAME                               "polarize.raw"
#define FILE_NAME_0DEG                  "0degree.raw"
#define FILE_NAME_45DEG                 "45degree.raw"
#define FILE_NAME_90DEG                 "90degree.raw"
#define FILE_NAME_135DEG                "135degree.raw"
#define FILE_NAME_0DEG_BGR              "0degree_bgr.raw"
#define FILE_NAME_45DEG_BGR             "45degree_bgr.raw"
#define FILE_NAME_90DEG_BGR             "90degree_bgr.raw"
#define FILE_NAME_135DEG_BGR    "135degree_bgr.raw"
#define RED_GAIN                                1.35
#define GREEN_GAIN                              1.0
#define BLUE_GAIN                               2.87
#define TIMEOUT                                 2000
#define numberOfSeconds                         10
#define waitTime                                1
using namespace Arena;

int GetPolarizeData(IImage* pImage, void* pDeg0, void* pDeg45, void* pDeg90, void* pDeg135){
    const uint32_t pixelFormat = static_cast<uint32_t>(pImage->GetPixelFormat());
    uint8_t* inputBuff = const_cast<uint8_t*>(pImage->GetData());
    uint8_t* src = inputBuff;
    unsigned char* dest0 = (unsigned char*)pDeg0;
    unsigned char* dest45 = (unsigned char*)pDeg45;
    unsigned char* dest90 = (unsigned char*)pDeg90;
    unsigned char* dest135 = (unsigned char*)pDeg135;

    // 偏光データを取り出しメモリに格納
    for (int y = 0; y < HEIGHT; y += 2){
        uint8_t* srcImage = src + y * WIDTH;
        for (int x = 0; x < WIDTH; x += 2){
            *dest90++ = *srcImage;
            *dest45++ = *(srcImage + 1);
            *dest135++ = *(srcImage + WIDTH);
            *dest0++ = *(srcImage + WIDTH + 1);
            srcImage += 2;
        }
    }
    return 0;
}


//OpenCVで動画像を取得
void OpencvVideo(IDevice* pDevice, GenApi::INodeMap* pNodeMap){
    gcstring windowName = GetNodeValue<GenICam::gcstring>(pNodeMap, "DeviceModelName") + " (" + GetNodeValue<GenICam::gcstring>(pNodeMap, "DeviceSerialNumber") + ")";

    IImage *pImage;
    cv::Mat img;

    double framesPerSecond = GetNodeValue<double>(pNodeMap, "AcquisitionFrameRate");

    for (int i = 0; i < framesPerSecond * numberOfSeconds;  i++){
        pImage = pDevice->GetImage(TIMEOUT);
        img = cv::Mat((int)pImage->GetHeight(), (int)pImage->GetWidth(), CV_8UC1, (void *)pImage->GetData());
        //イメージのリサイズ
        cv::resize(img, img, cv::Size(), 0.5, 0.5);

        cv::imshow(windowName.c_str(), img);
        pDevice->RequeueBuffer(pImage);
        cv::waitKey(waitTime);
    }
    cv::destroyAllWindows();
    pDevice->StopStream();
}


// ストリームの開始，終了まではここで行う
void AcquireImage( IDevice* pDevice ){
    // Acquisition ModeをContinuousに設定
    SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "AcquisitionMode", "Continuous");

    // Buffer handling modeをNewestOnlyに設定
    Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetTLStreamNodeMap(), "StreamBufferHandlingMode", "NewestOnly");
    
    // Stream auto negotiate packet sizeを有効化
    Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamAutoNegotiatePacketSize", true);

    // ピクセルフォーマットをPolarizeMono8に設定
    SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "PixelFormat", "PolarizeMono8");

    // ストリームパケットの再送信を有効化
    Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamPacketResendEnable", true);

    // ストリームの開始
    std::cout << "ストリームの開始\n";
    pDevice->StartStream();
    
    // ノードマップの取得
    GenApi::INodeMap* pNodeMap = pDevice->GetNodeMap();
    GenApi::CFloatPtr pExposureTimeNode = pNodeMap->GetNode("ExposureTime");

    //動画像として再生
    OpencvVideo(pDevice, pNodeMap);

    //// 画像データの取得
    //std::cout << "画像データを1枚取得\n";
    //IImage* pImage = pDevice->GetImage(TIMEOUT);

    //// RAW画像から偏光データを抽出
    //std::cout << "偏光データの抽出\n";

    //std::unique_ptr<unsigned short> deg0(new unsigned short[(WIDTH / 2 ) * ( HEIGHT / 2 )]);
    //std::unique_ptr<unsigned short> deg45(new unsigned short[(WIDTH / 2 ) * ( HEIGHT / 2 )]);
    //std::unique_ptr<unsigned short> deg90(new unsigned short[(WIDTH / 2 ) * ( HEIGHT / 2 )]);
    //std::unique_ptr<unsigned short> deg135(new unsigned short[(WIDTH / 2 ) * ( HEIGHT / 2 )]);

    //GetPolarizeData(pImage, deg0.get(), deg45.get(), deg90.get(), deg135.get());

    //pDevice->RequeueBuffer(pImage);

    //// ストリームの停止
    //std::cout << "ストリームの停止\n";
    //pDevice->StopStream();
}

//デバイスとの接続，IDevice* pDeviceの生成
int main(){
    std::cout << "カラー偏光カメラ Console Sample\n";

    try{
        ISystem* pSystem = OpenSystem();
        pSystem->UpdateDevices(100);
        std::vector<DeviceInfo> deviceInfos = pSystem->GetDevices();

        if (deviceInfos.size() == 0){
            throw GenICam::GenericException( "カメラが接続されていません。", __FILE__, __LINE__ );
        }

        IDevice* pDevice = pSystem->CreateDevice(deviceInfos[0]);

        AcquireImage(pDevice);

        pSystem->DestroyDevice(pDevice);
        CloseSystem(pSystem);
    }
    catch (GenICam::GenericException& ge){
        std::cout << "\nGenICam 例外がスローされました : " << ge.what() << "\n";
        return -1;
    }
    catch (std::exception& ex){
        std::cout << "\n標準例外がスローされました : " << ex.what() << "\n";
        return -1;
    }
    catch (...){
        std::cout << "\n未定義の例外がスローされました\n";
        return -1;
    }

    return 0;
}

