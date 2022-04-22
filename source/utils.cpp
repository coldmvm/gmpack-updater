#include "utils.hpp"

#include <switch.h>

#include <filesystem>
#include <fstream>

#include "current_cfw.hpp"
#include "download.hpp"
#include "extract.hpp"
#include "fs.hpp"
#include "main_frame.hpp"
#include "progress_event.hpp"
#include "reboot_payload.h"
#include "unistd.h"

namespace i18n = brls::i18n;
using namespace i18n::literals;

namespace util {

    bool isArchive(const std::string& path)
    {
        if (std::filesystem::exists(path)) {
            std::ifstream is(path, std::ifstream::binary);
            char zip_signature[4] = {0x50, 0x4B, 0x03, 0x04};  // zip signature header PK\3\4
            char signature[4];
            is.read(signature, 4);
            if (is.good() && std::equal(std::begin(signature), std::end(signature), std::begin(zip_signature), std::end(zip_signature))) {
                return true;
            }
        }
        return false;
    }

    void downloadArchive(const std::string& url, contentType type)
    {
        long status_code;
        downloadArchive(url, type, status_code);
    }

    void downloadArchive(const std::string& url, contentType type, long& status_code)
    {
        fs::createTree(DOWNLOAD_PATH);
        switch (type) {
            case contentType::fw:
                status_code = download::downloadFile(url, FIRMWARE_FILENAME, OFF);
                break;
            case contentType::app:
                status_code = download::downloadFile(url, APP_FILENAME, OFF);
                break;
            case contentType::ams_cfw:
                status_code = download::downloadFile(url, AMS_FILENAME, OFF);
                break;
            case contentType::translations:
                status_code = download::downloadFile(url, TRANSLATIONS_ZIP_PATH, OFF);
                break;
            case contentType::modifications:
                status_code = download::downloadFile(url, MODIFICATIONS_ZIP_PATH, OFF);
                break;
            default:
                break;
        }
        ProgressEvent::instance().setStatusCode(status_code);
    }

    void showDialogBoxInfo(const std::string& text)
    {
        brls::Dialog* dialog;
        dialog = new brls::Dialog(text);
        brls::GenericEvent::Callback callback = [dialog](brls::View* view) {
            dialog->close();
        };
        dialog->addButton("menus/common/ok"_i18n, callback);
        dialog->setCancelable(true);
        dialog->open();
    }

    int showDialogBoxBlocking(const std::string& text, const std::string& opt)
    {
        int dialogResult = -1;
        int result = -1;
        brls::Dialog* dialog = new brls::Dialog(text);
        brls::GenericEvent::Callback callback = [dialog, &dialogResult](brls::View* view) {
            dialogResult = 0;
            dialog->close();
        };
        dialog->addButton(opt, callback);
        dialog->setCancelable(false);
        dialog->open();
        while (result == -1) {
            usleep(1);
            result = dialogResult;
        }
        return result;
    }

    int showDialogBoxBlocking(const std::string& text, const std::string& opt1, const std::string& opt2)
    {
        int dialogResult = -1;
        int result = -1;
        brls::Dialog* dialog = new brls::Dialog(text);
        brls::GenericEvent::Callback callback1 = [dialog, &dialogResult](brls::View* view) {
            dialogResult = 0;
            dialog->close();
        };
        brls::GenericEvent::Callback callback2 = [dialog, &dialogResult](brls::View* view) {
            dialogResult = 1;
            dialog->close();
        };
        dialog->addButton(opt1, callback1);
        dialog->addButton(opt2, callback2);
        dialog->setCancelable(false);
        dialog->open();
        while (result == -1) {
            usleep(1);
            result = dialogResult;
        }
        return result;
    }

