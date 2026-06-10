#ifndef BITCOIN_CLIENT_HPP
#define BITCOIN_CLIENT_HPP

#include <string>

class BitcoinClient {
public:
    BitcoinClient();
    ~BitcoinClient();

    // Obtiene el precio actual de Bitcoin en USD
    // Retorna 0.0 si hay error
    double getCurrentPrice();

    // Obtiene el precio formateado como string (ej: "  87,234.12")
    std::string getFormattedPrice();

    // Obtiene el cambio porcentual en 24h
    double getPriceChangePercentage();

private:
    std::string fetchFromAPI();
    double parsePrice(const std::string& response);
    double parseChangePercentage(const std::string& response);
};

#endif
