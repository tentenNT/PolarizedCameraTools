#include "main.hpp"

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