    void crashIfNotArchive(contentType type)
    {
        std::string filename;
        switch (type) {
            case contentType::fw:
                filename = FIRMWARE_FILENAME;
                break;
            case contentType::app:
                filename = APP_FILENAME;
                break;
            case contentType::ams_cfw:
                filename = AMS_FILENAME;
                break;
            default:
                return;
        }
        if (!isArchive(filename)) {
            brls::Application::crash("menus/utils/not_an_archive"_i18n);
        }
    }

    void extractArchive(contentType type)
    {
        chdir(ROOT_PATH);
        crashIfNotArchive(type);
        switch (type) {
            case contentType::fw:
                if (std::filesystem::exists(FIRMWARE_PATH)) std::filesystem::remove_all(FIRMWARE_PATH);
                fs::createTree(FIRMWARE_PATH);
                extract::extract(FIRMWARE_FILENAME, FIRMWARE_PATH);
                break;
            case contentType::app:
                extract::extract(APP_FILENAME, CONFIG_PATH);
                fs::copyFile(ROMFS_FORWARDER, FORWARDER_PATH);
                envSetNextLoad(FORWARDER_PATH, fmt::format("\"{}\"", FORWARDER_PATH).c_str());
                romfsExit();
                brls::Application::quit();
                break;
            case contentType::ams_cfw: {
                //int freshInstall = showDialogBoxBlocking("menus/utils/fresh_install"_i18n, "menus/common/no"_i18n, "menus/common/yes"_i18n);
                //int overwriteInis = showDialogBoxBlocking("menus/utils/overwrite_inis"_i18n, "menus/common/no"_i18n, "menus/common/yes"_i18n);
                //int deleteContents = showDialogBoxBlocking("menus/utils/delete_sysmodules_flags"_i18n, "menus/common/no"_i18n, "menus/common/yes"_i18n);
                
                int freshInstall = 0;
                int overwriteInis = 1;
                int deleteContents = 1;

                usleep(800000);
                if (deleteContents == 1)
                    removeSysmodulesFlags(AMS_CONTENTS);

                extract::extract(AMS_FILENAME, ROOT_PATH, overwriteInis);
                break;
            }
            case contentType::translations:
                extract::extract(TRANSLATIONS_ZIP_PATH, AMS_CONTENTS);
                break;
            case contentType::modifications:
                extract::extract(MODIFICATIONS_ZIP_PATH, AMS_CONTENTS);
                break;

            default:
                break;
        }
        if (type == contentType::ams_cfw)
            fs::copyFiles(COPY_FILES_TXT);
    }

    std::string formatListItemTitle(const std::string& str, size_t maxScore)
    {
        size_t score = 0;
        for (size_t i = 0; i < str.length(); i++) {
            score += std::isupper(str[i]) ? 4 : 3;
            if (score > maxScore) {
                return str.substr(0, i - 1) + "\u2026";
            }
        }
        return str;
    }

    std::string formatApplicationId(u64 ApplicationId)
    {
        return fmt::format("{:016X}", ApplicationId);
    }

    void shutDown(bool reboot)
    {
        spsmInitialize();
        spsmShutdown(reboot);
    }

    void rebootToPayload(const std::string& path)
    {
        reboot_to_payload(path.c_str(), CurrentCfw::running_cfw != CFW::ams);
    }

    std::string getLatestTag(const std::string& url)
    {
        nlohmann::ordered_json tag;
        download::getRequest(url, tag, {"accept: application/vnd.github.v3+json"});
        if (tag.find("tag_name") != tag.end())
            return tag["tag_name"];
        else
            return "";
    }

    std::string downloadFileToString(const std::string& url)
    {
        std::vector<uint8_t> bytes;
        download::downloadFile(url, bytes);
        std::string str(bytes.begin(), bytes.end());
        return str;
    }

    void saveToFile(const std::string& text, const std::string& path)
    {
        std::ofstream file(path);
        file << text << std::endl;
    }

    std::string readFile(const std::string& path)
    {
        std::string text = "";
        std::ifstream file(path);
        if (file.good()) {
            file >> text;
        }
        return text;
    }

