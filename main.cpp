// main.cpp
#include "bloom_filter.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <limits>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <set> // Added this include for std::set

// Function to clear input buffer
void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Function to get integer input with validation
template <typename T>
T getNumericInput(const std::string& prompt) {
    T value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            clearInputBuffer();
            return value;
        } else {
            std::cout << "Invalid input. Please enter a valid number." << std::endl;
            clearInputBuffer();
        }
    }
}

// Function to get string input
std::string getStringInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);
    return input;
}

// Function to display menu and get choice
int displayMenu() {
    std::cout << "\n===== Bloom Filter File Checker =====" << std::endl;
    std::cout << "1. Create new Bloom filter" << std::endl;
    std::cout << "2. Add files to the filter" << std::endl;
    std::cout << "3. Check if a file might exist" << std::endl;
    std::cout << "4. Add filenames from a list" << std::endl;
    std::cout << "5. Display filter statistics" << std::endl;
    std::cout << "6. Clear filter" << std::endl;
    std::cout << "7. Test false positive rate" << std::endl;     // New option
    std::cout << "8. Save filter to file" << std::endl;          // New option
    std::cout << "9. Load filter from file" << std::endl;        // New option
    std::cout << "10. Benchmark performance" << std::endl;       // New option
    std::cout << "11. Exit" << std::endl;                        // Updated number
    std::cout << "Enter your choice (1-11): ";                   // Updated range
    
    int choice;
    std::cin >> choice;
    clearInputBuffer();
    return choice;
}

// Function to add files from a list file
void addFilesFromList(BloomFilter& filter, std::vector<std::string>& insertedElements) {
    std::string filename = getStringInput("Enter file containing list of filenames: ");
    
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Error opening file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    int count = 0;
    
    while (std::getline(inFile, line)) {
        if (!line.empty()) {
            filter.insert(line);
            insertedElements.push_back(line);
            count++;
        }
    }
    
    std::cout << "Added " << count << " filenames to the filter." << std::endl;
}

// Function to test false positive rate empirically
void testFalsePositiveRate(BloomFilter& filter, const std::vector<std::string>& insertedElements) {
    if (insertedElements.empty()) {
        std::cout << "No elements in the filter to test. Please add elements first." << std::endl;
        return;
    }
    
    size_t numTests = getNumericInput<size_t>("Enter number of test cases to run: ");
    
    // Generate test strings that weren't inserted
    std::vector<std::string> testStrings;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lenDist(5, 20);  // Random length between 5-20
    std::uniform_int_distribution<> charDist(97, 122); // ASCII 'a' to 'z'
    
    // Set to track generated strings to avoid duplicates
    std::set<std::string> generatedStrings(insertedElements.begin(), insertedElements.end());
    
    std::cout << "Generating " << numTests << " random test strings..." << std::endl;
    
    while (testStrings.size() < numTests) {
        // Generate a random string
        int len = lenDist(gen);
        std::string randomStr = "test_";
        for (int i = 0; i < len; i++) {
            randomStr.push_back(static_cast<char>(charDist(gen)));
        }
        randomStr += ".txt";
        
        // Make sure it's not already in our filter
        if (generatedStrings.find(randomStr) == generatedStrings.end()) {
            testStrings.push_back(randomStr);
            generatedStrings.insert(randomStr);
        }
    }
    
    // Count false positives
    size_t falsePositives = 0;
    for (const auto& testStr : testStrings) {
        if (filter.mightContain(testStr)) {
            falsePositives++;
        }
    }
    
    // Calculate empirical false positive rate
    double empiricalFPR = static_cast<double>(falsePositives) / numTests;
    double theoreticalFPR = filter.getCurrentFalsePositiveRate(insertedElements.size());
    
    std::cout << "\n===== False Positive Rate Test Results =====" << std::endl;
    std::cout << "Elements in filter: " << insertedElements.size() << std::endl;
    std::cout << "Test cases run: " << numTests << std::endl;
    std::cout << "False positives: " << falsePositives << std::endl;
    std::cout << "Empirical false positive rate: " << std::fixed << std::setprecision(6) 
              << empiricalFPR * 100 << "%" << std::endl;
    std::cout << "Theoretical false positive rate: " << std::fixed << std::setprecision(6) 
              << theoreticalFPR * 100 << "%" << std::endl;
    std::cout << "Difference: " << std::fixed << std::setprecision(6) 
              << std::abs(empiricalFPR - theoreticalFPR) * 100 << "%" << std::endl;
}

