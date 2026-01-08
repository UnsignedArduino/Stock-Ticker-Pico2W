#include "config.h"
#include "pins.h"
#include <Arduino.h>
#include <Button.h>
#include <MD_MAX72xx.h>
#include <MD_MAX72xx_Text.h>
#include <SPI.h>
#include <StockTicker.h>
#include <TickerSettings.h>
#include <WiFi.h>
#include <WiFiSettings.h>

Button configBtn(CONFIG_BTN_PIN);

Settings::WiFiSettings wifiSettings;
Settings::TickerSettings tickerSettings;
StockTicker::StockTicker stockTicker;

#ifdef USE_HARDWARE_SPI
// Not using Parola for manual control
MD_MAX72XX display =
  MD_MAX72XX(HARDWARE_TYPE, SPI, CS_PIN, matrixModulesCount * 4);
#else
MD_MAX72XX display =
  MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, matrixModulesCount * 4);
#endif

MD_MAX72XX_Print textDisplay(&display);
MD_MAX72XX_Scrolling scrollingDisplay(&display);

void startWiFiConfigOverUSBAndReboot(const char* msg) {
  Serial1.println("Exposing FatFSUSB for WiFi settings editing");
  wifiSettings.fatFSUSBBegin();
  Serial1.println("USB connected, waiting for eject...");
  scrollingDisplay.setText(msg, true);
  bool hasPressedYet = false;
  while (wifiSettings.fatFSUSBConnected()) {
    scrollingDisplay.update();
    if (configBtn.pressed()) {
      hasPressedYet = true;
    }
    if (configBtn.released() && hasPressedYet) {
      Serial1.println("Config button pressed and released, stopping FatFSUSB");
      break;
    }
  }
  wifiSettings.fatFSUSBEnd();
  Serial1.println("Rebooting to try loading settings again");
  rp2040.reboot();
}

void startTickerConfigOverUSBAndReboot(const char* msg) {
  Serial1.println("Exposing FatFSUSB for Ticker settings editing");
  tickerSettings.fatFSUSBBegin();
  Serial1.println("USB connected, waiting for eject...");
  scrollingDisplay.setText(msg, true);
  bool hasPressedYet = false;
  while (tickerSettings.fatFSUSBConnected()) {
    scrollingDisplay.update();
    if (configBtn.pressed()) {
      hasPressedYet = true;
    }
    if (configBtn.released() && hasPressedYet) {
      Serial1.println("Config button pressed and released, stopping FatFSUSB");
      break;
    }
  }
  tickerSettings.fatFSUSBEnd();
  Serial1.println("Rebooting to try loading settings again");
  rp2040.reboot();
}

