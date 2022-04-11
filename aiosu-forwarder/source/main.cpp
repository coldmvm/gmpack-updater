#include <filesystem>
#include <string>

#include <switch.h>

#define PATH        "/switch/gmpack-updater/"
#define FULL_PATH   "/switch/gmpack-updater/gmpack-updater.nro"
#define CONFIG_PATH "/config/gmpack-updater/switch/gmpack-updater/gmpack-updater.nro"
#define PREFIX      "/switch/gmpack-updater/gmpack-updater-v"
#define FORWARDER_PATH      "/config/gmpack-updater/aiosu-forwarder.nro"
#define CONFIG_SWITCH       "/config/gmpack-updater/switch/"
#define HIDDEN_FILE "/config/gmpack-updater/.gmpack-updater"

int removeDir(const char* path)
{
    Result ret = 0;
    FsFileSystem *fs = fsdevGetDeviceFileSystem("sdmc");
    if (R_FAILED(ret = fsFsDeleteDirectoryRecursively(fs, path))) {
        return ret;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    std::filesystem::create_directory(PATH);
    for (const auto & entry : std::filesystem::directory_iterator(PATH)){
        if(entry.path().string().find(PREFIX) != std::string::npos) {
            std::filesystem::remove(entry.path().string());
            std::filesystem::remove(entry.path().string() + ".star");
        }
    }
    std::filesystem::remove(HIDDEN_FILE);

    if(std::filesystem::exists(CONFIG_PATH)){
        std::filesystem::create_directory(PATH);
        std::filesystem::remove(FULL_PATH);
        std::filesystem::rename(CONFIG_PATH, FULL_PATH);
        removeDir(CONFIG_SWITCH);
    }

    std::filesystem::remove(FORWARDER_PATH);

    envSetNextLoad(FULL_PATH, ("\"" + std::string(FULL_PATH) + "\"").c_str());
    return 0;
}
