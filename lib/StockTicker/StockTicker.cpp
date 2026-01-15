//
// Created by ckyiu on 7/8/2025.
//

#include <StockTicker.h>

namespace StockTicker {
  uint16_t stockSymbolsCount(const char* symbolsString) {
    char str[MAX_SYMBOLS_STRING_LEN];
    strncpy(str, symbolsString, MAX_SYMBOLS_STRING_LEN);
    char* token;
    char* rest = str;
    uint16_t symbolCount = 0;
    while ((token = strtok_r(rest, ",", &rest))) {
      if (strlen(token) < MAX_ID_LEN) {
        //        strncpy(allSymbolPrices[this->symbolCount].id, token,
        //        MAX_ID_LEN);
        //        // If price is negative than no data yet
        //        allSymbolPrices[this->symbolCount].price = -1;
        //        Serial1.printf("Symbol '%s' initialized at index %d\n", token,
        //                       this->symbolCount);
        symbolCount++;
      } else {
        //        Serial1.printf("Symbol '%s' is too long, skipping.\n", token);
      }
    }
    return symbolCount;
  }

  /**
   * @brief Initialize with parameters
   *
   * This function initializes the StockTicker with a comma-separated list of
   * stocks, (up to a configured maximum) the feed to use, and the time between
   * each request.
   *
   * @param apiKeyId Your Alpaca Markets API key ID.
   * @param apiSecretKey Your Alpaca Markets API secret key.
   * @param symbolsString The comma-separated list of stocks to track.
   * @param feed What feed to use. Either "sip", "iex", "delayed_sip", "boats",
   *  "overnight", or "otc". Only iex or delayed_sip are available with a free
   *  account. The default is "iex".
   * @param request The time between each request in milliseconds. The default
   *  is 60 seconds.
   */
  void StockTicker::begin(const char* apiKeyId, const char* apiSecretKey,
                          const char* symbolsString,
                          const char* feed /* = "iex" */,
                          uint32_t request /* = 60 * 1000*/) {
    this->apcaApiKeyId = apiKeyId;
    this->apcaApiSecretKey = apiSecretKey;
    this->sourceFeed = feed;
    this->requestPeriod = request;
    // Parse comma-separated symbols string
    this->symbols = symbolsString;
    memset(this->allSymbolPrices, 0, sizeof(this->allSymbolPrices));
    // Temp string for strtok_r
    char str[MAX_SYMBOLS_STRING_LEN];
    strncpy(str, this->symbols, MAX_SYMBOLS_STRING_LEN);
    char* token;
    char* rest = str;
    this->symbolCount = 0;
    while ((token = strtok_r(rest, ",", &rest)) &&
           this->symbolCount < MAX_SYMBOLS) {
      if (strlen(token) < MAX_ID_LEN) {
        strncpy(this->allSymbolPrices[this->symbolCount].id, token, MAX_ID_LEN);
        // If price is negative than no data yet
        allSymbolPrices[this->symbolCount].price = -1;
        Serial1.printf("Symbol '%s' initialized at index %d\n", token,
                       this->symbolCount);
        this->symbolCount++;
      } else {
        Serial1.printf("Symbol '%s' is too long, skipping.\n", token);
      }
    }
    this->nextRequestTime = 0; // Update as soon as possible
  }

