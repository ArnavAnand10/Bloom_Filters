    #include "bloom_filter.h"
    #include <iostream>
    #include <random>
    #include <chrono>
    #include <iomanip>

    using namespace std;

    BloomFilter::BloomFilter(size_t filterSize, unsigned int numHashFunctions) 
        : size(filterSize), numHashes(numHashFunctions), bitArray(filterSize, false) {
        initializeHashFunctions();
    }

    BloomFilter BloomFilter::createOptimal(size_t expectedItems, double falsePositiveRate) {
        size_t optimalSize = static_cast<size_t>(
            ceil(-1.0 * expectedItems * log(falsePositiveRate) / (log(2) * log(2)))
        );
        unsigned int optimalHashes = static_cast<unsigned int>(
            ceil((optimalSize / static_cast<double>(expectedItems)) * log(2))
        );
        if (optimalHashes < 1) optimalHashes = 1;
        if (optimalSize < 8) optimalSize = 8;
        
        return BloomFilter(optimalSize, optimalHashes);
    }

    void BloomFilter::initializeHashFunctions() {
        hashFunctions.clear();
        for (unsigned int i = 0; i < numHashes; i++) {
            hashFunctions.push_back([this, i](const string& key) {
                return this->combinedHash(key, i);
            });
        }
    }
    
    // djb2 hashing algo
    size_t BloomFilter::hashFunction1(const string& key) const {
        unsigned long hash = 5381;
        for (char c : key) {
            hash = ((hash << 5) + hash) + c;
        }
        return hash % size;
    }
    // sdbm hashing algo
    size_t BloomFilter::hashFunction2(const string& key) const {
        unsigned long hash = 0;
        for (char c : key) {
            hash = c + (hash << 6) + (hash << 16) - hash;
        }
        return hash % size;
    }

    size_t BloomFilter::combinedHash(const string& key, int seed) const {
        size_t hash1 = hashFunction1(key);
        size_t hash2 = hashFunction2(key);
        return (hash1 + seed * hash2) % size;
    }


    void BloomFilter::insert(const string& element) {
        for (const auto& hashFunc : hashFunctions) {
            size_t index = hashFunc(element);
            bitArray[index] = true;
        }
    }

    bool BloomFilter::mightContain(const string& element) const {
        for (const auto& hashFunc : hashFunctions) {
            size_t index = hashFunc(element);
            if (!bitArray[index]) return false;
        }
        return true;
    }

    double BloomFilter::getCurrentFalsePositiveRate(size_t insertedItems) const {
        if (insertedItems == 0) return 0.0;
        double exponent = -1.0 * numHashes * insertedItems / size;
        double probability = pow(1.0 - exp(exponent), numHashes);
        return probability;
    }

    size_t BloomFilter::getSize() const {
        return size;
    }

    unsigned int BloomFilter::getNumHashes() const {
        return numHashes;
    }

    void BloomFilter::clear() {
        fill(bitArray.begin(), bitArray.end(), false);
    }

    

    bool BloomFilter::saveToFile(const string& filename) const {
        ofstream outFile(filename, ios::binary);
        
        if (!outFile.is_open()) {
            return false;
        }
        
        outFile.write(reinterpret_cast<const char*>(&size), sizeof(size));
        outFile.write(reinterpret_cast<const char*>(&numHashes), sizeof(numHashes));
        
        vector<unsigned char> packedBits((size + 7) / 8, 0);
        
        for (size_t i = 0; i < size; i++) {
            if (bitArray[i]) {
                packedBits[i / 8] |= (1 << (i % 8));
            }
        }
        
        outFile.write(reinterpret_cast<const char*>(packedBits.data()), packedBits.size());
        
        return true;
    }

    BloomFilter* BloomFilter::loadFromFile(const string& filename) {
        ifstream inFile(filename, ios::binary);
        
        if (!inFile.is_open()) {
            return nullptr;
        }
        
        size_t loadedSize;
        unsigned int loadedNumHashes;
        
        inFile.read(reinterpret_cast<char*>(&loadedSize), sizeof(loadedSize));
        inFile.read(reinterpret_cast<char*>(&loadedNumHashes), sizeof(loadedNumHashes));
        
        if (inFile.fail()) {
            return nullptr;
        }
        
        BloomFilter* loadedFilter = new BloomFilter(loadedSize, loadedNumHashes);
        
        vector<unsigned char> packedBits((loadedSize + 7) / 8, 0);
        inFile.read(reinterpret_cast<char*>(packedBits.data()), packedBits.size());
        
        if (inFile.fail()) {
            delete loadedFilter;
            return nullptr;
        }
        
        for (size_t i = 0; i < loadedSize; i++) {
            if (packedBits[i / 8] & (1 << (i % 8))) {
                loadedFilter->bitArray[i] = true;
            }
        }
        
        return loadedFilter;
    }
