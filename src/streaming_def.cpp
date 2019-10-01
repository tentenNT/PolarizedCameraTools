#include "streaming_def.hpp"

// 偏光データを再生
void RunOpencvPolarizedVideo(IDevice* pDevice, GenApi::INodeMap* pNodeMap, Polarized pol_chunk, std::string choice){

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
        if(choice == "I_max_color"){
            pol_chunk.I_max.convertTo(pol_chunk.I_max, CV_8UC1);
            cv::cvtColor(pol_chunk.I_max, pol_chunk.I_max, cv::COLOR_BayerBG2BGR);
            ApplyWhiteBalance(pol_chunk.I_max);
            cv::imshow("polarizeddata", pol_chunk.I_max);
        }
        else if(choice == "I_min"){
            pol_chunk.I_min.convertTo(pol_chunk.I_min, CV_8UC1);
            cv::imshow("polarizeddata", pol_chunk.I_min);
        }
        else if(choice == "I_min_color"){
            pol_chunk.I_min.convertTo(pol_chunk.I_min, CV_8UC1);
            cv::cvtColor(pol_chunk.I_min, pol_chunk.I_min, cv::COLOR_BayerBG2BGR);
            ApplyWhiteBalance(pol_chunk.I_min);
            cv::imshow("polarizeddata", pol_chunk.I_min);
        }
        else if(choice == "I_max-I_min"){
            cv::Mat img = pol_chunk.I_max - pol_chunk.I_min; // 何故かI_max, I_minをコンバートして計算すると駄目だった
            img.convertTo(img, CV_8UC1);
            cv::imshow("polarizeddata", img);
        }
        else if(choice == "45"){
            //cv::cvtColor(pol_chunk.deg45_mat, pol_chunk.deg45_mat, cv::COLOR_BayerBG2BGR);
            cv::demosaicing(pol_chunk.deg45_mat, pol_chunk.deg45_mat, cv::COLOR_BayerBG2BGR);
            ApplyWhiteBalance(pol_chunk.deg45_mat);
            cv::imshow("polarizeddata", pol_chunk.deg45_mat);
        }
        // 出力が変なような
        else if(choice == "R"){
            pol_chunk.R.convertTo(pol_chunk.R, CV_8UC1);
            cv::imshow("polarizeddata", pol_chunk.R);
        }
        else if(choice == "rho"){
            pol_chunk.CalculateDoLP();
            pol_chunk.rho.convertTo(pol_chunk.rho, CV_8UC1, 255);
            cv::imshow("polarizeddata", pol_chunk.rho);
        }
        else if(choice == "theta"){
            pol_chunk.CalculateAoLP();
            //pol_chunk.theta.convertTo(pol_chunk.theta, CV_8UC1, 255 / M_PI);
            pol_chunk.ConvertAoLPmonoToHSV();
            cv::imshow("polarizeddata", pol_chunk.theta_c3);
            //cv::imshow("polarizeddata", pol_chunk.theta);
        }

        pDevice->RequeueBuffer(pImage);
        cv::waitKey(WAITTIME);
    }
    cv::destroyAllWindows();
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
    RunOpencvPolarizedVideo(pDevice, pNodeMap, pol_chunk, "45");

    // ストリームの停止
    std::cout << "ストリームの停止\n";
    pDevice->StopStream();
}

