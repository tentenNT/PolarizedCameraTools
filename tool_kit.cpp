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
#define TIMEOUT                                 2000
#define STREAMINGTIME                         100
#define WAITTIME                                1

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

    int rows, cols;

    //コンストラクタ
    Polarized(IDevice* pDevice, void* pDeg0, void* pDeg45, void* pDeg90, void* pDeg135)
        : pDeg0(pDeg0), pDeg45(pDeg45), pDeg90(pDeg90), pDeg135(pDeg135){
        IImage* pImage;
        pImage = pDevice->GetImage(TIMEOUT);
        rows = (int)pImage->GetHeight() / 2;
        cols = (int)pImage->GetWidth() / 2;
        pDevice->RequeueBuffer(pImage); // いらないかも
        std::cout << rows << "\n" << cols << std::endl;
 }

    void GetPolarizedData(IImage* pImage){
        uint8_t* inputBuff = const_cast<uint8_t*>(pImage->GetData());
        uint8_t* src = inputBuff;
        // このようにする事でpDegは変化せずdestのポインタの指す位置が変化し続ける
        unsigned char* dest0 = (unsigned char*)pDeg0;
        unsigned char* dest45 = (unsigned char*)pDeg45;
        unsigned char* dest90 = (unsigned char*)pDeg90;
        unsigned char* dest135 = (unsigned char*)pDeg135;

        // 偏光データを取り出しメモリに格納
        for (int y = 0; y < HEIGHT; y += 2){
            uint8_t* srcImage = src + y * WIDTH;
            for (int x = 0; x < WIDTH; x += 2){
                // *i++の場合，*iの処理が終わった後，ポインタのインクリメントが行われる
                // *(i++)も同様
                *dest90++ = *srcImage;
                *dest45++ = *(srcImage + 1);
                *dest135++ = *(srcImage + WIDTH);
                *dest0++ = *(srcImage + WIDTH + 1);
                srcImage += 2;
            }
        }
    }

    // ポインタからcv::Matへ変換
    void TranslatePointerToMatrix(IImage* pImage){
        int rows = (int)pImage->GetHeight() / 2;
        int cols = (int)pImage->GetWidth() / 2;
        deg0_mat = cv::Mat(rows, cols, CV_8UC1, pDeg0);
        deg45_mat = cv::Mat(rows, cols, CV_8UC1, pDeg45);
        deg90_mat = cv::Mat(rows, cols, CV_8UC1, pDeg90);
        deg135_mat = cv::Mat(rows, cols, CV_8UC1, pDeg135);
    }

    // I_max, I_minを計算
    // 数式そのままなのでここでは命名規則は無視している
    void CalculateIntensity(){
        deg0_mat.convertTo(deg0_mat_f, CV_32F);
        deg45_mat.convertTo(deg45_mat_f, CV_32F);
        deg90_mat.convertTo(deg90_mat_f, CV_32F);
        deg135_mat.convertTo(deg135_mat_f, CV_32F);

        C_1 = deg0_mat_f - deg90_mat_f;
        C_2 = deg135_mat_f - deg45_mat_f;
        cv::magnitude(C_1, C_2, R);
        I_a = R / 2;
        I_b = (deg0_mat_f + deg45_mat_f + deg90_mat_f + deg135_mat_f) / 4;
        I_max = I_b + I_a;
        I_min = I_b - I_a;
    }

    // rho（DoLP, 偏光度）を計算
    void CalculateDoLP(){
        rho = I_a / I_b;
    }
    // theta（AoLP，偏光角）を計算
    void CalculateAoLP(){
        theta = C_1 / (2*I_a);
        theta.forEach<float>([](float &p, const int * position) -> void{
            p = acos(p);
        });
        // theta = theta / 2; // なぜ偏光角は0~180の間にある筈なのに2で割るのだろうか？
    }

    void ConvertAoLPmonoToHSV(){
        cv::Mat mat_hsv(rows, cols, CV_8UC3);
        cv::Mat mat_s(rows, cols, CV_8UC1);
        cv::Mat mat_v(rows, cols, CV_8UC1);
        //mat_c3.forEach<cv::Point3_<uint8_t>>([](cv::Point3_<uint8_t> &p, const int * position) -> void{
        //    p.y = 255;
        //    p.z = 255;
        //});
        mat_s.forEach<uint8_t>([](uint8_t &p, const int * position) -> void{
            p = 255;
        });
        mat_v = mat_s;
        theta = theta * 180/ M_PI;
        theta.convertTo(theta, CV_8UC1);
        const std::vector<cv::Mat> mv{theta, mat_s, mat_v};
        cv::merge(mv, mat_hsv);
        cv::cvtColor(mat_hsv, mat_hsv, cv::COLOR_HSV2BGR);
        theta_c3 = mat_hsv;
    }
};