  #define RESCHEDULE_MACRO()                                                     \
  {                                                                            \
    Serial1.printf("Next request in %d seconds", this->requestPeriod / 1000);  \
    this->nextRequestTime = millis() + this->requestPeriod;                    \
  }
  /**
   * @brief Update the StockTicker.
   *
   * This function should be called periodically to update the StockTicker. It
   * will check if it's time to make a request to the API and update the symbol
   * prices accordingly.
   */
  void StockTicker::update() {
    if (static_cast<int32_t>(millis() - this->nextRequestTime) < 0) {
      return; // Not time to request yet
    }
    Serial1.println("Time to request data from Alpaca Markets API");

    if (WiFi.status() != WL_CONNECTED) {
      Serial1.println("No WiFi connection, cannot update stock prices.");
      this->status = StockTickerStatus::ERROR_NO_WIFI;
      RESCHEDULE_MACRO()
      return;
    }

    #ifdef LOG_FREE_MEMORY
    Serial1.printf("Free memory before request: heap %d kb, stack %d kb\n",
                   rp2040.getFreeHeap() / 1024, rp2040.getFreeStack() / 1024);
    #endif
    {
      // Scope to destroy client and parser to print free memory after request
      // https://data.alpaca.markets/v2/stocks/snapshots?symbols={SYMBOLS}&feed={FEED}
      HTTPClient httpsClient;
      httpsClient.setInsecure();
      httpsClient.useHTTP10(true);
      const size_t MAX_URL_LEN = 80 + MAX_SYMBOLS_STRING_LEN;
      char url[MAX_URL_LEN];
      snprintf(
        url, MAX_URL_LEN,
        "https://data.alpaca.markets/v2/stocks/snapshots?symbols=%s&feed=%s",
        this->symbols, this->sourceFeed);
      Serial1.printf("Requesting %s\n", url);
      if (httpsClient.begin(url)) {
        httpsClient.addHeader("Accept", "application/json");
        httpsClient.addHeader("Apca-Api-Key-Id", this->apcaApiKeyId);
        httpsClient.addHeader("Apca-Api-Secret-Key", this->apcaApiSecretKey);
        int32_t statusCode = httpsClient.GET();
        #ifdef LOG_FREE_MEMORY
        Serial1.printf(
          "Free memory after sending request: heap %d kb, stack %d kb\n",
          rp2040.getFreeHeap() / 1024, rp2040.getFreeStack() / 1024);
        #endif
        #ifdef BUFFER_JSON_READING
        ReadBufferingClient bufferedClient(httpsClient.getStream(), 256);
        #endif
        if (statusCode == 200) {
          // OK
          JsonDocument doc;
          #ifdef LOG_JSON_PARSED
          Serial1.println("JSON read:");
          #ifdef BUFFER_JSON_READING
          ReadLoggingStream loggingStream(bufferedClient, Serial1);
          #else
          ReadLoggingStream loggingStream(httpsClient.getStream(), Serial1);
          #endif
          DeserializationError error = deserializeJson(doc, loggingStream);
          Serial1.println("");
          #else
          #ifdef BUFFER_JSON_READING
          DeserializationError error = deserializeJson(doc, bufferedClient);
          #else
          DeserializationError error =
            deserializeJson(doc, httpsClient.getStream());
          #endif
          #endif
          #ifdef LOG_FREE_MEMORY
          Serial1.printf(
            "Free memory after JSON parse: heap %d kb, stack %d kb\n",
            rp2040.getFreeHeap() / 1024, rp2040.getFreeStack() / 1024);
          #endif
          if (error) {
            Serial1.printf("Failed to parse JSON: %s\n", error.c_str());
            this->status = StockTickerStatus::ERROR_BAD_JSON_RESPONSE;
          } else {
            for (JsonPair snapshot : doc.as<JsonObject>()) {
              const char* symbol = snapshot.key().c_str();
              JsonObject daily_bar = snapshot.value()["dailyBar"];
              float open_price = daily_bar["o"]; // Start of day price
              float close_price = daily_bar["c"]; // End of day / current price
              this->updateSymbolPriceInMemory(
                symbol, close_price, close_price - open_price,
                ((close_price - open_price) / open_price) * 100.0f);
            }
            this->updateDisplayStr();
          }
        } else {
          Serial1.printf("Bad status code: %d\n", statusCode);
          switch (statusCode) {
            case HTTPC_ERROR_CONNECTION_FAILED: {
              Serial1.println("Connection failed, check WiFi connection");
              this->status = StockTickerStatus::ERROR_CONNECTION_FAILED;
              break;
            }
            case HTTPC_ERROR_SEND_HEADER_FAILED: {
              Serial1.println("Failed to send header, check WiFi connection");
              this->status = StockTickerStatus::ERROR_SEND_HEADER_FAILED;
              break;
            }
            case HTTPC_ERROR_SEND_PAYLOAD_FAILED: {
              Serial1.println("Failed to send payload, check WiFi connection");
              this->status = StockTickerStatus::ERROR_SEND_PAYLOAD_FAILED;
              break;
            }
            case 400: {
              Serial1.println("Bad request, check API key and secret");
              this->status = StockTickerStatus::ERROR_BAD_REQUEST;
              break;
            }
            case 403: {
              Serial1.println("Forbidden, check API key and secret");
              this->status = StockTickerStatus::ERROR_FORBIDDEN;
              break;
            }
            case 429: {
              Serial1.println("Too many requests, check request period");
              this->status = StockTickerStatus::ERROR_TOO_MANY_REQUESTS;
              break;
            }
            case 500: {
              Serial1.println("Internal server error, check Alpaca Markets' "
                "Slack or Community Forum and try again later");
              this->status = StockTickerStatus::ERROR_INTERNAL_SERVER_ERROR;
              break;
            }
            default: {
              Serial1.printf("Unknown error, status code: %d\n", statusCode);
              this->status = StockTickerStatus::ERROR_UNKNOWN;
              break;
            }
          }
          Serial1.println("Response: ");
          #ifdef BUFFER_JSON_READING
          while (bufferedClient.available()) {
            Serial1.write(bufferedClient.read());
          }
          #else
          while (httpsClient.getStream().available()) {
            Serial1.write(httpsClient.getStream().read());
          }
          #endif
          return;
        }
      } else {
        Serial1.println("Failed to initialize request");
        this->status = StockTickerStatus::ERROR_INIT_REQUEST_FAILED;
      }
    }
    #ifdef LOG_FREE_MEMORY
    Serial1.printf("Free memory after request: heap %d kb, stack %d kb\n",
                   rp2040.getFreeHeap() / 1024, rp2040.getFreeStack() / 1024);
    #endif

    this->status = StockTickerStatus::OK;
    RESCHEDULE_MACRO()
  }
  #undef RESCHEDULE_MACRO

