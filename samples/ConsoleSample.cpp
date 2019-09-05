// ConsoleSample.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//
/****************************************
 ***                                  ***
 *** Copyright(C) 2018, ViewPLUS Inc. ***
 *** Version 0.8.0                    ***
 ***                                  ***
 ***************************************/

#include "stdafx.h"
#include "ArenaApi.h"
#include "SaveApi.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <string>

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

using namespace Arena;

// このサンプルはカメラから偏光データを取得し、そのままRAWでファイル保存します。
// また、偏光データ毎にデモザイキングしたデータもファイル保存します。

int GetPolarizeData( IImage* pImage, void* pDeg0, void* pDeg45, void* pDeg90, void* pDeg135 )
{
        const uint32_t pixelFormat = static_cast< uint32_t >( pImage->GetPixelFormat() );
        uint8_t* inputBuff = const_cast< uint8_t* >( pImage->GetData() );

        if ( pixelFormat == LUCID_PolarizeMono8 )
        {
                uint8_t* src = inputBuff;
                unsigned char* dest0 = ( unsigned char* )pDeg0;
                unsigned char* dest45 = ( unsigned char* )pDeg45;
                unsigned char* dest90 = ( unsigned char* )pDeg90;
                unsigned char* dest135 = ( unsigned char* )pDeg135;

                // 偏光データを取り出しメモリに格納
                for ( int y = 0; y < HEIGHT; y += 2 )
                {
                        uint8_t* srcImage = src + y * WIDTH;

                        for ( int x = 0; x < WIDTH; x += 2 )
                        {
                                *dest90++ = *srcImage;
                                *dest45++ = *( srcImage + 1 );
                                *dest135++ = *( srcImage + WIDTH );
                                *dest0++ = *( srcImage + WIDTH + 1 );

                                srcImage += 2;
                        }
                }

                return 0;
        }
        else if ( pixelFormat == LUCID_PolarizeMono16 )
        {
                uint16_t* src = reinterpret_cast< uint16_t* >( inputBuff );
                unsigned short* dest0 = ( unsigned short* )pDeg0;
                unsigned short* dest45 = ( unsigned short* )pDeg45;
                unsigned short* dest90 = ( unsigned short* )pDeg90;
                unsigned short* dest135 = ( unsigned short* )pDeg135;

                // 偏光データを取り出しメモリに格納
                for ( int y = 0; y < HEIGHT; y += 2 )
                {
                        uint16_t* srcImage = src + y * WIDTH;

                        for ( int x = 0; x < WIDTH; x += 2 )
                        {
                                *dest90++ = *srcImage;
                                *dest45++ = *( srcImage + 1 );
                                *dest135++ = *( srcImage + WIDTH );
                                *dest0++ = *( srcImage + WIDTH + 1 );

                                srcImage += 2;
                        }
                }

                return 0;
        }
        else return -1;
}

void ApplyWhiteBalanceBayerRGGB8( unsigned char* dest, int width, int height, double rGain, double gGain, double bGain )
{
        uint32_t value;
        unsigned int bGain2 = ( int )( bGain * 0xFF );
        unsigned int gGain2 = ( int )( gGain * 0xFF );
        unsigned int rGain2 = ( int )( rGain * 0xFF );

        for ( int y = 0; y < height; y += 2 )
        {
                for ( int x = 0; x < width; x += 2 )
                {
                        value = rGain2 * *dest; //R
                        if ( value > 0xFFFF )
                                *dest++ = 0xFF;
                        else
                                *dest++ = value >> 8;

                        value = gGain2 * *dest; //G
                        if ( value > 0xFFFF )
                                *dest++ = 0xFF;
                        else
                                *dest++ = value >> 8;
                }

                for ( int x = 0; x < width; x += 2 )
                {
                        value = gGain2 * *dest; //G
                        if ( value > 0xFFFF )
                                *dest++ = 0xFF;
                        else
                                *dest++ = value >> 8;

                        value = bGain2 * *dest; //B
                        if ( value > 0xFFFF )
                                *dest++ = 0xFF;
                        else
                                *dest++ = value >> 8;
                }
        }
}