// Function to save filter state to a file
void saveFilterToFile(const BloomFilter& filter, const std::vector<std::string>& insertedElements) {
    std::string filename = getStringInput("Enter filename to save filter state: ");
    
    // Save the Bloom filter
    if (!filter.saveToFile(filename)) {
        std::cout << "Error saving filter to file: " << filename << std::endl;
        return;
    }
    
    // Also save the list of inserted elements for reference
    std::string elementListFile = filename + ".elements";
    std::ofstream outFile(elementListFile);
    
    if (!outFile.is_open()) {
        std::cout << "Warning: Could not save element list to " << elementListFile << std::endl;
        std::cout << "Filter was saved, but element list was not." << std::endl;
        return;
    }
    
    for (const auto& element : insertedElements) {
        outFile << element << std::endl;
    }
    
    std::cout << "Filter saved to " << filename << std::endl;
    std::cout << "Element list saved to " << elementListFile << std::endl;
}

// Function to load filter state from a file
bool loadFilterFromFile(BloomFilter*& filter, std::vector<std::string>& insertedElements) {
    std::string filename = getStringInput("Enter filename to load filter state: ");
    
    // Load the Bloom filter
    BloomFilter* loadedFilter = BloomFilter::loadFromFile(filename);
    if (!loadedFilter) {
        std::cout << "Error loading filter from file: " << filename << std::endl;
        return false;
    }
    
    // Try to load the element list
    std::string elementListFile = filename + ".elements";
    std::ifstream inFile(elementListFile);
    
    insertedElements.clear();
    
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            if (!line.empty()) {
                insertedElements.push_back(line);
            }
        }
        std::cout << "Loaded " << insertedElements.size() << " elements from " << elementListFile << std::endl;
    } else {
        std::cout << "Warning: Could not load element list from " << elementListFile << std::endl;
        std::cout << "Filter was loaded, but you won't be able to view the list of elements." << std::endl;
    }
    
    // Replace the old filter with the loaded one
    delete filter;
    filter = loadedFilter;
    
    std::cout << "Filter loaded from " << filename << std::endl;
    std::cout << "Filter size: " << filter->getSize() << " bits" << std::endl;
    std::cout << "Hash functions: " << filter->getNumHashes() << std::endl;
    
    return true;
}

// Function to benchmark filter performance
void benchmarkPerformance(const BloomFilter& filter) {
    size_t numOperations = getNumericInput<size_t>("Enter number of operations to benchmark (recommended: 100000): ");
    
    std::cout << "\nGenerating random test data..." << std::endl;
    std::vector<std::string> testData;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lenDist(5, 20);
    std::uniform_int_distribution<> charDist(97, 122);
    
    for (size_t i = 0; i < numOperations; i++) {
        int len = lenDist(gen);
        std::string randomStr = "bench_";
        for (int j = 0; j < len; j++) {
            randomStr.push_back(static_cast<char>(charDist(gen)));
        }
        randomStr += ".txt";
        testData.push_back(randomStr);
    }
    
    std::cout << "Starting benchmark..." << std::endl;
    
    // Benchmark insertion
    auto startInsert = std::chrono::high_resolution_clock::now();
    
    BloomFilter testFilter(filter.getSize(), filter.getNumHashes());
    for (const auto& item : testData) {
        testFilter.insert(item);
    }
    
    auto endInsert = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> insertDuration = endInsert - startInsert;
    
    // Benchmark lookups
    auto startLookup = std::chrono::high_resolution_clock::now();
    
    size_t positiveResults = 0;
    for (const auto& item : testData) {
        if (testFilter.mightContain(item)) {
            positiveResults++;
        }
    }
    
    auto endLookup = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> lookupDuration = endLookup - startLookup;
    
    // Display results
    std::cout << "\n===== Benchmark Results =====" << std::endl;
    std::cout << "Number of operations: " << numOperations << std::endl;
    std::cout << "Filter size: " << filter.getSize() << " bits" << std::endl;
    std::cout << "Hash functions: " << filter.getNumHashes() << std::endl;
    
    std::cout << "\nInsertion performance:" << std::endl;
    std::cout << "Total time: " << insertDuration.count() << " ms" << std::endl;
    std::cout << "Average time per insertion: " << insertDuration.count() / numOperations << " ms" << std::endl;
    std::cout << "Insertions per second: " << (numOperations * 1000.0) / insertDuration.count() << std::endl;
    
    std::cout << "\nLookup performance:" << std::endl;
    std::cout << "Total time: " << lookupDuration.count() << " ms" << std::endl;
    std::cout << "Average time per lookup: " << lookupDuration.count() / numOperations << " ms" << std::endl;
    std::cout << "Lookups per second: " << (numOperations * 1000.0) / lookupDuration.count() << std::endl;
    std::cout << "Number of positive results: " << positiveResults << " (should be " << numOperations << ")" << std::endl;
}