//// OpenCVで動画像を取得
//void RunOpencvVideo(IDevice* pDevice, GenApi::INodeMap* pNodeMap){
//    gcstring windowName = GetNodeValue<GenICam::gcstring>(pNodeMap, "DeviceModelName") + " (" + GetNodeValue<GenICam::gcstring>(pNodeMap, "DeviceSerialNumber") + ")";
//
//    IImage *pImage;
//    cv::Mat img;
//
//    double framesPerSecond = GetNodeValue<double>(pNodeMap, "AcquisitionFrameRate");
//
//    for (int i = 0; i < framesPerSecond * STREAMINGTIME;  i++){
//        pImage = pDevice->GetImage(TIMEOUT);
//        img = cv::Mat((int)pImage->GetHeight(), (int)pImage->GetWidth(), CV_8UC1, (void *)pImage->GetData());
//        //イメージのリサイズ
//        cv::resize(img, img, cv::Size(), 0.5, 0.5);
//
//        cv::imshow(windowName.c_str(), img);
//        pDevice->RequeueBuffer(pImage);
//        cv::waitKey(WAITTIME);
//    }
//    cv::destroyAllWindows();
//}

// 偏光データを再生
void RunOpencvPolarizedVideo(IDevice* pDevice, GenApi::INodeMap* pNodeMap, Polarized pol_chunk, std::string choice = "I_max"){

    IImage* pImage;
    cv::Mat img;
    double framesPerSecond = GetNodeValue<double>(pNodeMap, "AcquisitionFrameRate");

    //for文の中にif文が入ってしまっているのは馬鹿馬鹿しいので後々直す
    //https://codeday.me/jp/qa/20181218/28477.html
    //ファンクタ（関数オブジェクト）を使えば良いらしい
    for (int i = 0; i < framesPerSecond * STREAMINGTIME;  i++){
        pImage = pDevice->GetImage(TIMEOUT);
        // 偏光データを取得
        pol_chunk.GetPolarizedData(pImage);
        // cv::Mat型に変換
        pol_chunk.TranslatePointerToMatrix(pImage);
        pol_chunk.CalculateIntensity();
        if(choice == "I_max"){
            pol_chunk.I_max.convertTo(pol_chunk.I_max, CV_8UC1);
            cv::imshow("polarizeddata", pol_chunk.I_max);
        }
        else if(choice == "I_min"){
            pol_chunk.I_min.convertTo(pol_chunk.I_min, CV_8UC1);
            cv::imshow("polarizeddata", pol_chunk.I_min);
        }
        else if(choice == "rho"){
            pol_chunk.CalculateDoLP();
            pol_chunk.rho.convertTo(pol_chunk.rho, CV_8UC1, 255);
            cv::imshow("polarizeddata", pol_chunk.rho);
        }
        else if(choice == "theta"){
            pol_chunk.CalculateAoLP();
            pol_chunk.theta.convertTo(pol_chunk.theta, CV_8UC1, 255 / M_PI);
            //pol_chunk.ConvertAoLPmonoToHSV();
            //cv::imshow("polarizeddata", pol_chunk.theta_c3);
            cv::imshow("polarizeddata", pol_chunk.theta);
        }

        pDevice->RequeueBuffer(pImage);
        cv::waitKey(WAITTIME);
    }
    cv::destroyAllWindows();
}
    //for (int i = 0; i < framesPerSecond * STREAMINGTIME;  i++){
    //    pImage = pDevice->GetImage(TIMEOUT);
    //    // 偏光データを取得
    //    pol_chunk.GetPolarizedData(pImage);
    //    // cv::Mat型に変換
    //    pol_chunk.TranslatePointerToMatrix(pImage);
    //    pol_chunk.CalculateIntensity();
    //    pol_chunk.I_max.convertTo(pol_chunk.I_max, CV_8UC1);
    //    // イメージのリサイズ
    //    // cv::resize(img, img, cv::Size(), 0.5, 0.5);
    //    cv::imshow("polarizeddata", pol_chunk.I_max);
    //    pDevice->RequeueBuffer(pImage);
    //    cv::waitKey(WAITTIME);
    //}
    //cv::destroyAllWindows();

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

    //Rawデータを動画像として再生
    //RunOpencvVideo(pDevice, pNodeMap);

    // 画像データの取得
    //std::cout << "画像データを1枚取得\n";
    //IImage* pImage = pDevice->GetImage(TIMEOUT);

    // RAW画像から偏光データを抽出
    std::cout << "偏光データの抽出\n";
    //const unique_ptrだと管理対象オブジェクトの生存期間が作成されたスコープに限定されてしまう
    std::unique_ptr<unsigned short> deg0(new unsigned short[(WIDTH / 2 ) * ( HEIGHT / 2 )]);
    std::unique_ptr<unsigned short> deg45(new unsigned short[(WIDTH / 2 ) * ( HEIGHT / 2 )]);
    std::unique_ptr<unsigned short> deg90(new unsigned short[(WIDTH / 2 ) * ( HEIGHT / 2 )]);
    std::unique_ptr<unsigned short> deg135(new unsigned short[(WIDTH / 2 ) * ( HEIGHT / 2 )]);

    // Polarizedクラスのインスタンスを生成
    Polarized pol_chunk(pDevice, deg0.get(), deg45.get(), deg90.get(), deg135.get());
    
    //偏光動画像の再生
    RunOpencvPolarizedVideo(pDevice, pNodeMap, pol_chunk, "theta");

    // ストリームの停止
    std::cout << "ストリームの停止\n";
    pDevice->StopStream();
}

// デバイスとの接続，IDevice* pDeviceの生成
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


