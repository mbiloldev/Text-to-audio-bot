#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ===== SOZLAMALAR =====
const std::string BOT_TOKEN = "YOUR_BOT_TOKEN_HERE";  // BotFather dan olingan token
const std::string API_URL   = "https://api.telegram.org/bot" + BOT_TOKEN;

// ===== CURL callback =====
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ===== CURL yordamchi funksiya =====
std::string httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

// ===== Fayl yuklash uchun multipart POST =====
bool sendAudioFile(long long chatId, const std::string& filePath, const std::string& caption) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = API_URL + "/sendAudio";
    std::string response;

    struct curl_httppost* formpost = nullptr;
    struct curl_httppost* lastptr  = nullptr;

    // chat_id
    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "chat_id",
        CURLFORM_COPYCONTENTS, std::to_string(chatId).c_str(),
        CURLFORM_END);

    // caption
    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "caption",
        CURLFORM_COPYCONTENTS, caption.c_str(),
        CURLFORM_END);

    // audio fayl
    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "audio",
        CURLFORM_FILE, filePath.c_str(),
        CURLFORM_CONTENTTYPE, "audio/mpeg",
        CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_formfree(formpost);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[XATO] Audio yuborishda muammo: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    json resp = json::parse(response, nullptr, false);
    return resp.contains("ok") && resp["ok"].get<bool>();
}

// ===== Oddiy text xabar yuborish =====
bool sendMessage(long long chatId, const std::string& text) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    char* encoded = curl_easy_escape(curl, text.c_str(), (int)text.size());
    std::string url = API_URL + "/sendMessage?chat_id=" +
                      std::to_string(chatId) + "&text=" + encoded +
                      "&parse_mode=HTML";
    curl_free(encoded);

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    json resp = json::parse(response, nullptr, false);
    return resp.contains("ok") && resp["ok"].get<bool>();
}

// ===== Text → MP3 (espeak + ffmpeg) =====
// espeak matnni wav ga, ffmpeg wav ni mp3 ga aylantiradi
bool textToSpeech(const std::string& text, const std::string& outputPath) {
    std::string wavPath = outputPath + ".wav";

    // Matnni vaqtinchalik faylga yozamiz (maxsus belgilardan himoya)
    std::string tmpText = "/tmp/tts_input.txt";
    std::ofstream ofs(tmpText);
    ofs << text;
    ofs.close();

    // espeak yordamida WAV yaratish
    // -v uz  → o'zbek tili (mavjud bo'lmasa en ishlatiladi)
    // -f     → fayldan o'qish
    // -w     → wav fayl chiqarish
    std::string cmd1 = "espeak -v uz -f " + tmpText + " -w " + wavPath + " 2>/dev/null";
    if (system(cmd1.c_str()) != 0) {
        // o'zbek tili yo'q bo'lsa ingliz tili bilan qayta urinib ko'ramiz
        cmd1 = "espeak -v en -f " + tmpText + " -w " + wavPath + " 2>/dev/null";
        if (system(cmd1.c_str()) != 0) {
            std::cerr << "[XATO] espeak muvaffaqiyatsiz" << std::endl;
            return false;
        }
    }

    // WAV → MP3
    std::string cmd2 = "ffmpeg -y -i " + wavPath + " -codec:a libmp3lame -qscale:a 2 "
                       + outputPath + " 2>/dev/null";
    if (system(cmd2.c_str()) != 0) {
        std::cerr << "[XATO] ffmpeg muvaffaqiyatsiz" << std::endl;
        return false;
    }

    // Vaqtinchalik fayllarni o'chirish
    std::remove(wavPath.c_str());
    std::remove(tmpText.c_str());

    return true;
}

// ===== Xabarni qayta ishlash =====
void processMessage(const json& message) {
    if (!message.contains("text")) return;   // faqat text xabarlar

    long long chatId  = message["chat"]["id"].get<long long>();
    std::string text  = message["text"].get<std::string>();
    std::string name  = message["from"].contains("first_name")
                        ? message["from"]["first_name"].get<std::string>()
                        : "Foydalanuvchi";

    std::cout << "[LOG] Chat " << chatId << " → " << text << std::endl;

    // /start komandasi
    if (text == "/start") {
        std::string welcome =
            "👋 Salom, <b>" + name + "</b>!\n\n"
            "🎙 Men <b>Text-to-Audio</b> botman.\n"
            "Menga istalgan matn yuboring — uni <b>audio xabar</b> sifatida qaytaraman!\n\n"
            "📌 Yordam uchun: /help";
        sendMessage(chatId, welcome);
        return;
    }

    // /help komandasi
    if (text == "/help") {
        std::string help =
            "ℹ️ <b>Yordam</b>\n\n"
            "1️⃣ Istalgan matn yuboring\n"
            "2️⃣ Bot uni ovozga aylantiradi\n"
            "3️⃣ Audio fayl sifatida qaytaradi\n\n"
            "<b>Buyruqlar:</b>\n"
            "/start — Botni qayta ishga tushirish\n"
            "/help  — Ushbu yordam xabari\n\n"
            "⚠️ Juda uzun matnlar qayta ishlanmasligi mumkin.";
        sendMessage(chatId, help);
        return;
    }

    // Matn juda uzunmi?
    if (text.size() > 500) {
        sendMessage(chatId, "⚠️ Matn 500 belgidan oshmasin. Iltimos qisqartiring.");
        return;
    }

    // "Ishlanmoqda..." xabari
    sendMessage(chatId, "⏳ Matn ovozga aylantirilmoqda...");

    // TTS
    std::string audioPath = "/tmp/tts_" + std::to_string(chatId) + ".mp3";
    if (!textToSpeech(text, audioPath)) {
        sendMessage(chatId, "❌ Ovoz yaratishda xatolik yuz berdi. Keyinroq urinib ko'ring.");
        return;
    }

    // Audio yuborish
    std::string caption = "🔊 \"" + text.substr(0, 50) + (text.size() > 50 ? "..." : "") + "\"";
    if (!sendAudioFile(chatId, audioPath, caption)) {
        sendMessage(chatId, "❌ Audio yuborishda xatolik yuz berdi.");
    }

    // Vaqtinchalik faylni o'chirish
    std::remove(audioPath.c_str());
}

// ===== Asosiy tsikl (long polling) =====
int main() {
    std::cout << "🤖 TTS Bot ishga tushdi..." << std::endl;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    long long offset = 0;

    while (true) {
        std::string url = API_URL + "/getUpdates?timeout=30&offset=" + std::to_string(offset);
        std::string resp = httpGet(url);

        json data = json::parse(resp, nullptr, false);
        if (data.is_discarded() || !data.contains("ok") || !data["ok"].get<bool>()) {
            std::cerr << "[XATO] getUpdates muvaffaqiyatsiz, 5s kutilmoqda..." << std::endl;
            sleep(5);
            continue;
        }

        for (const auto& update : data["result"]) {
            offset = update["update_id"].get<long long>() + 1;
            if (update.contains("message")) {
                processMessage(update["message"]);
            }
        }
    }

    curl_global_cleanup();
    return 0;
}
