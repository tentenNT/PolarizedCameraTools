#include "polarized.hpp"

void Polarized::GetPolarizedData(IImage* pImage){
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
void Polarized::TranslatePointerToMatrix(IImage* pImage){
    int rows = (int)pImage->GetHeight() / 2;
    int cols = (int)pImage->GetWidth() / 2;
    // uchar型
    deg0_mat = cv::Mat(rows, cols, CV_8UC1, pDeg0);
    deg45_mat = cv::Mat(rows, cols, CV_8UC1, pDeg45);
    deg90_mat = cv::Mat(rows, cols, CV_8UC1, pDeg90);
    deg135_mat = cv::Mat(rows, cols, CV_8UC1, pDeg135);
    // float型
    deg0_mat.convertTo(deg0_mat_f, CV_32F);
    deg45_mat.convertTo(deg45_mat_f, CV_32F);
    deg90_mat.convertTo(deg90_mat_f, CV_32F);
    deg135_mat.convertTo(deg135_mat_f, CV_32F);
}


// I_max, I_minを計算
void Polarized::CalculateIntensity(){
    //CV_8UC1は0~255だがCV_32FC1は0~1.0を取るので注意する必要がある
    C_1 = deg0_mat_f - deg90_mat_f;
    C_2 = deg135_mat_f - deg45_mat_f;
    cv::magnitude(C_1, C_2, R);
    I_a = R / 2;
    I_b = (deg0_mat_f + deg45_mat_f + deg90_mat_f + deg135_mat_f) / 4;
    I_max = I_b + I_a;
    I_min = I_b - I_a;
}

// rho（DoLP, 偏光度）を計算
void Polarized::CalculateDoLP(){
    rho = I_a / I_b;
}

// theta（AoLP，偏光角）を計算
void Polarized::CalculateAoLP(){
    theta = C_1 / (2*I_a);
    theta.forEach<float>([](float &p, const int * position) -> void{
        p = acos(p);
    });
    // theta = theta / 2; // なぜ偏光角は0~180の間にある筈なのに2で割るのだろうか？
}

void Polarized::ConvertAoLPmonoToHSV(){
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
    // 切り捨てだと思ったら四捨五入してる
    theta.convertTo(theta, CV_8UC1);
    const std::vector<cv::Mat> mv{theta, mat_s, mat_v};
    cv::merge(mv, mat_hsv);
    cv::cvtColor(mat_hsv, mat_hsv, cv::COLOR_HSV2BGR);
    theta_c3 = mat_hsv;
}

