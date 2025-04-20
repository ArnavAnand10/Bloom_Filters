#include "bloom_filter.h"
#include <iostream>
#include <random>
#include <chrono>
#include <iomanip>

// Constructor with specified parameters
BloomFilter::BloomFilter(size_t filterSize, unsigned int numHashFunctions) 
    : size(filterSize), numHashes(numHashFunctions), bitArray(filterSize, false) {
    initializeHashFunctions();
}

// Static method to create a Bloom filter with optimal parameters
BloomFilter BloomFilter::createOptimal(size_t expectedItems, double falsePositiveRate) {
    // Calculate optimal filter size: m = -n * ln(p) / (ln(2)^2)
    size_t optimalSize = static_cast<size_t>(
        std::ceil(-1.0 * expectedItems * std::log(falsePositiveRate) / (std::log(2) * std::log(2)))
    );
    
    // Calculate optimal number of hash functions: k = (m/n) * ln(2)
    unsigned int optimalHashes = static_cast<unsigned int>(
        std::ceil((optimalSize / static_cast<double>(expectedItems)) * std::log(2))
    );
    
    // Ensure we have at least 1 hash function and reasonable size
    if (optimalHashes < 1) optimalHashes = 1;
    if (optimalSize < 8) optimalSize = 8;
    
    return BloomFilter(optimalSize, optimalHashes);
}

// Initialize hash functions
void BloomFilter::initializeHashFunctions() {
    hashFunctions.clear();
    
    for (unsigned int i = 0; i < numHashes; i++) {
        // Create hash functions using different seeds
        hashFunctions.push_back([this, i](const std::string& key) {
            return this->combinedHash(key, i);
        });
    }
}

// First hash function - simple djb2 algorithm
size_t BloomFilter::hashFunction1(const std::string& key) const {
    unsigned long hash = 5381;
    for (char c : key) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % size;
}

// Second hash function - simple sdbm algorithm
size_t BloomFilter::hashFunction2(const std::string& key) const {
    unsigned long hash = 0;
    for (char c : key) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash % size;
}

// Combined hash function using the double hashing technique
size_t BloomFilter::combinedHash(const std::string& key, int seed) const {
    size_t hash1 = hashFunction1(key);
    size_t hash2 = hashFunction2(key);
    return (hash1 + seed * hash2) % size;
}

// Insert an element into the filter
void BloomFilter::insert(const std::string& element) {
    for (const auto& hashFunc : hashFunctions) {
        size_t index = hashFunc(element);
        bitArray[index] = true;
    }
}

// Check if an element might be in the set
bool BloomFilter::mightContain(const std::string& element) const {
    for (const auto& hashFunc : hashFunctions) {
        size_t index = hashFunc(element);
        if (!bitArray[index]) {
            return false; // Definitely not in the set
        }
    }
    return true; // Might be in the set
}

// Calculate current false positive rate based on items inserted
double BloomFilter::getCurrentFalsePositiveRate(size_t insertedItems) const {
    if (insertedItems == 0) return 0.0;
    
    // p ≈ (1 - e^(-kn/m))^k
    // where k = num hash functions, n = inserted items, m = filter size
    double exponent = -1.0 * numHashes * insertedItems / size;
    double probability = std::pow(1.0 - std::exp(exponent), numHashes);
    
    return probability;
}

// Get size of the filter
size_t BloomFilter::getSize() const {
    return size;
}

// Get number of hash functions
unsigned int BloomFilter::getNumHashes() const {
    return numHashes;
}

// Clear the filter
void BloomFilter::clear() {
    for (size_t i = 0; i < size; i++) {
        bitArray[i] = false;
    }
}

// Print a visualization of the filter's bit array
void BloomFilter::printFilter() const {
    const size_t maxBitsToShow = 100;
    std::cout << "Filter state (first " << std::min(size, maxBitsToShow) << " bits):" << std::endl;
    
    for (size_t i = 0; i < std::min(size, maxBitsToShow); i++) {
        std::cout << (bitArray[i] ? "1" : "0");
        if ((i + 1) % 10 == 0) std::cout << " ";
    }
    std::cout << std::endl;
    
    // Calculate bit density
    size_t setBits = 0;
    for (bool bit : bitArray) {
        if (bit) setBits++;
    }
    
    double density = static_cast<double>(setBits) / size;
    std::cout << "Bit density: " << std::fixed << std::setprecision(4) << density * 100 << "%" << std::endl;
}

// Save filter state to a file
bool BloomFilter::saveToFile(const std::string& filename) const {
    std::ofstream outFile(filename, std::ios::binary);
    
    if (!outFile.is_open()) {
        return false;
    }
    
    // Write filter metadata
    outFile.write(reinterpret_cast<const char*>(&size), sizeof(size));
    outFile.write(reinterpret_cast<const char*>(&numHashes), sizeof(numHashes));
    
    // Write the bit array
    // We'll pack the bits into bytes for efficiency
    std::vector<unsigned char> packedBits((size + 7) / 8, 0);
    
    for (size_t i = 0; i < size; i++) {
        if (bitArray[i]) {
            packedBits[i / 8] |= (1 << (i % 8));
        }
    }
    
    outFile.write(reinterpret_cast<const char*>(packedBits.data()), packedBits.size());
    
    return true;
}

// Load filter state from a file
BloomFilter* BloomFilter::loadFromFile(const std::string& filename) {
    std::ifstream inFile(filename, std::ios::binary);
    
    if (!inFile.is_open()) {
        return nullptr;
    }
    
    // Read filter metadata
    size_t loadedSize;
    unsigned int loadedNumHashes;
    
    inFile.read(reinterpret_cast<char*>(&loadedSize), sizeof(loadedSize));
    inFile.read(reinterpret_cast<char*>(&loadedNumHashes), sizeof(loadedNumHashes));
    
    if (inFile.fail()) {
        return nullptr;
    }
    
    // Create a new filter with the loaded parameters
    BloomFilter* loadedFilter = new BloomFilter(loadedSize, loadedNumHashes);
    
    // Read the bit array
    std::vector<unsigned char> packedBits((loadedSize + 7) / 8, 0);
    inFile.read(reinterpret_cast<char*>(packedBits.data()), packedBits.size());
    
    if (inFile.fail()) {
        delete loadedFilter;
        return nullptr;
    }
    
    // Unpack the bits
    for (size_t i = 0; i < loadedSize; i++) {
        if (packedBits[i / 8] & (1 << (i % 8))) {
            loadedFilter->bitArray[i] = true;
        }
    }
    
    return loadedFilter;
}