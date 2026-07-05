#include "backtester.hpp"

int main() {
    std::string ticker = "AAPL";
    
    // Core Engine Component Objects
    std::queue<std::shared_ptr<Event>> event_queue;
    DataHandler data_handler("data.csv", ticker);
    MovingAverageStrategy strategy(5, 20); // 5-period vs 20-period Crossover
    Portfolio portfolio(10000.0);          // Start with $10,000 cash

    if (!data_handler.load_csv()) {
        return -1;
    }

    std::cout << "Starting Backtest Engine for " << ticker << "...\n";

    MarketEvent next_market_bar("", "", 0.0);
    
    // OUTER LOOP: Simulates real-time market historical delivery
    while (data_handler.get_next_bar(next_market_bar)) {
        event_queue.push(std::make_shared<MarketEvent>(next_market_bar));

        // INNER LOOP: Process events cascade sequentially
        while (!event_queue.empty()) {
            auto event = event_queue.front();
            event_queue.pop();

            if (event->type == EventType::MARKET) {
                auto m_event = std::static_pointer_cast<MarketEvent>(event);
                portfolio.handle_market_update(*m_event);
                
                SignalEvent sig = strategy.calculate_signal(*m_event);
                if (sig.direction != OrderDirection::HOLD) {
                    event_queue.push(std::make_shared<SignalEvent>(sig));
                }
            } 
            else if (event->type == EventType::SIGNAL) {
                auto s_event = std::static_pointer_cast<SignalEvent>(event);
                OrderEvent ord = portfolio.handle_signal(*s_event);
                if (ord.direction != OrderDirection::HOLD) {
                    event_queue.push(std::make_shared<OrderEvent>(ord));
                }
            } 
            else if (event->type == EventType::ORDER) {
                auto o_event = std::static_pointer_cast<OrderEvent>(event);
                FillEvent fill = portfolio.simulate_execution(*o_event, next_market_bar.timestamp);
                event_queue.push(std::make_shared<FillEvent>(fill));
            } 
            else if (event->type == EventType::FILL) {
                auto f_event = std::static_pointer_cast<FillEvent>(event);
                portfolio.apply_fill(*f_event);
            }
        }
    }

    portfolio.print_performance_summary();
    return 0;
}