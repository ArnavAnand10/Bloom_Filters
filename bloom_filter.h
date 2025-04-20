#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <vector>
#include <functional>
#include <string>
#include <cmath>
#include <fstream>

class BloomFilter {
private:
    std::vector<bool> bitArray;
    size_t size;
    unsigned int numHashes;
    
    // Simple hash functions for demonstration
    std::vector<std::function<size_t(const std::string&)>> hashFunctions;
    
    // Initialize hash functions
    void initializeHashFunctions();
    
    // Helper methods for creating hash functions
    size_t hashFunction1(const std::string& key) const;
    size_t hashFunction2(const std::string& key) const;
    size_t combinedHash(const std::string& key, int seed) const;

public:
    // Constructor with specified size and number of hash functions
    BloomFilter(size_t filterSize, unsigned int numHashFunctions);
    
    // Static method that calculates optimal parameters based on expected items and false positive rate
    static BloomFilter createOptimal(size_t expectedItems, double falsePositiveRate);
    
    // Insert an element into the bloom filter
    void insert(const std::string& element);
    
    // Check if an element might be in the set
    bool mightContain(const std::string& element) const;
    
    // Get current false positive probability based on items inserted
    double getCurrentFalsePositiveRate(size_t insertedItems) const;
    
    // Get size of the bit array
    size_t getSize() const;
    
    // Get number of hash functions
    unsigned int getNumHashes() const;
    
    // Reset the filter
    void clear();
    
    // Print the current state of the bit array (useful for debugging)
    void printFilter() const;
    
    // Save filter state to a file
    bool saveToFile(const std::string& filename) const;
    
    // Load filter state from a file
    static BloomFilter* loadFromFile(const std::string& filename);
};

#endif // BLOOM_FILTER_H