void ApplyWhiteBalanceBayerRGGB16( unsigned short* dest, int width, int height, double rGain, double gGain, double bGain )
{
        uint32_t value;
        unsigned int bGain2 = ( int )( bGain * 0xFF );
        unsigned int gGain2 = ( int )( gGain * 0xFF );
        unsigned int rGain2 = ( int )( rGain * 0xFF );

        for ( int y = 0; y < height; y += 2 )
        {
                for ( int x = 0; x < width; x += 2 )
                {
                        value = rGain2 * *dest; //R
                        if ( value > 0xFFFFFF )
                                *dest++ = 0xFFFF;
                        else
                                *dest++ = value >> 8;

                        value = gGain2 * *dest; //G
                        if ( value > 0xFFFFFF )
                                *dest++ = 0xFFFF;
                        else
                                *dest++ = value >> 8;
                }

                for ( int x = 0; x < width; x += 2 )
                {
                        value = gGain2 * *dest; //G
                        if ( value > 0xFFFFFF )
                                *dest++ = 0xFFFF;
                        else
                                *dest++ = value >> 8;

                        value = bGain2 * *dest; //B
                        if ( value > 0xFFFFFF )
                                *dest++ = 0xFFFF;
                        else
                                *dest++ = value >> 8;
                }
        }
}

