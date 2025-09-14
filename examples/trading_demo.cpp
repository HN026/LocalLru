#include "../include/locallru/local_lru.hpp"
#include "../src/lock_cache.hpp"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <mutex>

using namespace locallru;
using namespace lockedlru;

struct PriceData {
    std::vector<double> prices;
};

std::mutex cout_mutex;

PriceData preload_csv(const std::string& filepath, const std::string& sym) {
    PriceData pdata;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "Error opening file: " << filepath << std::endl;
        return pdata;
    }

    std::string line;
    std::getline(file, line); // skip header
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string date, open, high, low, close, adjclose, volume;
        std::getline(ss, date, ',');       
        std::getline(ss, open, ',');       
        std::getline(ss, high, ',');       
        std::getline(ss, low, ',');        
        std::getline(ss, close, ',');      
        std::getline(ss, adjclose, ',');   
        std::getline(ss, volume, ',');     

        if (close.empty() || close == "null" || close == "N/A") continue;

        try {
            pdata.prices.push_back(std::stod(close));
        } catch (const std::exception&) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cerr << "Warning: skipping invalid price '" << close << "' for symbol " << sym << std::endl;
        }
    }

    return pdata;
}

int main() {
    std::filesystem::create_directories("../results");

    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOG", "TSLA"};
    std::vector<PriceData> all_data(symbols.size());

    // Preload CSVs
    for (size_t i = 0; i < symbols.size(); i++) {
        std::string path = "../data/" + symbols[i] + ".csv";
        all_data[i] = preload_csv(path, symbols[i]);
    }

    // Setup caches
    auto lockfree = LocalCache<double>::initialize(1000, 0); // no TTL
    LockCache<std::string, double> locking(1000);

    std::ofstream log_file("../results/trading_benchmark.log");
    log_file << "[Benchmark Start]\nSymbols: ";
    for (auto& s : symbols) log_file << s << " ";
    log_file << "\n---------------------------------\n";

    auto start_total = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < symbols.size(); i++) {
        auto& pdata = all_data[i];

        log_file << symbols[i] << " processing " << pdata.prices.size() << " rows\n";

        for (auto price : pdata.prices) {
            // Lock-free
            auto t1_start = std::chrono::high_resolution_clock::now();
            lockfree.add_item(symbols[i], price);
            auto val1 = lockfree.get_item(symbols[i]);
            auto t1_end = std::chrono::high_resolution_clock::now();

            // Locking
            auto t2_start = std::chrono::high_resolution_clock::now();
            locking.put(symbols[i], price);
            auto val2 = locking.get(symbols[i]);
            auto t2_end = std::chrono::high_resolution_clock::now();

            log_file << symbols[i] << " price=" << price 
                     << " lockfree_ns=" 
                     << std::chrono::duration_cast<std::chrono::nanoseconds>(t1_end - t1_start).count()
                     << " locking_ns=" 
                     << std::chrono::duration_cast<std::chrono::nanoseconds>(t2_end - t2_start).count()
                     << "\n";
        }
    }

    auto end_total = std::chrono::high_resolution_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end_total - start_total).count();
    log_file << "Total elapsed time (s): " << elapsed_sec << "\n";
    log_file << "[Benchmark End]\n";

    std::cout << "Benchmark complete. Results written to ../results/trading_benchmark.log\n";

    return 0;
}
