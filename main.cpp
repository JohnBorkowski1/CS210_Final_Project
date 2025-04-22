#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <list>
#include <string>
#include <algorithm>

using namespace std;

struct CityKey {
    string countryCode;
    string cityName;

    bool operator==(const CityKey& other) const {
        return countryCode == other.countryCode && cityName == other.cityName;
    }
};

struct CityKeyHasher {
    size_t operator()(const CityKey& key) const {
        return hash<string>()(key.countryCode) ^ hash<string>()(key.cityName);
    }
};

class LRUCache {
private:
    int capacity;
    list<pair<CityKey, double>> lruList;
    unordered_map<CityKey, list<pair<CityKey, double>>::iterator, CityKeyHasher> cacheMap;

public:
    LRUCache(int cap) : capacity(cap) {}

    bool get(const CityKey& key, double& population) {
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) return false;

        lruList.splice(lruList.begin(), lruList, it->second);
        population = it->second->second;
        return true;
    }

    void put(const CityKey& key, double population) {
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            // Update value and move to front
            it->second->second = population;
            lruList.splice(lruList.begin(), lruList, it->second);
        } else {
            // Add new entry
            if (lruList.size() >= capacity) {
                // Remove least recently used
                auto del = lruList.back();
                cacheMap.erase(del.first);
                lruList.pop_back();
            }
            lruList.emplace_front(key, population);
            cacheMap[key] = lruList.begin();
        }
    }

    void displayCache() const {
        cout << "Cache contents recent first:" << endl;
        for (const auto& entry : lruList) {
            cout << entry.first.countryCode << ", " << entry.first.cityName << " => " << entry.second << endl;
        }
    }
};

double searchCSV(const string& filename, const CityKey& key) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file" << endl;
        return -1;
    }

    string line;
    getline(file, line); // skip header

    while (getline(file, line)) {
        stringstream ss(line);
        string countryCode, cityName, populationStr;
        getline(ss, countryCode, ',');
        getline(ss, cityName, ',');
        getline(ss, populationStr);

        transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::tolower);
        transform(cityName.begin(), cityName.end(), cityName.begin(), ::tolower);

        if (countryCode == key.countryCode && cityName == key.cityName) {
            return stod(populationStr);
        }
    }

    return -1;
}

int main() {
    string filename = "city_population.csv";
    LRUCache cache(10);

    while (true) {
        string countryCode, cityName;
        cout << "\nEnter country code or exit to quit: ";
        cin >> countryCode;
        if (countryCode == "exit") break;
        cout << "Enter city name: ";
        cin.ignore();
        getline(cin, cityName);

        transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::tolower);
        transform(cityName.begin(), cityName.end(), cityName.begin(), ::tolower);

        CityKey key{countryCode, cityName};
        double population;

        if (cache.get(key, population)) {
            cout << "Population (from cache): " << population << endl;
        } else {
            population = searchCSV(filename, key);
            if (population >= 0) {
                cache.put(key, population);
                cout << "Population (from file): " << population << endl;
            } else {
                cout << "City not found." << endl;
            }
        }

        cache.displayCache();
    }

    return 0;
}