  /**
   * @brief Updates the symbol with new price, change, and change percent
   * data.
   *
   * @param id The symbol of the stock.
   * @param price The new price of the stock.
   * @param change The change in price of the stock.
   * @param changePercent The change percent of the stock.
   */
  void StockTicker::updateSymbolPriceInMemory(const char* id, float price,
                                              float change,
                                              float changePercent) {
    for (auto& allSymbolPrice : this->allSymbolPrices) {
      if (strcmp(allSymbolPrice.id, id) == 0) {
        allSymbolPrice.price = price;
        allSymbolPrice.change = change;
        allSymbolPrice.changePercent = changePercent;
        Serial1.printf("Updated symbol %s in symbol data list (price: %.2f, "
                       "change: %.2f, changePercent: %.2f%%)\n",
                       allSymbolPrice.id, price, change, changePercent);
        return;
      }
    }
    Serial1.printf("Symbol %s not found in symbol data list\n", id);
  }

  /**
   * @brief Updates the stock string to display.
   */
  void StockTicker::updateDisplayStr() {
    memset(displayStr, 0, MAX_DISPLAY_STR_LEN);
    char* ptr = displayStr;
    for (uint16_t i = 0; i < this->symbolCount; i++) {
      size_t charsWritten = 0;
      if (allSymbolPrices[i].price > 0) {
        char sign = '+';
        if (allSymbolPrices[i].change < 0) {
          sign = '-';
        }
        charsWritten =
          snprintf(ptr, MAX_SYMBOL_DISPLAY_STR_LEN,
                   "%s: $%.2f %+.2f%% (%c$%.2f)    ", allSymbolPrices[i].id,
                   allSymbolPrices[i].price, allSymbolPrices[i].changePercent,
                   sign, abs(allSymbolPrices[i].change));
      } else {
        // No data yet cause price is negative
        charsWritten =
          snprintf(ptr, MAX_SYMBOL_DISPLAY_STR_LEN, "%s: No data yet...    ",
                   allSymbolPrices[i].id);
      }
      ptr += charsWritten;
    }
    Serial1.println("Display string updated:");
    Serial1.println(displayStr);
  }
} // StockTicker