void AcquireImage( IDevice* pDevice )
{
        // Acquisition ModeをContinuousに設定
        std::cout << "Acquisition modeを'Continuous'に設定\n";
        SetNodeValue<GenICam::gcstring>( pDevice->GetNodeMap(), "AcquisitionMode", "Continuous" );

        // Pixel Formatの選択
        std::cout << "1)8bit\n";
        std::cout << "2)16bit\n";
        std::cout << "ピクセルフォーマットを選択してください:";
//        char selPixelFormat = _getche();
        char selPixelFormat;
        std::cin >> selPixelFormat;
        std::cout << "\n\n";

        // Pixel Formatの設定
        if ( selPixelFormat == '1' )
        {
                std::cout << "ピクセルフォーマットを<8bit>に設定\n";
                SetNodeValue<GenICam::gcstring>( pDevice->GetNodeMap(), "PixelFormat", "PolarizeMono8" );
        }
        else if ( selPixelFormat == '2' )
        {
                std::cout << "ピクセルフォーマットを<16bit>に設定\n";
                SetNodeValue<GenICam::gcstring>( pDevice->GetNodeMap(), "PixelFormat", "PolarizeMono16" );
        }
        else
        {
                throw GenICam::GenericException( "選択が正しくありません。", __FILE__, __LINE__ );
        }

        // ストリームの開始
        std::cout << "ストリームの開始\n";
        pDevice->StartStream();

        // 画像データの取得
        std::cout << "画像データを1枚取得\n";
        IImage* pImage = pDevice->GetImage( 2000 );

        std::ofstream fout;

        // そのままファイルに保存
        std::cout << "RAWファイル保存\n";
        fout.open( FILE_NAME, std::ios::binary );
        fout.write( ( char* )pImage->GetData(), pImage->GetSizeOfBuffer() );
        fout.close();

        // RAW画像から偏光データを抽出
        std::cout << "偏光データの抽出\n";

        std::unique_ptr<unsigned short> deg0( new unsigned short[ ( WIDTH / 2 ) * ( HEIGHT / 2 ) ] );
        std::unique_ptr<unsigned short> deg45( new unsigned short[ ( WIDTH / 2 ) * ( HEIGHT / 2 ) ] );
        std::unique_ptr<unsigned short> deg90( new unsigned short[ ( WIDTH / 2 ) * ( HEIGHT / 2 ) ] );
        std::unique_ptr<unsigned short> deg135( new unsigned short[ ( WIDTH / 2 ) * ( HEIGHT / 2 ) ] );

        GetPolarizeData( pImage, deg0.get(), deg45.get(), deg90.get(), deg135.get() );

        pDevice->RequeueBuffer( pImage );

        // ストリームの停止
        std::cout << "ストリームの停止\n";
        pDevice->StopStream();

        // 偏光データを各ファイルに保存
        std::cout << "偏光データ毎にファイルに保存\n";

        std::string filename[] = { FILE_NAME_0DEG, FILE_NAME_45DEG, FILE_NAME_90DEG, FILE_NAME_135DEG };
        const char* image[] = { ( const char* )deg0.get(), ( const char* )deg45.get(), ( const char* )deg90.get(), ( const char* )deg135.get() };
        std::streamsize size = ( WIDTH / 2 ) * ( HEIGHT / 2 );
        if ( selPixelFormat == '2' )
                size *= 2;

        for ( int i = 0; i < 4; i++ )
        {
                fout.open( filename[ i ], std::ios::binary );
                fout.write( image[i], size );
                fout.close();
        }

        // フィルター毎のBayerデータをデモザイキングしてファイル保存
        std::cout << "フィルター毎にデモザイキングしてpngファイルに保存\n";

        // ホワイトバランスをとる
        std::cout << "ホワイトバランス適用\n";
        double rGain = RED_GAIN;
        double gGain = GREEN_GAIN;
        double bGain = BLUE_GAIN;

        size_t dataSize;
        PfncFormat pixelFormat;

        if ( selPixelFormat == '1' )
        {
                dataSize = ( WIDTH / 2 ) * ( HEIGHT / 2 );
                pixelFormat = BayerRG8;

                ApplyWhiteBalanceBayerRGGB8( ( unsigned char* )deg0.get(), WIDTH / 2, HEIGHT / 2, rGain, gGain, bGain );
                ApplyWhiteBalanceBayerRGGB8( ( unsigned char* )deg45.get(), WIDTH / 2, HEIGHT / 2, rGain, gGain, bGain );
                ApplyWhiteBalanceBayerRGGB8( ( unsigned char* )deg90.get(), WIDTH / 2, HEIGHT / 2, rGain, gGain, bGain );
                ApplyWhiteBalanceBayerRGGB8( ( unsigned char* )deg135.get(), WIDTH / 2, HEIGHT / 2, rGain, gGain, bGain );
        }
        else if ( selPixelFormat == '2' )
        {
                dataSize = ( WIDTH / 2 ) * ( HEIGHT / 2 ) * 2;
                pixelFormat = BayerRG16;

                ApplyWhiteBalanceBayerRGGB16( deg0.get(), WIDTH / 2, HEIGHT / 2, rGain, gGain, bGain );
                ApplyWhiteBalanceBayerRGGB16( deg45.get(), WIDTH / 2, HEIGHT / 2, rGain, gGain, bGain );
                ApplyWhiteBalanceBayerRGGB16( deg90.get(), WIDTH / 2, HEIGHT / 2, rGain, gGain, bGain );
                ApplyWhiteBalanceBayerRGGB16( deg135.get(), WIDTH / 2, HEIGHT / 2, rGain, gGain, bGain );
        }

        // デモザイキングのためにベイヤーイメージをセット
        IImage* pBayer0 = ImageFactory::Create( ( const uint8_t* )deg0.get(), dataSize, WIDTH / 2, HEIGHT / 2, pixelFormat );
        IImage* pBayer45 = ImageFactory::Create( ( const uint8_t* )deg45.get(), dataSize, WIDTH / 2, HEIGHT / 2, pixelFormat );
        IImage* pBayer90 = ImageFactory::Create( ( const uint8_t* )deg90.get(), dataSize, WIDTH / 2, HEIGHT / 2, pixelFormat );
        IImage* pBayer135 = ImageFactory::Create( ( const uint8_t* )deg135.get(), dataSize, WIDTH / 2, HEIGHT / 2, pixelFormat );

        // デモザイキング
        IImage* pConverted0 = ImageFactory::Convert( pBayer0, BGR8 );
        IImage* pConverted45 = ImageFactory::Convert( pBayer45, BGR8 );
        IImage* pConverted90 = ImageFactory::Convert( pBayer90, BGR8 );
        IImage* pConverted135 = ImageFactory::Convert( pBayer135, BGR8 );

        // デモザイキング画像をファイル保存
        Save::ImageWriter writer;
        Save::ImageParams params;

        // 0deg
        std::cout << "0度のカラーデータ保存...";
        params.SetWidth( pConverted0->GetWidth() );
        params.SetHeight( pConverted0->GetHeight() );
        params.SetBitsPerPixel( pConverted0->GetBitsPerPixel() );

        writer.SetParams( params );
        writer.SetFileNamePattern( FILE_NAME_0DEG_BGR );
        writer << pConverted0->GetData();
        std::cout << "完了\n";

        // 45deg
        std::cout << "45度のカラーデータ保存...";
        params.SetWidth( pConverted45->GetWidth() );
        params.SetHeight( pConverted45->GetHeight() );
        params.SetBitsPerPixel( pConverted45->GetBitsPerPixel() );

        writer.SetParams( params );
        writer.SetFileNamePattern( FILE_NAME_45DEG_BGR );
        writer << pConverted45->GetData();
        std::cout << "完了\n";

        // 90deg
        std::cout << "90度のカラーデータ保存...";
        params.SetWidth( pConverted90->GetWidth() );
        params.SetHeight( pConverted90->GetHeight() );
        params.SetBitsPerPixel( pConverted90->GetBitsPerPixel() );

        writer.SetParams( params );
        writer.SetFileNamePattern( FILE_NAME_90DEG_BGR );
        writer << pConverted90->GetData();
        std::cout << "完了\n";

        // 135deg
        std::cout << "135度のカラーデータ保存...";
        params.SetWidth( pConverted135->GetWidth() );
        params.SetHeight( pConverted135->GetHeight() );
        params.SetBitsPerPixel( pConverted135->GetBitsPerPixel() );

        writer.SetParams( params );
        writer.SetFileNamePattern( FILE_NAME_135DEG_BGR );
        writer << pConverted135->GetData();
        std::cout << "完了\n";

        // イメージメモリの破棄
        ImageFactory::Destroy( pConverted90 );
        ImageFactory::Destroy( pConverted45 );
        ImageFactory::Destroy( pConverted135 );
        ImageFactory::Destroy( pConverted0 );

        ImageFactory::Destroy( pBayer0 );
        ImageFactory::Destroy( pBayer45 );
        ImageFactory::Destroy( pBayer90 );
        ImageFactory::Destroy( pBayer135 );
}

int main()
{
        std::cout << "カラー偏光カメラ Console Sample\n";

        try
        {
                ISystem* pSystem = OpenSystem();
                pSystem->UpdateDevices( 100 );
                std::vector<DeviceInfo> deviceInfos = pSystem->GetDevices();

                if ( deviceInfos.size() == 0 )
                {
                        throw GenICam::GenericException( "カメラが接続されていません。", __FILE__, __LINE__ );
                }

                IDevice* pDevice = pSystem->CreateDevice( deviceInfos[ 0 ] );

                AcquireImage( pDevice );


                pSystem->DestroyDevice( pDevice );
                CloseSystem( pSystem );
        }
        catch ( GenICam::GenericException& ge )
        {
                std::cout << "\nGenICam 例外がスローされました : " << ge.what() << "\n";
                return -1;
        }
        catch ( std::exception& ex )
        {
                std::cout << "\n標準例外がスローされました : " << ex.what() << "\n";
                return -1;
        }
        catch ( ... )
        {
                std::cout << "\n未定義の例外がスローされました\n";
                return -1;
        }

        std::cout << "Enterキーを押すと終了します。\n";
        std::getchar();

    return 0;
}

