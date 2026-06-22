#pragma once

class QApplication;

namespace Cereblix {

enum class ThemeMode {
    Light,
    Dark,
    System,
};

class AppTheme {
public:
    static ThemeMode mode();
    static void setMode(ThemeMode mode);
    static bool isDark();
    static void apply(QApplication *app);
};

} // namespace Cereblix
