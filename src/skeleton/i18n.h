//
// Created for pEMU / pfbneo Zero-Intrusion Chinese Localization
//

#ifndef PEMU_I18N_H
#define PEMU_I18N_H

#include <string>
#include <unordered_map>

namespace c2d {
    class Io;
}

namespace pemu {

class I18n {
public:
    static I18n &getInstance();

    void init(c2d::Io *io);

    // Translate UI text, returns original text if not found
    const std::string &translate(const std::string &key);

    // Auto-wrap CJK text to fit a specific display character width per line
    static std::string wrapCJK(const std::string &text, size_t maxCharsPerLine = 38);

    // Get game Chinese name by zip rom name (e.g., "kof97" -> "拳皇 97")
    std::string getGameTitle(const std::string &zipName);

    // Format dynamic ROM loading progress message
    static std::string formatLoadingMsg(const std::string &rawMsg);

    // Convenience static shortcuts
    static const std::string &tr(const std::string &key) {
        return getInstance().translate(key);
    }

    static std::string getTitle(const std::string &zipName) {
        return getInstance().getGameTitle(zipName);
    }

private:
    I18n();
    ~I18n() = default;

    void loadLanguageFile(const std::string &path, c2d::Io *io);
    void loadTitlesFile(const std::string &path, c2d::Io *io);
    void loadDefaultDictionary();

    std::unordered_map<std::string, std::string> m_translations;
    std::unordered_map<std::string, std::string> m_titles;
    bool m_initialized = false;
};

} // namespace pemu

#endif // PEMU_I18N_H