    bool isErista()
    {
        SetSysProductModel model;
        setsysGetProductModel(&model);
        return (model == SetSysProductModel_Nx || model == SetSysProductModel_Copper);
    }

    void removeSysmodulesFlags(const std::string& directory)
    {
        for (const auto& e : std::filesystem::recursive_directory_iterator(directory)) {
            if (e.path().string().find("boot2.flag") != std::string::npos) {
                std::filesystem::remove(e.path());
            }
        }
    }
    
    std::string lowerCase(const std::string& str)
    {
        std::string res = str;
        std::for_each(res.begin(), res.end(), [](char& c) {
            c = std::tolower(c);
        });
        return res;
    }

    std::string upperCase(const std::string& str)
    {
        std::string res = str;
        std::for_each(res.begin(), res.end(), [](char& c) {
            c = std::toupper(c);
        });
        return res;
    }

    std::string getErrorMessage(long status_code)
    {
        std::string res;
        switch (status_code) {
            case 500:
                res = fmt::format("{0:}: Internal Server Error", status_code);
                break;
            case 503:
                res = fmt::format("{0:}: Service Temporarily Unavailable", status_code);
                break;
            default:
                res = fmt::format("error: {0:}", status_code);
                break;
        }
        return res;
    }

    bool isApplet()
    {
        AppletType at = appletGetAppletType();
        return at != AppletType_Application && at != AppletType_SystemApplication;
    }

    std::set<std::string> getExistingCheatsTids()
    {
        std::string path = getContentsPath();
        std::set<std::string> res;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            std::string cheatsPath = entry.path().string() + "/cheats";
            if (std::filesystem::exists(cheatsPath)) {
                res.insert(util::upperCase(cheatsPath.substr(cheatsPath.length() - 7 - 16, 16)));
            }
        }
        return res;
    }

    std::string getContentsPath()
    {
        std::string path;
        switch (CurrentCfw::running_cfw) {
            case CFW::ams:
                path = std::string(AMS_PATH) + std::string(CONTENTS_PATH);
                break;
            case CFW::rnx:
                path = std::string(REINX_PATH) + std::string(CONTENTS_PATH);
                break;
            case CFW::sxos:
                path = std::string(SXOS_PATH) + std::string(TITLES_PATH);
                break;
        }
        return path;
    }

    bool getBoolValue(const nlohmann::json& jsonFile, const std::string& key)
    {
        /* try { return jsonFile.at(key); }
    catch (nlohmann::json::out_of_range& e) { return false; } */
        return (jsonFile.find(key) != jsonFile.end()) ? jsonFile.at(key).get<bool>() : false;
    }

    const nlohmann::ordered_json getValueFromKey(const nlohmann::ordered_json& jsonFile, const std::string& key)
    {
        return (jsonFile.find(key) != jsonFile.end()) ? jsonFile.at(key) : nlohmann::ordered_json::object();
    }

    void writeLog(std::string line)
    {
        std::ofstream logFile;
        logFile.open(LOG_FILE, std::ofstream::out | std::ofstream::app);
        if (logFile.is_open()) {
            logFile << line << std::endl;
        }
        logFile.close();
    }

    std::string getGMPackVersion()
    {
        std::ifstream file(PACK_FILE);
        std::string line;

        if (file.is_open())
        {
            while (std::getline(file, line)) {
                if(line.find("{GMPACK", 0) != std::string::npos)
                {
                    line = line.substr(1, line.size() - 2) + " | AMS ";
                    break;
                }
                else
                    line = "";
            }
            file.close();
        }
        return line;
    }

    void doDelete(std::vector<std::string> folders)
    {
        ProgressEvent::instance().setTotalSteps(folders.size());
        ProgressEvent::instance().setStep(0);

        std::string contentsPath = util::getContentsPath();
        for (std::string f : folders) {
            std::filesystem::remove_all(contentsPath + f);
            ProgressEvent::instance().incrementStep(1);
        }
    }

    bool isExtraPresent(const std::vector<std::string> folders)
    {
        std::string contentsPath = util::getContentsPath();
        for (const auto& folder : folders) {
            if (std::filesystem::exists(contentsPath + folder) && !std::filesystem::is_empty(contentsPath + folder)) {
                return true;
                break;
            }
        }
        return false;
    }

    bool wasMOTDAlreadyDisplayed()
    {
        std::string hiddenFileLine;

        std::string MOTDLine = getMOTD();
        if (MOTDLine != "")
        {
            //getting only the first line of the MOTD
            std::string firstLine;
            size_t pos_end;

            if ((pos_end = MOTDLine.find("\n", 0)) != std::string::npos)
                firstLine = MOTDLine.substr(0, pos_end);
            else
                firstLine = MOTDLine;

            std::ifstream MOTDFile;
            MOTDFile.open(HIDDEN_APG_FILE);
            if (MOTDFile.is_open()) {
                getline(MOTDFile, hiddenFileLine);
            }
            MOTDFile.close();
            return (hiddenFileLine == firstLine);
        }
        else
            return true;
    }

    std::string getMOTD()
    {
        std::string text = "";
        nlohmann::ordered_json nxlinks;
        download::getRequest(NXLINKS_URL, nxlinks);
        if (nxlinks.size())
            text = nxlinks.at(MOTD_KEY);

        return text;
    }
