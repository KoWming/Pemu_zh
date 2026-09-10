//
// Created for pEMU / pfbneo Zero-Intrusion Chinese Localization
//

#include <sstream>
#include <vector>
#include <algorithm>
#include "cross2d/c2d.h"
#include "i18n.h"

namespace pemu {

static std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static std::string readFileContent(const std::string &path) {
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) return "";

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0) {
        fclose(fp);
        return "";
    }

    std::string content(static_cast<size_t>(size), '\0');
    size_t bytesRead = fread(&content[0], 1, static_cast<size_t>(size), fp);
    fclose(fp);

    if (bytesRead < static_cast<size_t>(size)) {
        content.resize(bytesRead);
    }
    return content;
}

I18n &I18n::getInstance() {
    static I18n instance;
    return instance;
}

I18n::I18n() {
    loadDefaultDictionary();
}

void I18n::init(c2d::Io *io) {
    if (!io || m_initialized) {
        return;
    }

    std::string dataPath = io->getDataPath();
    std::string romfsPath = io->getRomFsPath();

    // 1. Load language file: check user data path first, then fallback to romfs
    std::string userLang = dataPath + "lang/zh_CN.lang";
    std::string romfsLang = romfsPath + "lang/zh_CN.lang";

    if (io->exist(userLang)) {
        printf("I18n: loading language from user data: %s\n", userLang.c_str());
        loadLanguageFile(userLang, io);
    } else if (io->exist(romfsLang)) {
        printf("I18n: loading language from romfs: %s\n", romfsLang.c_str());
        loadLanguageFile(romfsLang, io);
    } else {
        printf("I18n: no external language file found, using built-in dictionary\n");
    }

    // 2. Load game titles: check user data path first, then fallback to romfs
    std::string userTitles = dataPath + "titles.csv";
    std::string romfsTitles = romfsPath + "titles.csv";

    if (io->exist(userTitles)) {
        printf("I18n: loading game titles from user data: %s\n", userTitles.c_str());
        loadTitlesFile(userTitles, io);
    } else if (io->exist(romfsTitles)) {
        printf("I18n: loading game titles from romfs: %s\n", romfsTitles.c_str());
        loadTitlesFile(romfsTitles, io);
    }

    m_initialized = true;
}

void I18n::loadLanguageFile(const std::string &path, c2d::Io *io) {
    std::string content = readFileContent(path);
    if (content.empty()) return;

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        std::string s = trim(line);
        if (s.empty() || s[0] == '#' || s[0] == ';' || s[0] == '[') {
            continue;
        }

        size_t eqPos = s.find('=');
        if (eqPos != std::string::npos) {
            std::string key = trim(s.substr(0, eqPos));
            std::string val = trim(s.substr(eqPos + 1));
            if (!key.empty() && !val.empty()) {
                m_translations[key] = val;
            }
        }
    }
}

void I18n::loadTitlesFile(const std::string &path, c2d::Io *io) {
    std::string content = readFileContent(path);
    if (content.empty()) return;

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        std::string s = trim(line);
        if (s.empty() || s[0] == '#') continue;

        size_t commaPos = s.find(',');
        if (commaPos != std::string::npos) {
            std::string zip = trim(s.substr(0, commaPos));
            std::string name = trim(s.substr(commaPos + 1));
            if (!zip.empty() && !name.empty()) {
                m_titles[zip] = name;
            }
        }
    }
}

const std::string &I18n::translate(const std::string &key) {
    auto it = m_translations.find(key);
    if (it != m_translations.end()) {
        return it->second;
    }
    return key;
}

std::string I18n::getGameTitle(const std::string &zipName) {
    auto it = m_titles.find(zipName);
    if (it != m_titles.end()) {
        return it->second;
    }
    return "";
}

void I18n::loadDefaultDictionary() {
    // Built-in core translations fallback
    m_translations = {
        // UI Navigation & Help
        {"NAVIGATION", "导航"},
        {"RUN", "启动游戏"},
        {"FAVORITE", "收藏"},
        {"ADD / REMOVE FAVORITE", "添加/移除收藏"},
        {"SWITCH SYSTEM", "切换系统"},
        {"MAIN MENU", "主菜单"},
        {"ROM MENU", "游戏菜单"},

        // Main Menu & States
        {"MAIN OPTIONS", "全局设置"},
        {"STATES", "即时存档"},
        {"QUIT", "退出程序"},
        {"OTHER", "其他选项"},
        {"GO", "进入"},
        {"NO SAVE", "无存档"},
        {"NO PIC", "无预览图"},
        {"LOAD", "读取"},
        {"SAVE", "保存"},
        {"PRESS FIRE2 TO CANCEL", "按取消键返回"},
        {"SAVE STATES", "即时存档管理"},
        {"PRESS", "操作提示"},
        {"UP/DOWN : CHANGE SLOT", "上/下 : 切换槽位"},
        {"A : LOAD", "A : 读取存档"},
        {"Y : SAVE", "Y : 保存存档"},

        // Config Groups
        {"UI_FILTERING", "游戏过滤与筛选"},
        {"UI_OPTIONS", "界面外观设置"},
        {"EMULATION", "模拟器核心设置"},
        {"ROMS", "ROM 目录设置"},

        // Filter Options
        {"FILTER_FAVORITES", "仅显示收藏"},
        {"FILTER_MISSING", "隐藏缺失的游戏"},
        {"FILTER_CLONES", "隐藏克隆版"},
        {"FILTER_SYSTEM", "按系统筛选"},
        {"FILTER_GENRE", "按类型筛选"},
        {"FILTER_DATE", "按年份筛选"},
        {"FILTER_EDITOR", "按发行商筛选"},
        {"FILTER_DEVELOPER", "按开发商筛选"},
        {"FILTER_PLAYERS", "按人数筛选"},
        {"FILTER_RATING", "按评分筛选"},

        // Global UI Options
        {"SHOW_ZIP_NAMES", "显示 ZIP 文件名"},
        {"FULLSCREEN", "全屏显示"},
        {"SKIN_ASPECT", "皮肤画面比例"},
        {"FONT_SCALING", "字体缩放比例"},
        {"VIDEO_SNAP_DELAY", "视频预览延迟(秒)"},
        {"SKIN", "主题皮肤"},

        // Emulation Options
        {"SCALING", "画面缩放模式"},
        {"FORCE_60HZ", "强制 60Hz 刷新率"},
        {"AUDIO_FREQUENCY", "音频采样率"},
        {"AUDIO_INTERPOLATION", "音频采样插值"},
        {"AUDIO_FM_INTERPOLATION", "FM 音频插值"},
        {"ROTATION", "屏幕画面旋转"},
        {"NEOBIOS", "NEOGEO BIOS 区域"},
        {"FRAMESKIP", "跳帧设置"},

        // Common Values
        {"OFF", "关"},
        {"ON", "开"},
        {"ALL", "全部"},
        {"FIT", "等比适应"},
        {"FULL", "全屏拉伸"},
        {"NONE", "无"},
        {"FLIP", "翻转"},
        {"CAB MODE", "街机台模式"},

        // ROM Info Box labels
        {"System: ", "系统: "},
        {"Developer: ", "开发商: "},
        {"Editor: ", "发行商: "},
        {"Date: ", "发行日期: "},
        {"Genre: ", "类型: "},
        {"Players: ", "玩家人数: "},
        {"Rating: ", "评分: "},
        {"Clone Of: ", "克隆自: "},
        {"File: ", "文件路径: "}
    };
}

} // namespace pemu
