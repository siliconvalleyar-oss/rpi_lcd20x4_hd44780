#include "BitcoinClient.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Callback para escribir la respuesta de la API
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total = size * nmemb;
    output->append((char*)contents, total);
    return total;
}

BitcoinClient::BitcoinClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

BitcoinClient::~BitcoinClient() {
    curl_global_cleanup();
}

std::string BitcoinClient::fetchFromAPI() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Error: No se pudo inicializar CURL" << std::endl;
        return "";
    }

    // Usar CoinGecko API (no requiere API key)
    // Incluye precio y cambio porcentual en 24h
    std::string url = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Timeout de 10 segundos
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RaspberryPi-LCD-Ticker/1.0");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Error HTTP: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    return response;
}

double BitcoinClient::parsePrice(const std::string& response) {
    if (response.empty()) return 0.0;
    
    try {
        json data = json::parse(response);
        if (data.contains("bitcoin") && data["bitcoin"].contains("usd")) {
            return data["bitcoin"]["usd"].get<double>();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error al parsear JSON: " << e.what() << std::endl;
    }
    return 0.0;
}

double BitcoinClient::parseChangePercentage(const std::string& response) {
    if (response.empty()) return 0.0;
    
    try {
        json data = json::parse(response);
        if (data.contains("bitcoin") && data["bitcoin"].contains("usd_24h_change")) {
            return data["bitcoin"]["usd_24h_change"].get<double>();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error al parsear cambio porcentual: " << e.what() << std::endl;
    }
    return 0.0;
}

double BitcoinClient::getCurrentPrice() {
    std::string response = fetchFromAPI();
    return parsePrice(response);
}

double BitcoinClient::getPriceChangePercentage() {
    std::string response = fetchFromAPI();
    return parseChangePercentage(response);
}

std::string BitcoinClient::getFormattedPrice() {
    double price = getCurrentPrice();
    if (price <= 0.0) {
        return "  Error    ";
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "$" << price;
    
    std::string result = ss.str();
    // Limitar a 12 caracteres para que quepa en el LCD
    if (result.length() > 12) {
        result = result.substr(0, 12);
    }
    return result;
}
