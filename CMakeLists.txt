cmake_minimum_required(VERSION 3.15)
project(TelegramTTSBot)

set(CMAKE_CXX_STANDARD 17)

# libcurl topish
find_package(CURL REQUIRED)

# nlohmann/json (header-only)
# sudo apt install nlohmann-json3-dev
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(tts_bot telegram_tts_bot.cpp)

target_link_libraries(tts_bot
    CURL::libcurl
    nlohmann_json::nlohmann_json
)
