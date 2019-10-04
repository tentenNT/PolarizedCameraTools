#include "streaming.hpp"
#include "general_image_processing.hpp"
#include "testcode.hpp"

// パラメータ変えるたびにビルドし直すのはあほだからconfigファイルを用意しよう

int ChoiceImage(cv::Mat& img, Polarized pol_chunk, const std::string &choice){
    if(choice == "0" || choice == "0_color"){
        img = pol_chunk.deg0_mat;
        if(choice == "0_color"){
            RawdataToColor(img);
        }
    }
    else if(choice == "45" || choice == "45_color"){
        img = pol_chunk.deg45_mat;
        if(choice == "45_color"){
            RawdataToColor(img);
        }
    }
    else if(choice == "90" || choice == "90_color"){
        img = pol_chunk.deg90_mat;
        if(choice == "90_color"){
            RawdataToColor(img);
        }
    }
    else if(choice == "135" || choice == "135_color"){
        img = pol_chunk.deg135_mat;
        if(choice == "135_color"){
            RawdataToColor(img);
        }
    }

    
    else if(choice == "I_max" || choice == "I_max_color"){
        img = pol_chunk.I_max;
        if(choice == "I_max_color"){
            RawdataToColor(img);
        }
    }
    else if(choice == "I_min" || choice == "I_min_color"){
        img = pol_chunk.I_min;
        if(choice == "I_min_color"){
            RawdataToColor(img);
        }
    }
    else if(choice == "I_max-I_min"){
        img = pol_chunk.I_max - pol_chunk.I_min; // 何故かI_max, I_minをコンバートして計算すると駄目だった
    }

    //出力が変なような
    else if(choice == "R"){
        img = pol_chunk.R;
    }
    else if(choice == "I_a"){
        img = pol_chunk.I_a;
    }
    // 輝度の平均
    else if(choice == "I_b" || choice == "I_b_color"){
        img = pol_chunk.I_b;
        if(choice == "I_b_color"){
            RawdataToColor(img);
        }
    }

    else if(choice == "rho"){
        pol_chunk.CalculateDoLP();
        img = pol_chunk.rho;
        img.convertTo(img, CV_8UC1, 255);
    }
    else if(choice == "theta" || choice == "theta_color"){
        pol_chunk.CalculateAoLP();
        if (choice == "theta"){
            img = pol_chunk.theta;
            img.convertTo(img, CV_8UC1, 255 / M_PI);
        }
        else if(choice == "theta_color"){
            pol_chunk.ConvertAoLPmonoToHSV();
            img = pol_chunk.theta_c3;
        }
    }

    else{
        std::cout << "有効な名前を入力して下さい" << std::endl;
        return -1;
    }
    return 0;
}


void RunOpencvPolarizedVideo(IDevice* pDevice, GenApi::INodeMap* pNodeMap, Polarized pol_chunk, std::string choice){
    IImage* pImage;
    // ちゃんとインスタンス化する
    cv::Mat img(pol_chunk.rows, pol_chunk.cols, CV_32FC1);
    int key = 0;
    int count = 0;
    double framesPerSecond = GetNodeValue<double>(pNodeMap, "AcquisitionFrameRate");

    for (int i = 0; i < framesPerSecond * STREAMINGTIME;  i++){
        pImage = pDevice->GetImage(TIMEOUT);
        // 偏光データを取得
        pol_chunk.GetPolarizedData(pImage);
        // cv::Mat型に変換
        pol_chunk.TranslatePointerToMatrix(pImage);
        pol_chunk.CalculateIntensity();
        if(ChoiceImage(img, pol_chunk, choice)){
            break;
        }
        img.convertTo(img, CV_8UC1);
        cv::imshow("polarizeddata", img);
        key = cv::waitKey(WAITTIME);
        if(key == 's'){
            cv::imwrite(choice + std::to_string(count) + ".png", img);
            std::cout >> "captured: " >> std::to_string(count) >> std::endl;
            count++
        }
        pDevice->RequeueBuffer(pImage);
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
    std::string choice;
    std::cout << "見たい画像の形式を入力して下さい" << std::endl;
    std::cin >> choice;
    RunOpencvPolarizedVideo(pDevice, pNodeMap, pol_chunk, choice);
    // ストリームの停止
    RunOpencvPolarizedVideo(pDevice, pNodeMap, pol_chunk, "I_min");
    std::cout << "ストリームの停止\n";
    pDevice->StopStream();
}

