cmake_minimum_required(VERSION 3.16)
project(cereblix-qt-wallet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(unofficial-sodium CONFIG REQUIRED)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/settings/AppSettings.cpp
    src/rpc/NodeClient.cpp
    src/wallet/WalletStore.cpp
    src/wallet/TxBuilder.cpp
    src/crypto/CereblixCrypto.cpp
    src/crypto/Pbkdf2.cpp
    src/node/LocalNodeManager.cpp
    src/util/QrCodeImage.cpp
    src/util/SecureClipboard.cpp
    src/util/SecureZero.cpp
    src/ui/WalletEncrypt.cpp
    src/ui/OverviewPage.cpp
    src/ui/ReceivePage.cpp
    src/ui/SendPage.cpp
    src/ui/HistoryPage.cpp
    src/ui/AddressesPage.cpp
    src/ui/ExplorerPage.cpp
    src/ui/SettingsPage.cpp
    src/SelfTest.cpp
    third_party/qrcodegen/cpp/qrcodegen.cpp
)

add_executable(cereblix-qt-wallet ${SOURCES} resources/resources.qrc)

target_include_directories(cereblix-qt-wallet PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/qrcodegen/cpp
)

target_link_libraries(cereblix-qt-wallet PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    unofficial-sodium::sodium
)

if(WIN32)
    set_target_properties(cereblix-qt-wallet PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS cereblix-qt-wallet RUNTIME DESTINATION bin)
