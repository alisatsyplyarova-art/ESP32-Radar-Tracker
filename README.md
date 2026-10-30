# ESP32 Radar Tracker

Проект радара на **LILYGO TTGO T-Display (ESP32, ST7789 240×135)** с поддержкой одного датчика присутствия/движения **HLK-LD2401 или HLK-LD2450**.

English documentation: see [README_EN.md](docs/README_EN.md).

## Возможности

- Экран радара с режимами отображения.
- Поддержка LD2401 и LD2450 (одновременно подключается только один датчик).
- Wi-Fi точка доступа `LD-RADAR` и веб-интерфейс по адресу `http://192.168.4.1/`.
- Отображение данных датчика, график и журнал измерений с экспортом CSV.
- Настройка Wi-Fi через веб-интерфейс.

## Быстрый старт

1. Откройте папку `firmware/ESP32_Radar_Tracker` в Arduino IDE.
2. Установите пакет плат Espressif ESP32 и библиотеку **TFT_eSPI**.
3. Настройте TFT_eSPI согласно файлу `firmware/ESP32_Radar_Tracker/TFT_eSPI_Setup25_TTGO_T_Display.h`.
4. Выберите плату классического ESP32, выберите COM-порт и загрузите прошивку.
5. Подключитесь к Wi-Fi `LD-RADAR` и откройте `http://192.168.4.1/`.

> Перед подключением питания проверьте распиновку и требования конкретного датчика. Не подключайте LD2401 и LD2450 одновременно.

## Структура репозитория

```text
ESP32-Radar-Tracker/
├── firmware/
│   └── ESP32_Radar_Tracker/   # исходники Arduino и файлы конфигурации
├── docs/
│   ├── README_EN.md           # English overview and setup
│   ├── BUILD_RU.md             # сборка в Arduino IDE
│   ├── HARDWARE_RU.md          # подключение и оборудование
│   └── WEB_INTERFACE_RU.md     # веб-интерфейс
├── examples/                   # примеры/тесты (если доступны)
├── .gitignore
├── LICENSE
├── README.md
└── VERSIONS_RU.md
```

## Документация

- [Сборка в Arduino IDE](docs/BUILD_RU.md)
- [Оборудование и распиновка](docs/HARDWARE_RU.md)
- [Веб-интерфейс](docs/WEB_INTERFACE_RU.md)
- [English documentation](docs/README_EN.md)
- [История версий](VERSIONS_RU.md)

## Версия

**1.1.03** — версия, подтверждённая пользователем как работающая на устройстве. См. [каталог версий](VERSIONS_RU.md).

## Важно

В репозитории хранятся исходники и документация. Готовые ZIP-архивы и бинарные прошивки следует публиковать отдельно в разделе GitHub Releases; запись в журнале версий сама по себе не означает наличие релиза.