void setup() {
  Serial1.begin(115200);
  Serial1.println("\n");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // configBtn.begin();
  pinMode(CONFIG_BTN_PIN, INPUT);  // Using external pull up

#ifdef USE_HARDWARE_SPI
  SPI.setSCK(CLK_PIN);
  SPI.setTX(DATA_PIN);
  SPI.setCS(CS_PIN);
  SPI.begin();
#endif
  display.begin();
  display.clear();
  display.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  display.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY / 2);

  const Settings::LoadFromDiskResult r = wifiSettings.loadFromDisk();
  // If fail to load WiFi settings, start WiFi configuration over USB
  if (r != Settings::LoadFromDiskResult::OK) {
    Serial1.printf("Failed to load settings from disk: %s\n", r);
    if (r == Settings::LoadFromDiskResult::ERROR_FILE_OPEN_FAILED) {
      // Write default settings because file not found
      wifiSettings.saveToDisk();
    }
    switch (r) {
      case Settings::LoadFromDiskResult::ERROR_FATFS_INIT_FAILED:
        startWiFiConfigOverUSBAndReboot(
          "Failed to initialize filesystem, eject USB drive to try again.");
      case Settings::LoadFromDiskResult::ERROR_FILE_OPEN_FAILED:
        startWiFiConfigOverUSBAndReboot(
          "Modify wifi_settings.json on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_TOO_DEEP:
        startWiFiConfigOverUSBAndReboot(
          "JSON too deep, modify wifi_settings.json on USB drive and eject to "
          "finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_NO_MEMORY:
        startWiFiConfigOverUSBAndReboot(
          "JSON parsing failed due to insufficient memory, modify "
          "wifi_settings.json on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_INVALID_INPUT:
        startWiFiConfigOverUSBAndReboot(
          "JSON parsing failed due to invalid input, modify wifi_settings.json "
          "on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_INCOMPLETE_INPUT:
        startWiFiConfigOverUSBAndReboot(
          "JSON parsing failed due to incomplete input, modify "
          "wifi_settings.json on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_EMPTY_INPUT:
        startWiFiConfigOverUSBAndReboot(
          "JSON parsing failed due to empty input, modify wifi_settings.json "
          "on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_UNKNOWN_ERROR:
        startWiFiConfigOverUSBAndReboot(
          "JSON parsing failed due to unknown error, modify wifi_settings.json "
          "on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_VALIDATION_FAILED:
        switch (static_cast<Settings::WiFiSettingsValidationResult>(
          wifiSettings.getLastValidationResult())) {
          case Settings::WiFiSettingsValidationResult::ERROR_INVALID_SSID:
            startWiFiConfigOverUSBAndReboot(
              "Invalid SSID, modify \"ssid\" key in wifi_settings.json on "
              "USB drive and eject to finish.");
          case Settings::WiFiSettingsValidationResult::ERROR_INVALID_PASSWORD:
            startWiFiConfigOverUSBAndReboot(
              "Invalid password, modify \"password\" key in "
              "wifi_settings.json on USB drive and eject to finish.");
          case Settings::WiFiSettingsValidationResult::OK:
            break;
        }
      case Settings::LoadFromDiskResult::OK:
        break;
    }
  }
  const Settings::LoadFromDiskResult r2 = tickerSettings.loadFromDisk();
  // If fail to load Ticker settings, start Ticker configuration over USB
  if (r2 != Settings::LoadFromDiskResult::OK) {
    Serial1.printf("Failed to load settings from disk: %s\n", r2);
    if (r2 == Settings::LoadFromDiskResult::ERROR_FILE_OPEN_FAILED) {
      // Write default settings because file not found
      tickerSettings.saveToDisk();
    }
    switch (r2) {
      case Settings::LoadFromDiskResult::ERROR_FATFS_INIT_FAILED:
        startTickerConfigOverUSBAndReboot(
          "Failed to initialize filesystem, eject USB drive to try again.");
      case Settings::LoadFromDiskResult::ERROR_FILE_OPEN_FAILED:
        startTickerConfigOverUSBAndReboot(
          "Modify ticker_settings.json on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_TOO_DEEP:
        startTickerConfigOverUSBAndReboot(
          "JSON too deep, modify ticker_settings.json on USB drive and eject "
          "to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_NO_MEMORY:
        startTickerConfigOverUSBAndReboot(
          "JSON parsing failed due to insufficient memory, modify "
          "ticker_settings.json on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_INVALID_INPUT:
        startTickerConfigOverUSBAndReboot(
          "JSON parsing failed due to invalid input, modify "
          "ticker_settings.json on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_INCOMPLETE_INPUT:
        startTickerConfigOverUSBAndReboot(
          "JSON parsing failed due to incomplete input, modify "
          "ticker_settings.json on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_EMPTY_INPUT:
        startTickerConfigOverUSBAndReboot(
          "JSON parsing failed due to empty input, modify ticker_settings.json "
          "on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_JSON_PARSE_UNKNOWN_ERROR:
        startTickerConfigOverUSBAndReboot(
          "JSON parsing failed due to unknown error, modify "
          "ticker_settings.json on USB drive and eject to finish.");
      case Settings::LoadFromDiskResult::ERROR_VALIDATION_FAILED:
        switch (static_cast<Settings::TickerSettingsValidationResult>(
          tickerSettings.getLastValidationResult())) {
          case Settings::TickerSettingsValidationResult::
            ERROR_INVALID_APCA_API_KEY_ID:
            startTickerConfigOverUSBAndReboot(
              "Invalid Alpaca Markets API Key ID, modify \"apcaApiKeyId\" key "
              "in ticker_settings.json on USB drive and eject to finish.");
          case Settings::TickerSettingsValidationResult::
            ERROR_INVALID_APCA_API_SECRET_KEY:
            startTickerConfigOverUSBAndReboot(
              "Invalid Alpaca Markets API Secret Key, modify "
              "\"apcaApiSecretKey\" key in ticker_settings.json on USB drive "
              "and eject to finish.");
          case Settings::TickerSettingsValidationResult::ERROR_INVALID_SYMBOLS:
            startTickerConfigOverUSBAndReboot(
              "Invalid symbols, modify \"symbols\" key (must be comma "
              "separated list of 1 to 32 stock symbols) in "
              "ticker_settings.json on USB drive and eject to finish.");
          case Settings::TickerSettingsValidationResult::
            ERROR_INVALID_SOURCE_FEED:
            startTickerConfigOverUSBAndReboot(
              "Invalid source feed, modify \"sourceFeed\" key (must be "
              "\"sip\", \"iex\", \"delayed_sip\", \"boats\", \"overnight\", or "
              "\"otc\") in ticker_settings.json on USB drive and eject to "
              "finish.");
          case Settings::TickerSettingsValidationResult::
            ERROR_INVALID_REQUEST_PERIOD:
            startTickerConfigOverUSBAndReboot(
              "Invalid request period, modify \"requestPeriod\" key (must be a "
              "natural number) in ticker_settings.json on USB drive and eject "
              "to finish.");
            break;
          case Settings::TickerSettingsValidationResult::
            ERROR_INVALID_SCROLL_PERIOD:
            startTickerConfigOverUSBAndReboot(
              "Invalid scroll period, modify \"scrollPeriod\" key (must be a "
              "natural number) in ticker_settings.json on USB drive and eject "
              "to finish.");
            break;
          case Settings::TickerSettingsValidationResult::
            ERROR_INVALID_DISPLAY_BRIGHTNESS:
            startTickerConfigOverUSBAndReboot(
              "Invalid display brightness, modify \"displayBrightness\" key "
              "(must be a natural number between 1 and 15 inclusive) in "
              "ticker_settings.json on USB drive and eject to finish.");
            break;
          case Settings::TickerSettingsValidationResult::OK:
            break;
        }
      case Settings::LoadFromDiskResult::OK:
        break;
    }
  }
  // If configuration buton pressed, start WiFi or Ticker configuration over USB
  if (configBtn.pressed()) {
    Serial1.println("Config button pressed");
    startWiFiConfigOverUSBAndReboot(
      "Configuration button pressed, modify wifi_settings.json or "
      "ticker_settings.json on USB drive and eject to finish.");
  }

  Serial1.println(tickerSettings.symbols);
  stockTicker.begin(tickerSettings.apcaApiKeyId,
                    tickerSettings.apcaApiSecretKey, tickerSettings.symbols,
                    tickerSettings.sourceFeed,
                    tickerSettings.requestPeriod * 1000);

  scrollingDisplay.setText(stockTicker.getDisplayStr());
  scrollingDisplay.periodBetweenShifts = tickerSettings.scrollPeriod;
  display.control(MD_MAX72XX::INTENSITY, tickerSettings.displayBrightness);
}

void loop() {
  static StockTicker::StockTickerStatus lastStatus =
    StockTicker::StockTickerStatus::OK;

  // If configuration button pressed, start WiFi configuration over USB
  if (configBtn.pressed()) {
    Serial1.println("Config button pressed");
    Serial1.println("Stopping WiFi and rebooting");
    WiFi.end();
    rp2040.reboot();
  }
  if (WiFi.status() == WL_CONNECTED) {
    stockTicker.update();
    scrollingDisplay.update();
    if (stockTicker.getStatus() != lastStatus) {
      lastStatus = stockTicker.getStatus();
      Serial1.printf("Stock ticker status changed: %s\n", lastStatus);
      switch (lastStatus) {
        case StockTicker::StockTickerStatus::OK:
          scrollingDisplay.setText(stockTicker.getDisplayStr());
          break;
        case StockTicker::StockTickerStatus::ERROR_NO_WIFI:
          scrollingDisplay.setText(
            "No WiFi, check connection or credentials, trying again later.");
          break;
        case StockTicker::StockTickerStatus::ERROR_INIT_REQUEST_FAILED:
          scrollingDisplay.setText(
            "Failed to initialize request, trying again later.");
          break;
        case StockTicker::StockTickerStatus::ERROR_CONNECTION_FAILED:
          // Fallthrough
        case StockTicker::StockTickerStatus::ERROR_SEND_HEADER_FAILED:
          // Fallthrough
        case StockTicker::StockTickerStatus::ERROR_SEND_PAYLOAD_FAILED:
          scrollingDisplay.setText("Bad connection, check WiFi connection or "
                                   "credentials, trying again later.");
          break;
        case StockTicker::StockTickerStatus::ERROR_BAD_JSON_RESPONSE:
          scrollingDisplay.setText(
            "Bad response from server, trying again later.");
          break;
        case StockTicker::StockTickerStatus::ERROR_BAD_REQUEST:
          // Bad request, start Ticker configuration over USB
          startTickerConfigOverUSBAndReboot(
            "Bad request, modify ticker_settings.json on USB drive and eject "
            "to finish.");
          break;
        case StockTicker::StockTickerStatus::ERROR_FORBIDDEN:
          // Forbidden, start Ticker configuration over USB
          startTickerConfigOverUSBAndReboot(
            "Forbidden, modify \"apcaApiKeyId\" and/or \"apcaApiSecretKey\" "
            "key in ticker_settings.json on USB drive and eject to finish.");
          break;
        case StockTicker::StockTickerStatus::ERROR_TOO_MANY_REQUESTS:
          scrollingDisplay.setText("Too many requests, upgrade Alpaca Markets "
                                   "account, trying again later.");
          break;
        case StockTicker::StockTickerStatus::ERROR_INTERNAL_SERVER_ERROR:
          scrollingDisplay.setText(
            "Internal server error, check Alpaca Markets' Slack or Community "
            "Forum, trying again later.");
          break;
        case StockTicker::StockTickerStatus::ERROR_UNKNOWN:
          scrollingDisplay.setText("Unknown error, trying again later.");
          break;
      }
    }
  } else {
    Serial1.println("Connecting to WiFi...");
    textDisplay.print("Connecting to WiFi...");
    display.update();
    WiFi.begin(wifiSettings.ssid, wifiSettings.password);
    delay(1000);
    // If WiFi connection fails, start WiFi configuration over USB
    if (WiFi.status() != WL_CONNECTED) {
      Serial1.println("WiFi connection failed");
      startWiFiConfigOverUSBAndReboot(
        "WiFi connection failed, modify \"ssid\" and/or \"password\" in "
        "wifi_settings.json on USB drive and eject to finish.");
    }
    Serial1.println("Connected to WiFi");
    textDisplay.print("\nConnected to WiFi");
    display.update();
    Serial1.print("IP Address: ");
    Serial1.println(WiFi.localIP());
    delay(1000);
    scrollingDisplay.reset();
    stockTicker.refreshOnNextUpdate();
  }
}
