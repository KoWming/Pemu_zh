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

static std::string toLowerStr(const std::string &str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
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
                m_titles[toLowerStr(zip)] = name;
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

std::string I18n::wrapCJK(const std::string &text, size_t maxCharsPerLine) {
    if (text.empty()) return "";

    std::string result;
    size_t lineVisualWidth = 0;
    size_t i = 0;

    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\n') {
            result += '\n';
            lineVisualWidth = 0;
            i++;
            continue;
        }

        size_t charBytes = 1;
        size_t charVisualWidth = 1;

        if ((c & 0x80) == 0) {
            charBytes = 1;
            charVisualWidth = 1;
        } else if ((c & 0xE0) == 0xC0) {
            charBytes = 2;
            charVisualWidth = 1;
        } else if ((c & 0xF0) == 0xE0) {
            charBytes = 3;
            charVisualWidth = 2; // CJK 汉字通常为3字节UTF-8，占据双倍字符宽度
        } else if ((c & 0xF8) == 0xF0) {
            charBytes = 4;
            charVisualWidth = 2;
        }

        if (lineVisualWidth + charVisualWidth > maxCharsPerLine && lineVisualWidth > 0) {
            result += '\n';
            lineVisualWidth = 0;
        }

        for (size_t b = 0; b < charBytes && (i + b) < text.size(); b++) {
            result += text[i + b];
        }

        lineVisualWidth += charVisualWidth;
        i += charBytes;
    }

    return result;
}

std::string I18n::formatLoadingMsg(const std::string &rawMsg) {
    if (rawMsg.empty()) return "正在加载中...";

    // 解析 "Loading graphics (file.bin)...", "Loading program (xxx)..."
    std::string result = rawMsg;
    if (result.rfind("Loading", 0) == 0) {
        std::string rest = trim(result.substr(7)); // 去除 "Loading"
        std::string type;
        std::string filePart;

        if (rest.rfind("graphics", 0) == 0) {
            type = "图像数据";
            filePart = trim(rest.substr(8));
        } else if (rest.rfind("program", 0) == 0) {
            type = "程序代码";
            filePart = trim(rest.substr(7));
        } else if (rest.rfind("sound", 0) == 0) {
            type = "声音数据";
            filePart = trim(rest.substr(5));
        } else if (rest.rfind("BIOS", 0) == 0) {
            type = "BIOS固件";
            filePart = trim(rest.substr(4));
        } else {
            type = "资源";
            filePart = rest;
        }

        return "正在加载 " + type + " " + filePart;
    }

    return tr(rawMsg);
}

std::string I18n::getGameTitle(const std::string &name) {
    if (name.empty()) return "";

    // 1. 精确匹配
    auto it = m_titles.find(name);
    if (it != m_titles.end()) return it->second;

    // 2. 小写精确匹配
    std::string lower = toLowerStr(name);
    it = m_titles.find(lower);
    if (it != m_titles.end()) return it->second;

    // 3. 去掉版本括号匹配 (例如 "Air Buster: Trouble Specialty Raid Unit (World)" -> "Air Buster")
    size_t paren = name.find('(');
    if (paren != std::string::npos && paren > 0) {
        std::string clean = trim(name.substr(0, paren));
        it = m_titles.find(clean);
        if (it != m_titles.end()) return it->second;
        it = m_titles.find(toLowerStr(clean));
        if (it != m_titles.end()) return it->second;

        // 去掉冒号前缀 (例如 "Air Buster: xxx" -> "Air Buster")
        size_t colon = clean.find(':');
        if (colon != std::string::npos && colon > 0) {
            std::string sub = trim(clean.substr(0, colon));
            it = m_titles.find(sub);
            if (it != m_titles.end()) return it->second;
            it = m_titles.find(toLowerStr(sub));
            if (it != m_titles.end()) return it->second;
        }
    }

    // 4. 去掉斜杠别名匹配 (例如 "Aero Fighters 2 / Sonic Wings 2" -> 查 "Aero Fighters 2" 或 "Sonic Wings 2")
    size_t slash = name.find('/');
    if (slash != std::string::npos && slash > 0) {
        std::string left = trim(name.substr(0, slash));
        std::string right = trim(name.substr(slash + 1));
        // 去除右侧可能存在的括号
        size_t rParen = right.find('(');
        if (rParen != std::string::npos) right = trim(right.substr(0, rParen));

        it = m_titles.find(left);
        if (it != m_titles.end()) return it->second;
        it = m_titles.find(toLowerStr(left));
        if (it != m_titles.end()) return it->second;

        it = m_titles.find(right);
        if (it != m_titles.end()) return it->second;
        it = m_titles.find(toLowerStr(right));
        if (it != m_titles.end()) return it->second;
    }

    // 5. 去掉冒号副标题匹配 (例如 "Baryon - Future Assault" -> "Baryon")
    size_t dash = name.find('-');
    if (dash != std::string::npos && dash > 0) {
        std::string mainTitle = trim(name.substr(0, dash));
        it = m_titles.find(mainTitle);
        if (it != m_titles.end()) return it->second;
        it = m_titles.find(toLowerStr(mainTitle));
        if (it != m_titles.end()) return it->second;
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
        {"File: ", "文件路径: "},

        // Popup Dialogs & System Notices
        {"ERROR", "错误提示"},
        {"OK", "确定"},
        {"CANCEL", "取消"},
        {"WARNING", "警告"},
        {"THIS GAME IS NOT SUPPORTED BY FBNEO...", "FBNEO 不支持此游戏或驱动缺失..."},
        {"DRIVER INIT FAILED", "驱动核心初始化失败"},
        {"INVALID ROM FILE", "无效的 ROM 游戏文件"},
        {"TIPS: PRESS MENU1 + MENU2 BUTTONS FOR IN GAME MENU...", "提示: 同时按下 菜单1 + 菜单2 按键可呼出游戏内菜单..."},
        {"TIPS: PRESS START BUTTON 2 SECONDS FOR DIAG MENU...", "提示: 按住 START 键 2 秒进入机台诊断菜单(基板设置)..."},
        {"TIPS: PRESS COIN BUTTON 2 SECONDS TO RESET CURRENT GAME...", "提示: 按住 SELECT(投币) 键 2 秒可强制复位当前游戏..."},
        {"THIS GAME DOES NOT SUPPORT THE M68K ASM CORE\nCYCLONE ASM CORE DISABLED", "此游戏不支持 M68K 汇编核心，已自动禁用 Cyclone 汇编核心"},
        {"YOU NEED TO RESTART EMULATION AFTER CHANGING THIS OPTION", "更改此设置项后，需要重新启动游戏模拟才能生效"},
        {"YOU NEED TO RESTART THE APPLICATION AFTER CHANGING THIS OPTION", "更改此设置项后，需要重新启动模拟器才能生效"},
        {"TRY TO KEEP INTEGER SCALING IF ASPECT RATIO IS NOT TOO DIVERGENT", "如果画面比例差异不大，建议保持整数倍缩放以获得最佳画质"},
        {"KEEP GAME ASPECT RATIO - SOME SHADERS MAY NOT RENDER CORRECTLY", "建议保持原机画面比例 - 某些着色滤镜可能无法正确拉伸"},
        {"Please wait...", "请稍候..."}
    };
}

} // namespace pemu
