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
        {"SCALING_MODE", "画面比例模式"},
        {"FILTER", "纹理过滤算法"},
        {"EFFECT", "画面着色特效"},
        {"WAIT_RENDERING", "垂直同步(等待渲染)"},
        {"SHOW_FPS", "显示运行帧率(FPS)"},
        {"FORCE_60HZ", "强制 60Hz 刷新率"},
        {"AUDIO_FREQUENCY", "音频采样率"},
        {"AUDIO_INTERPOLATION", "音频采样插值"},
        {"AUDIO_FM_INTERPOLATION", "FM 音频插值"},
        {"ROTATION", "屏幕画面旋转"},
        {"NEOBIOS", "NEOGEO BIOS 区域"},
        {"FRAMESKIP", "跳帧设置"},

        // Gamepad Options & Keys
        {"GAMEPAD", "手柄按键映射"},
        {"JOY_UP", "方向键 上"},
        {"JOY_DOWN", "方向键 下"},
        {"JOY_LEFT", "方向键 左"},
        {"JOY_RIGHT", "方向键 右"},
        {"JOY_A", "按键 A"},
        {"JOY_B", "按键 B"},
        {"JOY_X", "按键 X"},
        {"JOY_Y", "按键 Y"},
        {"JOY_LT", "按键 L (LT)"},
        {"JOY_RT", "按键 R (RT)"},
        {"JOY_LB", "按键 L1 (LB)"},
        {"JOY_RB", "按键 R1 (RB)"},
        {"JOY_SELECT", "选择键 (Select)"},
        {"JOY_START", "开始键 (Start)"},
        {"JOY_MENU1", "快捷菜单键 1"},
        {"JOY_MENU2", "快捷菜单键 2"},
        {"JOY_DEADZONE", "摇杆死区阈值"},

        // Keyboard Options & Keys
        {"KEYBOARD", "键盘按键映射"},
        {"KEY_UP", "键盘 方向上"},
        {"KEY_DOWN", "键盘 方向下"},
        {"KEY_LEFT", "键盘 方向左"},
        {"KEY_RIGHT", "键盘 方向右"},
        {"KEY_A", "键盘 A"},
        {"KEY_B", "键盘 B"},
        {"KEY_X", "键盘 X"},
        {"KEY_Y", "键盘 Y"},
        {"KEY_LT", "键盘 LT"},
        {"KEY_RT", "键盘 RT"},
        {"KEY_LB", "键盘 LB"},
        {"KEY_RB", "键盘 RB"},
        {"KEY_SELECT", "键盘 Select"},
        {"KEY_START", "键盘 Start"},
        {"KEY_MENU1", "键盘 菜单1"},
        {"KEY_MENU2", "键盘 菜单2"},

        // Common Values
        {"OFF", "关"},
        {"ON", "开"},
        {"ALL", "全部"},
        {"FIT", "等比适应"},
        {"FULL", "全屏拉伸"},
        {"NONE", "无"},
        {"FLIP", "翻转"},
        {"CAB MODE", "街机台模式"},
        {"AUTO", "自动"},
        {"ASPECT", "保持比例"},
        {"INTEGER", "整数倍缩放"},
        {"POINT", "点采样(锐利像素)"},
        {"LINEAR", "线性过滤(平滑)"},
        {"c2d-texture", "默认材质(无特效)"},

        // NeoGeo BIOS 区域名称
        {"UNIBIOS_4_0", "通用 BIOS 4.0"},
        {"UNIBIOS_3_3", "通用 BIOS 3.3"},
        {"UNIBIOS_3_2", "通用 BIOS 3.2"},
        {"UNIBIOS_3_1", "通用 BIOS 3.1"},
        {"MVS_ASIA_EUR_V6S1", "亚洲/欧洲 MVS (v6s1)"},
        {"MVS_ASIA_EUR_V5S1", "亚洲/欧洲 MVS (v5s1)"},
        {"MVS_ASIA_EUR_V3S4", "亚洲/欧洲 MVS (v3s4)"},
        {"MVS_USA_V5S2", "美国版 MVS (v5s2)"},
        {"MVS_USA_V5S4", "美国版 MVS (v5s4)"},
        {"MVS_USA_V5S6", "美国版 MVS (v5s6)"},
        {"MVS_JPN_V6", "日本版 MVS (v6)"},
        {"MVS_JPN_V5", "日本版 MVS (v5)"},
        {"MVS_JPN_V3S4", "日本版 MVS (v3s4)"},
        {"MVS_JPN_J3", "日本版 MVS (J3)"},
        {"AES_ASIA", "亚洲家用机 AES"},
        {"AES_JAPAN", "日本家用机 AES"},
        {"NEO_MVH_MV1CA", "街机基板 MV1CA"},
        {"NEO_MVH_MV1CJ", "街机基板 MV1CJ"},
        {"DECK_V6", "机台版 V6"},
        {"DEVKIT", "开发者调试套件"},

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