/*
Base64
*/
    static const char* B64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static const int B64index[256] =
    {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,
        0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63,
        0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
    };

    const std::string b64encode(const void* data, const size_t &len)
    {
        std::string result((len + 2) / 3 * 4, '=');
        unsigned char *p = (unsigned  char*) data;
        char *str = &result[0];
        size_t j = 0, pad = len % 3;
        const size_t last = len - pad;

        for (size_t i = 0; i < last; i += 3)
        {
            int n = int(p[i]) << 16 | int(p[i + 1]) << 8 | p[i + 2];
            str[j++] = B64chars[n >> 18];
            str[j++] = B64chars[n >> 12 & 0x3F];
            str[j++] = B64chars[n >> 6 & 0x3F];
            str[j++] = B64chars[n & 0x3F];
        }
        if (pad)  /// Set padding
        {
            int n = --pad ? int(p[last]) << 8 | p[last + 1] : p[last];
            str[j++] = B64chars[pad ? n >> 10 & 0x3F : n >> 2];
            str[j++] = B64chars[pad ? n >> 4 & 0x03F : n << 4 & 0x3F];
            str[j++] = pad ? B64chars[n << 2 & 0x3F] : '=';
        }
        return result;
    }

    const std::string b64decode(const void* data, const size_t &len)
    {
        if (len == 0) return "";

        unsigned char *p = (unsigned char*) data;
        size_t j = 0,
            pad1 = len % 4 || p[len - 1] == '=',
            pad2 = pad1 && (len % 4 > 2 || p[len - 2] != '=');
        const size_t last = (len - pad1) / 4 << 2;
        std::string result(last / 4 * 3 + pad1 + pad2, '\0');
        unsigned char *str = (unsigned char*) &result[0];

        for (size_t i = 0; i < last; i += 4)
        {
            int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
            str[j++] = n >> 16;
            str[j++] = n >> 8 & 0xFF;
            str[j++] = n & 0xFF;
        }
        if (pad1)
        {
            int n = B64index[p[last]] << 18 | B64index[p[last + 1]] << 12;
            str[j++] = n >> 16;
            if (pad2)
            {
                n |= B64index[p[last + 2]] << 6;
                str[j++] = n >> 8 & 0xFF;
            }
        }
        return result;
    }

    std::string b64encode(const std::string& str)
    {
        return b64encode(str.c_str(), str.size());
    }

    std::string b64decode(const std::string& str64)
    {
        return b64decode(str64.c_str(), str64.size());
    }
}  // namespace util