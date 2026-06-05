# 🤖 Telegram TTS Bot (C++)

Foydalanuvchi yuborgan **matnni** avtomatik ravishda **audio xabarga** aylantirib qaytaruvchi Telegram bot.

---

## 📦 Kerakli paketlar

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y \
    build-essential cmake \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    espeak \
    ffmpeg
```

---

## ⚙️ Sozlash

`telegram_tts_bot.cpp` faylini oching va **2-qatordagi** tokenni o'zgartiring:

```cpp
const std::string BOT_TOKEN = "YOUR_BOT_TOKEN_HERE";
//                             ↑ BotFather dan olingan token
```

> **Token olish:** Telegramda [@BotFather](https://t.me/BotFather) ga yozing → `/newbot` → token nusxalab oling.

---

## 🔨 Build qilish

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Yoki to'g'ridan-to'g'ri `g++` bilan:

```bash
g++ -std=c++17 -O2 \
    telegram_tts_bot.cpp \
    -lcurl \
    -o tts_bot
```

---

## ▶️ Ishga tushirish

```bash
./tts_bot
```

Bot ishlayotganini ko'rasiz:
```
🤖 TTS Bot ishga tushdi...
[LOG] Chat 123456789 → Salom dunyo
```

---

## 💬 Bot buyruqlari

| Buyruq | Tavsif |
|--------|--------|
| `/start` | Salomlashuv va qo'llanma |
| `/help`  | Yordam xabari |
| *(har qanday matn)* | Ovozga aylantirib qaytaradi |

---

## 🔄 Ishlash tartibi

```
Foydalanuvchi (matn)
        ↓
    Bot qabul qiladi
        ↓
  espeak → WAV fayl
        ↓
  ffmpeg → MP3 fayl
        ↓
Foydalanuvchi (audio 🔊)
```

---

## 🌐 Til sozlamasi

Bot avval **o'zbek tili** (`-v uz`) bilan urinadi. Agar espeak'da o'zbek tili o'rnatilmagan bo'lsa, **ingliz tili** (`-v en`) ishlatiladi.

O'zbek tilini o'rnatish:
```bash
sudo apt install espeak-data
# yoki
espeak --voices | grep uz
```

---

## 🖥 Fon rejimida ishlatish

```bash
# nohup bilan
nohup ./tts_bot > bot.log 2>&1 &

# yoki systemd service sifatida
# /etc/systemd/system/tts_bot.service fayliga yozing
```

---

## ⚠️ Cheklovlar

- Matn uzunligi: **500 belgi** gacha
- Bir vaqtda ko'p foydalanuvchi: thread-safe emas (oddiy foydalanish uchun yetarli)
- Katta yuklamalar uchun thread pool qo'shish tavsiya etiladi