int main() {
    BloomFilter* filter = nullptr;
    std::vector<std::string> insertedElements;
    bool filterCreated = false;
    
    std::cout << "Welcome to the Bloom Filter File Checker!" << std::endl;
    
    while (true) {
        int choice = displayMenu();
        
        switch (choice) {
            case 1: {
                // Create new filter with optimal parameters
                size_t expectedItems = getNumericInput<size_t>("Enter expected number of files: ");
                double falsePositiveRate = getNumericInput<double>("Enter desired false positive rate (0.0-1.0): ");
                
                delete filter;
                filter = new BloomFilter(BloomFilter::createOptimal(expectedItems, falsePositiveRate));
                insertedElements.clear();
                filterCreated = true;
                
                std::cout << "Bloom filter created with optimal parameters:" << std::endl;
                std::cout << "Size: " << filter->getSize() << " bits" << std::endl;
                std::cout << "Hash functions: " << filter->getNumHashes() << std::endl;
                break;
            }
            
            case 2: {
                // Add file to the filter
                if (!filterCreated) {
                    std::cout << "Please create a filter first." << std::endl;
                    break;
                }
                
                std::string filename = getStringInput("Enter filename to add to the filter: ");
                filter->insert(filename);
                insertedElements.push_back(filename);
                std::cout << "File '" << filename << "' added to the filter." << std::endl;
                break;
            }
            
            case 3: {
                // Check if file might exist
                if (!filterCreated) {
                    std::cout << "Please create a filter first." << std::endl;
                    break;
                }
                
                std::string filename = getStringInput("Enter filename to check: ");
                
                if (filter->mightContain(filename)) {
                    std::cout << "File '" << filename << "' might exist in the set." << std::endl;
                    std::cout << "Note: This is a probabilistic result. False positives are possible." << std::endl;
                } else {
                    std::cout << "File '" << filename << "' definitely does NOT exist in the set." << std::endl;
                }
                break;
            }
            
            case 4: {
                // Add filenames from a list
                if (!filterCreated) {
                    std::cout << "Please create a filter first." << std::endl;
                    break;
                }
                
                addFilesFromList(*filter, insertedElements);
                break;
            }
            
            case 5: {
                // Display filter statistics
                if (!filterCreated) {
                    std::cout << "Please create a filter first." << std::endl;
                    break;
                }
                
                std::cout << "\n===== Bloom Filter Statistics =====" << std::endl;
                std::cout << "Size: " << filter->getSize() << " bits" << std::endl;
                std::cout << "Hash functions: " << filter->getNumHashes() << std::endl;
                std::cout << "Files added: " << insertedElements.size() << std::endl;
                
                if (!insertedElements.empty()) {
                    std::cout << "Current estimated false positive rate: " 
                              << filter->getCurrentFalsePositiveRate(insertedElements.size()) << std::endl;
                }
                
                // Print a few bits of the filter for visualization
                filter->printFilter();
                break;
            }
            
            case 6: {
                // Clear filter
                if (!filterCreated) {
                    std::cout << "Please create a filter first." << std::endl;
                    break;
                }
                
                filter->clear();
                insertedElements.clear();
                std::cout << "Filter cleared." << std::endl;
                break;
            }
            
            case 7: {
                // Test false positive rate
                if (!filterCreated) {
                    std::cout << "Please create a filter first." << std::endl;
                    break;
                }
                
                testFalsePositiveRate(*filter, insertedElements);
                break;
            }
            
            case 8: {
                // Save filter to file
                if (!filterCreated) {
                    std::cout << "Please create a filter first." << std::endl;
                    break;
                }
                
                saveFilterToFile(*filter, insertedElements);
                break;
            }
            
            case 9: {
                // Load filter from file
                if (loadFilterFromFile(filter, insertedElements)) {
                    filterCreated = true;
                }
                break;
            }
            
            case 10: {
                // Benchmark performance
                if (!filterCreated) {
                    std::cout << "Please create a filter first." << std::endl;
                    break;
                }
                
                benchmarkPerformance(*filter);
                break;
            }
            
            case 11: {
                // Exit
                std::cout << "Thank you for using the Bloom Filter File Checker. Goodbye!" << std::endl;
                delete filter;
                return 0;
            }
            
            default: {
                std::cout << "Invalid choice. Please try again." << std::endl;
                break;
            }
        }
    }
    
    return 0;
}