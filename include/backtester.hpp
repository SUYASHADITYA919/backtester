#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <fstream>
#include <sstream>
#include <cmath>

// ==========================================
// 1. EVENTS SYSTEM
// ==========================================
enum class EventType { MARKET, SIGNAL, ORDER, FILL };
enum class OrderDirection { BUY, SELL, HOLD };

struct Event {
    EventType type;
    virtual ~Event() = default;
};

struct MarketEvent : public Event {
    std::string timestamp;
    std::string symbol;
    double close_price;
    MarketEvent(std::string ts, std::string sym, double price) 
        : timestamp(ts), symbol(sym), close_price(price) { type = EventType::MARKET; }
};

struct SignalEvent : public Event {
    std::string symbol;
    std::string timestamp;
    OrderDirection direction;
    SignalEvent(std::string sym, std::string ts, OrderDirection dir) 
        : symbol(sym), timestamp(ts), direction(dir) { type = EventType::SIGNAL; }
};

struct OrderEvent : public Event {
    std::string symbol;
    OrderDirection direction;
    int quantity;
    OrderEvent(std::string sym, OrderDirection dir, int qty) 
        : symbol(sym), direction(dir), quantity(qty) { type = EventType::ORDER; }
};

struct FillEvent : public Event {
    std::string timestamp;
    std::string symbol;
    OrderDirection direction;
    int quantity;
    double fill_price;
    double commission;
    FillEvent(std::string ts, std::string sym, OrderDirection dir, int qty, double price, double comm) 
        : timestamp(ts), symbol(sym), direction(dir), quantity(qty), fill_price(price), commission(comm) { type = EventType::FILL; }
};

// ==========================================
// 2. DATA HANDLER
// ==========================================
struct BarData {
    std::string timestamp;
    double close;
};

class DataHandler {
private:
    std::string csv_path;
    std::string symbol;
    std::vector<BarData> data_store;
    size_t current_index = 0;

public:
    DataHandler(std::string path, std::string sym) : csv_path(path), symbol(sym) {}

    bool load_csv() {
        std::ifstream file(csv_path);
        if (!file.is_open()) {
            std::cerr << "Failed to open CSV file: " << csv_path << "\n";
            return false;
        }
        std::string line;
        std::getline(file, line); // Skip CSV header
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string ts, val;
            std::getline(ss, ts, ',');
            std::getline(ss, val, ',');
            if(!ts.empty() && !val.empty()) {
                data_store.push_back({ts, std::stod(val)});
            }
        }
        return !data_store.empty();
    }

    bool get_next_bar(MarketEvent& next_event) {
        if (current_index >= data_store.size()) return false;
        auto& bar = data_store[current_index++];
        next_event = MarketEvent(bar.timestamp, symbol, bar.close);
        return true;
    }
};

// ==========================================
// 3. TRADING STRATEGY (Moving Average Crossover)
// ==========================================
class MovingAverageStrategy {
private:
    size_t short_window;
    size_t long_window;
    std::vector<double> prices;

public:
    MovingAverageStrategy(size_t short_w, size_t long_w) : short_window(short_w), long_window(long_w) {}

    SignalEvent calculate_signal(const MarketEvent& event) {
        prices.push_back(event.close_price);
        if (prices.size() < long_window) {
            return SignalEvent(event.symbol, event.timestamp, OrderDirection::HOLD);
        }

        double short_ma = 0.0, long_ma = 0.0;
        for (size_t i = prices.size() - short_window; i < prices.size(); ++i) short_ma += prices[i];
        for (size_t i = prices.size() - long_window; i < prices.size(); ++i) long_ma += prices[i];
        short_ma /= short_window;
        long_ma /= long_window;

        // Simple previous bar state to look for a crossover
        double prev_short_ma = 0.0, prev_long_ma = 0.0;
        for (size_t i = prices.size() - short_window - 1; i < prices.size() - 1; ++i) prev_short_ma += prices[i];
        for (size_t i = prices.size() - long_window - 1; i < prices.size() - 1; ++i) prev_long_ma += prices[i];
        prev_short_ma /= short_window;
        prev_long_ma /= long_window;

        if (prev_short_ma <= prev_long_ma && short_ma > long_ma) {
            return SignalEvent(event.symbol, event.timestamp, OrderDirection::BUY);
        } else if (prev_short_ma >= prev_long_ma && short_ma < long_ma) {
            return SignalEvent(event.symbol, event.timestamp, OrderDirection::SELL);
        }
        return SignalEvent(event.symbol, event.timestamp, OrderDirection::HOLD);
    }
};

// ==========================================
// 4. PORTFOLIO & EXECUTION ENGINE
// ==========================================
class Portfolio {
public:
    double cash;
    double total_equity;
    int current_position = 0;
    double last_known_price = 0.0;
    std::vector<double> equity_curve;

    Portfolio(double initial_capital) : cash(initial_capital), total_equity(initial_capital) {}

    void handle_market_update(const MarketEvent& event) {
        last_known_price = event.close_price;
        total_equity = cash + (current_position * last_known_price);
        equity_curve.push_back(total_equity);
    }

    OrderEvent handle_signal(const SignalEvent& signal) {
        // Simple execution rule: Buy 10 shares or Sell 10 shares
        if (signal.direction == OrderDirection::BUY && current_position == 0) {
            return OrderEvent(signal.symbol, OrderDirection::BUY, 10);
        } else if (signal.direction == OrderDirection::SELL && current_position > 0) {
            return OrderEvent(signal.symbol, OrderDirection::SELL, current_position);
        }
        return OrderEvent(signal.symbol, OrderDirection::HOLD, 0);
    }

    FillEvent simulate_execution(const OrderEvent& order, const std::string& timestamp) {
        double commission = 1.00; // Flat $1 per trade broker fee simulation
        return FillEvent(timestamp, order.symbol, order.direction, order.quantity, last_known_price, commission);
    }

    void apply_fill(const FillEvent& fill) {
        if (fill.direction == OrderDirection::BUY) {
            current_position += fill.quantity;
            cash -= (fill.quantity * fill.fill_price) + fill.commission;
        } else if (fill.direction == OrderDirection::SELL) {
            current_position -= fill.quantity;
            cash += (fill.quantity * fill.fill_price) - fill.commission;
        }
        total_equity = cash + (current_position * last_known_price);
        std::cout << "[" << fill.timestamp << "] EXECUTION: " 
                  << (fill.direction == OrderDirection::BUY ? "BUY " : "SELL ") 
                  << fill.quantity << " units at $" << fill.fill_price 
                  << " | Equity: $" << total_equity << "\n";
    }

    void print_performance_summary() {
        std::cout << "\n==============================\n";
        std::cout << "     PERFORMANCE REPORT       \n";
        std::cout << "==============================\n";
        std::cout << "Final Total Equity: $" << total_equity << "\n";
        std::cout << "Remaining Cash:     $" << cash << "\n";
        std::cout << "Open Position Size: " << current_position << "\n";
    }
};