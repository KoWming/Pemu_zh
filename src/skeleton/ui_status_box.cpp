//
// Created by cpasjuste on 08/12/18.
//

#include "pemu.h"

UiStatusBox::UiStatusBox(UiMain *m)
        : SkinnedRectangle(m, {"SKIN_CONFIG", "STATUSBOX"}) {
    main = m;
    clock = new C2DClock();

    SkinnedRectangle::setCornersRadius(8);
    SkinnedRectangle::setCornerPointCount(8);

    text = new Text("TIPS:", (int) (getSize().y * 0.65f), main->getSkin()->getFont());
    text->setOutlineThickness(1.0f);
    text->setOrigin(Origin::Left);
    text->setPosition(6 * main->getScaling().x, getSize().y / 2);
    text->setFillColor(SkinnedRectangle::getOutlineColor());
    add(text);

    tween = new TweenAlpha(0, SkinnedRectangle::getAlpha(), 1.0f);
    add(tween);

    setVisibility(Visibility::Hidden);
}

void UiStatusBox::show(const std::string &t) {
    std::string translated = I18n::tr(t);
    // 处理动态拼接文本 (例如 "FILTER_MISSING: ON" -> "隐藏缺失的游戏: 开")
    size_t colon = translated.find(':');
    if (colon != std::string::npos && translated == t) {
        std::string key = t.substr(0, colon);
        size_t firstNonSpace = t.find_first_not_of(' ', colon + 1);
        std::string val = (firstNonSpace != std::string::npos) ? t.substr(firstNonSpace) : "";
        translated = I18n::tr(key) + ": " + I18n::tr(val);
    }

    text->setString(translated);
    text->setPosition(6 * main->getScaling().x, getSize().y / 2);
    setSize(text->getLocalBounds().width + (12 * main->getScaling().x), getSize().y);

    clock->restart();
    setVisibility(Visibility::Visible, true);
}

void UiStatusBox::show(const char *fmt, ...) {
    char msg[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, 512, fmt, args);
    va_end(args);
    show(std::string(msg));
}

void UiStatusBox::hide() {
    clock->restart();
}

void UiStatusBox::onDraw(c2d::Transform &transform, bool draw) {
    if (isVisible() && clock->getElapsedTime().asSeconds() > 5) {
        setVisibility(Visibility::Hidden, true);
    }
    SkinnedRectangle::onDraw(transform, draw);
}

UiStatusBox::~UiStatusBox() {
    delete (clock);
}