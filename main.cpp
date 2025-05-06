#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <queue>
#include <string>
#include <random>
#include <algorithm>
#include <ctime>

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

class ICache {
public:
    virtual bool get(const CityKey& key, double& population) = 0;
    virtual void put(const CityKey& key, double population) = 0;
    virtual void displayCache() const = 0;
    virtual ~ICache() {}
};

class LFUCache : public ICache {
private:
    int capacity;
    unordered_map<CityKey, pair<double, int>, CityKeyHasher> values;

public:
    LFUCache(int cap) : capacity(cap) {}

    bool get(const CityKey& key, double& population) override {
        if (values.find(key) == values.end()) return false;
        values[key].second++;
        population = values[key].first;
        return true;
    }

    void put(const CityKey& key, double population) override {
        if (values.find(key) != values.end()) {
            values[key] = {population, values[key].second + 1};
            return;
        }

        if ((int)values.size() >= capacity) {
            int minFreq = INT_MAX;
            CityKey toRemove;
            for (auto& [k, v] : values) {
                if (v.second < minFreq) {
                    minFreq = v.second;
                    toRemove = k;
                }
            }
            values.erase(toRemove);
        }
        values[key] = {population, 1};
    }

    void displayCache() const override {
        cout << "LFU Cache contents:" << endl;
        for (const auto& [k, v] : values) {
            cout << k.countryCode << ", " << k.cityName << " => " << v.first << " (freq: " << v.second << ")" << endl;
        }
    }
};

class FIFOCache : public ICache {
private:
    int capacity;
    queue<CityKey> fifoQueue;
    unordered_map<CityKey, double, CityKeyHasher> cache;

public:
    FIFOCache(int cap) : capacity(cap) {}

    bool get(const CityKey& key, double& population) override {
        auto it = cache.find(key);
        if (it == cache.end()) return false;
        population = it->second;
        return true;
    }

    void put(const CityKey& key, double population) override {
        if (cache.find(key) != cache.end()) return;

        if ((int)cache.size() >= capacity) {
            CityKey old = fifoQueue.front();
            fifoQueue.pop();
            cache.erase(old);
        }

        fifoQueue.push(key);
        cache[key] = population;
    }

    void displayCache() const override {
        cout << "FIFO Cache contents:" << endl;
        for (const auto& [k, v] : cache) {
            cout << k.countryCode << ", " << k.cityName << " => " << v << endl;
        }
    }
};

class RandomCache : public ICache {
private:
    int capacity;
    unordered_map<CityKey, double, CityKeyHasher> cache;
    vector<CityKey> keys;
    default_random_engine rng;

public:
    RandomCache(int cap) : capacity(cap), rng(time(nullptr)) {}

    bool get(const CityKey& key, double& population) override {
        auto it = cache.find(key);
        if (it == cache.end()) return false;
        population = it->second;
        return true;
    }

    void put(const CityKey& key, double population) override {
        if (cache.find(key) != cache.end()) return;

        if ((int)cache.size() >= capacity) {
            uniform_int_distribution<int> dist(0, keys.size() - 1);
            int idx = dist(rng);
            CityKey toRemove = keys[idx];
            cache.erase(toRemove);
            keys.erase(keys.begin() + idx);
        }

        cache[key] = population;
        keys.push_back(key);
    }

    void displayCache() const override {
        cout << "Random Cache contents:" << endl;
        for (const auto& [k, v] : cache) {
            cout << k.countryCode << ", " << k.cityName << " => " << v << endl;
        }
    }
};

struct TrieNode {
    unordered_map<char, TrieNode*> children;
    unordered_map<string, double> countryPopMap;
    bool isEndOfCity = false;
};

class CityTrie {
private:
    TrieNode* root;

public:
    CityTrie() { root = new TrieNode(); }

    void insert(const string& city, const string& country, double population) {
        TrieNode* node = root;
        for (char c : city) {
            if (!node->children.count(c))
                node->children[c] = new TrieNode();
            node = node->children[c];
        }
        node->isEndOfCity = true;
        node->countryPopMap[country] = population;
    }

    bool search(const string& city, const string& country, double& population) {
        TrieNode* node = root;
        for (char c : city) {
            if (!node->children.count(c)) return false;
            node = node->children[c];
        }
        if (node->isEndOfCity && node->countryPopMap.count(country)) {
            population = node->countryPopMap[country];
            return true;
        }
        return false;
    }

    ~CityTrie() {
        freeNode(root);
    }

    void freeNode(TrieNode* node) {
        for (auto& [c, child] : node->children)
            freeNode(child);
        delete node;
    }
};

void loadDataIntoTrie(const string& filename, CityTrie& trie) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file" << endl;
        return;
    }

    string line;
    getline(file, line);
    while (getline(file, line)) {
        stringstream ss(line);
        string countryCode, cityName, populationStr;
        getline(ss, countryCode, ',');
        getline(ss, cityName, ',');
        getline(ss, populationStr);

        transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::tolower);
        transform(cityName.begin(), cityName.end(), cityName.begin(), ::tolower);

        double population = stod(populationStr);
        trie.insert(cityName, countryCode, population);
    }
}

int main() {
    string filename = "city_population.csv";

    cout << "Select cache strategy (lfu, fifo, random): ";
    string strategy;
    cin >> strategy;

    ICache* cache = nullptr;
    if (strategy == "lfu") cache = new LFUCache(10);
    else if (strategy == "fifo") cache = new FIFOCache(10);
    else if (strategy == "random") cache = new RandomCache(10);
    else {
        cerr << "Invalid strategy!" << endl;
        return 1;
    }

    CityTrie trie;
    loadDataIntoTrie(filename, trie);
    cout << "Trie loaded successfully.\\n";

    while (true) {
        string countryCode, cityName;
        cout << "\\nEnter country code or 'exit' to quit: ";
        cin >> countryCode;
        if (countryCode == "exit") break;
        cout << "Enter city name: ";
        cin.ignore();
        getline(cin, cityName);

        transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::tolower);
        transform(cityName.begin(), cityName.end(), cityName.begin(), ::tolower);

        CityKey key{countryCode, cityName};
        double population;

        if (cache->get(key, population)) {
            cout << "Population (from cache): " << population << endl;
        } else if (trie.search(cityName, countryCode, population)) {
            cache->put(key, population);
            cout << "Population (from trie): " << population << endl;
        } else {
            cout << "City not found." << endl;
        }

        cache->displayCache();
    }

    delete cache;
    return 0;
}
