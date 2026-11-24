```
Author : Fatih Nurrobi Alannshori + AI Agent
```

---
# BeaconNetra (FuMS-TX)

Firmware ESP32 untuk **Fuelman Netra Transmitter**: membaca kondisi unit (tegangan suplai, hour meter, GPS, dan data tanki dari sensor ATG via Modbus RTU), lalu mengirimkannya sebagai **BLE beacon** ke aplikasi penerima. Firmware juga menyediakan kanal **OTA**, kontrol WiFi, dan kalibrasi/trigger hour meter lewat BLE.

- Board: `esp32dev` (PlatformIO, framework Arduino)
- Versi firmware: lihat `FIRMWARE_VERSION` di `include/common.h` (`FuMS->v1.0.1-260515`)

## Fitur

| Fitur | Keterangan |
| --- | --- |
| BLE Beacon | Broadcast data unit (ID, tegangan, hour meter, GPS, level tanki) dalam manufacturer data |
| Hour Meter | Perhitungan otomatis dari analog input (di atas `thresholdHM`), dengan offset & penyimpanan ke storage |
| Analog Input | Pembacaan tegangan suplai via ADS1x15 + filter anti-glitch |
| GPS | Pembacaan koordinat & status fix via TinyGPSPlus |
| Modbus ATG | Baca/tulis register sensor tanki (level, volume, alarm) via RS485 |
| Deep Sleep | Siklus hemat daya (default 0.5 menit ON / 1.5 menit OFF) saat unit berjalan di baterai |
| OTA BLE + WiFi AP | Update firmware lewat BLE, atau nyalakan WiFi AP + HTTP server untuk upload |
| GATT WiFi Control | Nyalakan/matikan WiFi AP & HTTP server dari BLE (`WIFI_ON` / `WIFI_OFF`) |
| GATT HM Control | Start/stop pencatatan hour meter dari BLE (`HM_START` / `HM_STOP`) |
| GATT HM Update | Set nilai hour meter dari BLE dengan token (`Qims_HM_BLE:<jam>`) |
| Autentikasi BLE | Challenge-response 8 byte + shared secret sebelum perintah diterima, dengan timeout 10 detik |
| Konfigurasi Serial | Ubah setting (ID, threshold, offset) lewat perintah serial |

## Struktur Proyek

```
src/main.cpp            Setup, task FreeRTOS, deep sleep, OTA loop
include/common.h        Struct data, daftar ID unit, versi firmware
lib/ble/                BLE beacon, GATT server, OTA, HTTP server, auth
lib/analog_input/       Pembacaan & filter tegangan suplai
lib/hour_meter_manager/ Perhitungan & penyimpanan hour meter
lib/rtc/                RTC (waktu untuk hour meter & timestamp)
lib/gps/                Pembacaan GPS
lib/modbus_atg/         Modbus RTU ke sensor ATG
lib/id_management/      Parsing ID unit dari nama device
lib/check_sleep/        Logika deep sleep
data/settings.json      Konfigurasi yang disimpan di LittleFS
```

Task utama (FreeRTOS): `Send BLE Data`, `Updating Hour Meter`, `get GPS Data`, `Data Acquisition`, `Updating setting`, `Check Deep Sleep`, `OTA BLE Update`, `Request ATG Info`.

## BLE GATT

WiFi AP: SSID `WIFI_<ID>`, password `netra_ota360`.

| Service | UUID | Characteristic | Perintah |
| --- | --- | --- | --- |
| OTA | `6f7e9280-e3d8-47b7-9733-ef0ffffc65fd` | `2233b614-a358-4551-8ad1-b113764c68f3` | Auth (`0x19` request, `0x18` response), OTA |
| Device Info | `180A` | `2A26` | Baca versi firmware |
| HM Update | `f0a1b2c3-d4e5-4f6a-8b7c-9d0e1f2a3b4c` | `a1b2c3d4-e5f6-4a7b-8c9d-0e1f2a3b4c5d` | `Qims_HM_BLE:<jam>` |
| WiFi Control | `e0b1c2d3-e4f5-4a6b-8c7d-9e0f1a2b3c4d` | `d1c2b3a4-e5f6-4a7b-8c9d-0e1f2a3b4c5e` | `WIFI_ON`, `WIFI_OFF` |
| HM Control | `b0c1d2e3-f4a5-4b6c-8d7e-9f0a1b2c3d4e` | `c1d2e3f4-a5b6-4c7d-8e9f-0a1b2c3d4e5f` | `HM_START`, `HM_STOP` |

Semua characteristic di atas hanya menerima perintah setelah autentikasi berhasil.

## HTTP Server (saat WiFi AP aktif)

| Endpoint | Method | Fungsi |
| --- | --- | --- |
| `/` | GET | Cek koneksi (`Hello from ESP32!`) |
| `/data` | GET | Data monitoring dalam JSON |
| `/setting` | POST (JSON) | Baca/ubah setting unit |

## Build & Flash

```bash
pio run                 # build
pio run -t upload       # flash
pio device monitor      # serial monitor (9600 baud)
pio run -t uploadfs     # upload isi folder data/ ke LittleFS
```

## Changelog

| VERSION | DESCRIPTION | ISSUE RESOLVED |
| ------- | ----------- | -------------- |
| v1.0.1  | - Menambahkan GATT service WiFi control (`WIFI_ON`/`WIFI_OFF`). | WiFi AP & HTTP server bisa dinyalakan dari BLE tanpa OTA. |
|         | - Menambahkan GATT service HM control (`HM_START`/`HM_STOP`) dan HM update via BLE. | Kalibrasi & trigger hour meter tanpa kabel. |
|         | - Autentikasi BLE challenge-response + timeout, ganti password WiFi AP menjadi `netra_ota360`. | Keamanan akses perintah BLE & WiFi. |
| v1.7.0  | - Penambahan fitur Deep Sleep Cycle dengan konfigurasi 30 detik ON - 90 detik OFF. | Meningkatkan daya tahan baterai menjadi 2-3 hari. |
|         | - Penambahan fitur update firmware melalui OTA (over-the-air) | Mitigasi apabila diperlukan pembaruan firmware di lapangan. |
| v1.5.2  | - Mengefisiensikan respons perhitungan Hour Meter terhadap Analog Input | Peningkatan akurasi Hour Meter. |
| v1.2.2  | - Menambahkan filter pencuplikan Hour Meter untuk mengatasi anomali data yang nilainya menlonjak menjadi jutaan. | Mengatasi anomali data Hour Meter yang nilainya melonjak menjadi angka yang sangat besar. |
| v1.2.1  | - Rilis Awal | |
