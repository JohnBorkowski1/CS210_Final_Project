#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <unordered_map>

using namespace std;
using namespace std::chrono;

vector<pair<string, string>> generateQueries(int numQueries) {
    vector<pair<string, string>> queries;
    vector<string> cities = {"andorra la vella", "canillo", "encamp", "la massana"};
    vector<string> countries = {"ad", "us", "gb", "fr"};
    default_random_engine rng(time(nullptr));
    uniform_int_distribution<int> cityDist(0, cities.size() - 1);
    uniform_int_distribution<int> countryDist(0, countries.size() - 1);

    for (int i = 0; i < numQueries; ++i) {
        string city = cities[cityDist(rng)];
        string country = countries[countryDist(rng)];
        queries.push_back({city, country});
    }
    return queries;
}

void logPerformanceMetrics(int totalLookups, int cacheHits, double totalTime) {
    double hitRate = (double)cacheHits / totalLookups * 100;
    double avgTime = totalTime / totalLookups;
    cout << "Total Lookups: " << totalLookups << endl;
    cout << "Cache Hits: " << cacheHits << " (" << hitRate << "%)" << endl;
    cout << "Average Lookup Time: " << avgTime << " seconds" << endl;
}

int main() {
    int numQueries = 1000;
    vector<pair<string, string>> queries = generateQueries(numQueries);

    CityTrie trie;
    ICache* cache;

    cache = new LFUCache(10);
    int cacheHits = 0;
    auto start = high_resolution_clock::now();

    for (const auto& query : queries) {
        string countryCode = query.second;
        string cityName = query.first;
        CityKey key{countryCode, cityName};
        double population;

        if (cache->get(key, population)) {
            cacheHits++;
        } else if (trie.search(cityName, countryCode, population)) {
            cache->put(key, population);
        }
    }

    auto end = high_resolution_clock::now();
    double totalTime = duration_cast<duration<double>>(end - start).count();
    cout << "LFU Cache Performance:" << endl;
    logPerformanceMetrics(numQueries, cacheHits, totalTime);
    cache->displayCache();

    cacheHits = 0;
    cache = new FIFOCache(10);
    start = high_resolution_clock::now();
    for (const auto& query : queries) {
        string countryCode = query.second;
        string cityName = query.first;
        CityKey key{countryCode, cityName};
        double population;

        if (cache->get(key, population)) {
            cacheHits++;
        } else if (trie.search(cityName, countryCode, population)) {
            cache->put(key, population);
        }
    }
    end = high_resolution_clock::now();
    totalTime = duration_cast<duration<double>>(end - start).count();
    cout << "FIFO Cache Performance:" << endl;
    logPerformanceMetrics(numQueries, cacheHits, totalTime);
    cache->displayCache();

    cacheHits = 0;
    cache = new RandomCache(10);
    start = high_resolution_clock::now();
    for (const auto& query : queries) {
        string countryCode = query.second;
        string cityName = query.first;
        CityKey key{countryCode, cityName};
        double population;

        if (cache->get(key, population)) {
            cacheHits++;
        } else if (trie.search(cityName, countryCode, population)) {
            cache->put(key, population);
        }
    }
    end = high_resolution_clock::now();
    totalTime = duration_cast<duration<double>>(end - start).count();
    cout << "Random Cache Performance:" << endl;
    logPerformanceMetrics(numQueries, cacheHits, totalTime);
    cache->displayCache();

    return 0;
}
