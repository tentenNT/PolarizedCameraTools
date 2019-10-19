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

void CaptureImage(const cv::Mat& img, IImage* pImage, const int& key, const std::string& choice, int& count){
        if(key == 's'){
            std::ofstream f_raw, f_csv;
            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_PXM_BINARY);
            compression_params.push_back(0);
            cv::imwrite(choice + std::to_string(count) + ".png", img);
            cv::imwrite(choice + std::to_string(count) + ".pgm", img, compression_params);
            cv::imwrite(choice + std::to_string(count) + ".ppm", img, compression_params);
            f_raw.open(choice + std::to_string(count) + ".raw", std::ios::binary);
            f_raw.write((char*)pImage->GetData(), pImage->GetSizeOfBuffer());
            f_raw.close();
            // 多分csvへの書き込みが一番重い
            // 必要になったら適当なコンテナにcv::Matを押し込んでいこう
            f_csv.open(choice + std::to_string(count) + ".csv");
            f_csv << cv::format(img, cv::Formatter::FMT_CSV) << std::endl;
            f_csv.close();
            std::cout << "captured: " << std::to_string(count) << std::endl;
            count++;
        }
}

void RunOpencvPolarizedVideo(IDevice* pDevice, GenApi::INodeMap* pNodeMap, Polarized pol_chunk, std::string choice, int streaming_time){
    IImage* pImage;
    // ちゃんとインスタンス化する
    cv::Mat img(pol_chunk.rows, pol_chunk.cols, CV_32FC1);
    int key = 0;
    int count = 0;
    double framesPerSecond = GetNodeValue<double>(pNodeMap, "AcquisitionFrameRate");

    for (int i = 0; i < framesPerSecond * streaming_time;  i++){
        pImage = pDevice->GetImage(TIMEOUT);
        // 偏光データを取得
        pol_chunk.GetPolarizedData(pImage);
        // cv::Mat型に変換
        pol_chunk.TranslatePointerToMatrix(pImage);
        pol_chunk.CalculateIntensity();
        if(ChoiceImage(img, pol_chunk, choice)){
            break;
        }
        img.convertTo(img, CV_8UC1); // imgがCV_8UC3でも問題なし？
        // cv::GaussianBlur(img, img, cv::Size(3, 3), 0, 0);
        // cv::medianBlur(img, img, 3);
        // img.convertTo(img, CV_16SC1); // どうも見た目では違いが分からない
        // cv::Sobel(img, img, CV_16S, 2, 2, 7);
        // img.convertTo(img, CV_8UC1); // imgがCV_8UC3でも問題なし？
        // cv::Scharr(img, img, CV_8U, 1, 0);
        // cv::Mat element;
        // cv::erode(img, img, element, cv::Point(-1, -1), 1);
        // cv::dilate(img, img, element, cv::Point(-1, -1), 2);
        // cv::morphologyEx(img, img, cv::MORPH_CLOSE, element, cv::Point(-1, -1), 2);
        // cv::morphologyEx(img, img, cv::MORPH_CLOSE, element, cv::Point(-1, -1), 2);
//        cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
//        cv::Canny(img, img, 10, 50);
        cv::imshow("polarizeddata", img);
        key = cv::waitKey(WAITTIME);
        CaptureImage(img, pImage, key, choice, count);
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
    int streaming_time;
    std::cout << "再生時間を入力して下さい" << std::endl;
    std::cin >> streaming_time;
    RunOpencvPolarizedVideo(pDevice, pNodeMap, pol_chunk, choice, streaming_time);
    std::cout << "ストリームの停止\n";
    pDevice->StopStream();
}

