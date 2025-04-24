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
#include <set>

using namespace std;

void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

template <typename T>
T getNumericInput(const string& prompt) {
    T value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            clearInputBuffer();
            return value;
        } else {
            cout << "Invalid input. Please enter a valid number." << endl;
            clearInputBuffer();
        }
    }
}

string getStringInput(const string& prompt) {
    string input;
    cout << prompt;
    getline(cin, input);
    return input;
}



void addFilesFromList(BloomFilter& filter, vector<string>& insertedElements) {
    string filename = getStringInput("Enter file containing list of filenames: ");
    
    ifstream inFile(filename);
    if (!inFile.is_open()) {
        cout << "Error opening file: " << filename << endl;
        return;
    }
    
    string line;
    int count = 0;
    
    while (getline(inFile, line)) {
        if (!line.empty()) {
            filter.insert(line);
            insertedElements.push_back(line);
            count++;
        }
    }
    
    cout << "Added " << count << " filenames to the filter." << endl;
}

void testFalsePositiveRate(BloomFilter& filter, const vector<string>& insertedElements) {
    if (insertedElements.empty()) {
        cout << "No elements in the filter to test. Please add elements first." << endl;
        return;
    }
    
    size_t numTests = getNumericInput<size_t>("Enter number of test cases to run: ");
    
    vector<string> testStrings;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> lenDist(5, 20);
    uniform_int_distribution<> charDist(97, 122);
    
    set<string> generatedStrings(insertedElements.begin(), insertedElements.end());
    
    cout << "Generating " << numTests << " random test strings..." << endl;
    
    while (testStrings.size() < numTests) {
        int len = lenDist(gen);
        string randomStr = "test_";
        for (int i = 0; i < len; i++) {
            randomStr.push_back(static_cast<char>(charDist(gen)));
        }
        randomStr += ".txt";
        
        if (generatedStrings.find(randomStr) == generatedStrings.end()) {
            testStrings.push_back(randomStr);
            generatedStrings.insert(randomStr);
        }
    }
    
    size_t falsePositives = 0;
    for (const auto& testStr : testStrings) {
        if (filter.mightContain(testStr)) {
            falsePositives++;
        }
    }
    
    double empiricalFPR = static_cast<double>(falsePositives) / numTests;
    double theoreticalFPR = filter.getCurrentFalsePositiveRate(insertedElements.size());
    
    cout << "\n===== False Positive Rate Test Results =====" << endl;
    cout << "Elements in filter: " << insertedElements.size() << endl;
    cout << "Test cases run: " << numTests << endl;
    cout << "False positives: " << falsePositives << endl;
    cout << "Empirical false positive rate: " << fixed << setprecision(6) 
              << empiricalFPR * 100 << "%" << endl;
    cout << "Theoretical false positive rate: " << fixed << setprecision(6) 
              << theoreticalFPR * 100 << "%" << endl;
    cout << "Difference: " << fixed << setprecision(6) 
              << abs(empiricalFPR - theoreticalFPR) * 100 << "%" << endl;
}

void saveFilterToFile(const BloomFilter& filter, const vector<string>& insertedElements) {
    string filename = getStringInput("Enter filename to save filter state: ");
    
    if (!filter.saveToFile(filename)) {
        cout << "Error saving filter to file: " << filename << endl;
        return;
    }
    
    string elementListFile = filename + ".elements";
    ofstream outFile(elementListFile);
    
    if (!outFile.is_open()) {
        cout << "Warning: Could not save element list to " << elementListFile << endl;
        cout << "Filter was saved, but element list was not." << endl;
        return;
    }
    
    for (const auto& element : insertedElements) {
        outFile << element << endl;
    }
    
    cout << "Filter saved to " << filename << endl;
    cout << "Element list saved to " << elementListFile << endl;
}

bool loadFilterFromFile(BloomFilter*& filter, vector<string>& insertedElements) {
    string filename = getStringInput("Enter filename to load filter state: ");
    
    BloomFilter* loadedFilter = BloomFilter::loadFromFile(filename);
    if (!loadedFilter) {
        cout << "Error loading filter from file: " << filename << endl;
        return false;
    }
    
    string elementListFile = filename + ".elements";
    ifstream inFile(elementListFile);
    
    insertedElements.clear();
    
    if (inFile.is_open()) {
        string line;
        while (getline(inFile, line)) {
            if (!line.empty()) {
                insertedElements.push_back(line);
            }
        }
        cout << "Loaded " << insertedElements.size() << " elements from " << elementListFile << endl;
    } else {
        cout << "Warning: Could not load element list from " << elementListFile << endl;
        cout << "Filter was loaded, but you won't be able to view the list of elements." << endl;
    }
    
    delete filter;
    filter = loadedFilter;
    
    cout << "Filter loaded from " << filename << endl;
    cout << "Filter size: " << filter->getSize() << " bits" << endl;
    cout << "Hash functions: " << filter->getNumHashes() << endl;
    
    return true;
}

void benchmarkPerformance(const BloomFilter& filter) {
    size_t numOperations = getNumericInput<size_t>("Enter number of operations to benchmark (recommended: 100000): ");
    
    cout << "\nGenerating random test data..." << endl;
    vector<string> testData;
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> lenDist(5, 20);
    uniform_int_distribution<> charDist(97, 122);
    
    for (size_t i = 0; i < numOperations; i++) {
        int len = lenDist(gen);
        string randomStr = "bench_";
        for (int j = 0; j < len; j++) {
            randomStr.push_back(static_cast<char>(charDist(gen)));
        }
        randomStr += ".txt";
        testData.push_back(randomStr);
    }
    
    cout << "Starting benchmark..." << endl;
    
    auto startInsert = chrono::high_resolution_clock::now();
    
    BloomFilter testFilter(filter.getSize(), filter.getNumHashes());
    for (const auto& item : testData) {
        testFilter.insert(item);
    }
    
    auto endInsert = chrono::high_resolution_clock::now();
    chrono::duration<double> insertDuration = endInsert - startInsert;
    cout << "Insertion time: " << insertDuration.count() << " seconds" << endl;
    
    auto startLookup = chrono::high_resolution_clock::now();
    
    for (const auto& item : testData) {
        testFilter.mightContain(item);
    }
    
    auto endLookup = chrono::high_resolution_clock::now();
    chrono::duration<double> lookupDuration = endLookup - startLookup;
    cout << "Lookup time: " << lookupDuration.count() << " seconds" << endl;
}
int displayMenu() {
    cout << "\n===== Bloom Filter File Checker =====" << endl;
    cout << "1. Create new Bloom filter (optimal parameters)" << endl;
    cout << "2. Create new Bloom filter (manual parameters)" << endl;
    cout << "3. Add files to the filter" << endl;
    cout << "4. Check if a file might exist" << endl;
    cout << "5. Display filter statistics" << endl;
    cout << "6. Test false positive rate" << endl;
    cout << "7. Save filter to file" << endl;
    cout << "8. Load filter from file" << endl;
    cout << "9. Benchmark performance" << endl;
    cout << "10. Clear filter" << endl;
    cout << "11. Exit" << endl;
    cout << "Enter your choice (1-11): ";
    
    int choice;
    cin >> choice;
    clearInputBuffer();
    return choice;
}
int main() {
    vector<string> insertedElements;
    BloomFilter* filter = nullptr;
    
    while (true) {
        int choice = displayMenu();
        
        switch (choice) {
            case 1: { // Create optimal filter
                if (filter) {
                    delete filter;
                    filter = nullptr;
                }
                
                size_t expectedElements = getNumericInput<size_t>("Enter expected number of elements: ");
                double falsePositiveRate = getNumericInput<double>("Enter desired false positive rate (e.g., 0.01 for 1%): ");
                
                try {
                    filter = new BloomFilter(BloomFilter::createOptimal(expectedElements, falsePositiveRate));
                    insertedElements.clear();
                    
                    cout << "Created optimal filter with:\n"
                         << "Size: " << filter->getSize() << " bits (" 
                         << (filter->getSize() / 8 / 1024) << " KB)\n"
                         << "Hash functions: " << filter->getNumHashes() << "\n"
                         << "Theoretical FPR: " << fixed << setprecision(4) 
                         << (falsePositiveRate * 100) << "%" << endl;
                } catch (const exception& e) {
                    cerr << "Error creating filter: " << e.what() << endl;
                }
                break;
            }
            
            case 2: { // Create manual filter
                if (filter) {
                    delete filter;
                    filter = nullptr;
                }
                
                size_t filterSize = getNumericInput<size_t>("Enter filter size (in bits): ");
                size_t numHashes = getNumericInput<size_t>("Enter number of hash functions: ");
                
                filter = new BloomFilter(filterSize, numHashes);
                insertedElements.clear();
                
                cout << "Created manual filter with:\n"
                     << "Size: " << filter->getSize() << " bits\n"
                     << "Hash functions: " << filter->getNumHashes() << "\n"
                     << "Current FPR: " << fixed << setprecision(4)
                     << (filter->getCurrentFalsePositiveRate(0) * 100) << "%" << endl;
                break;
            }
            
            case 3: { // Add files
                if (!filter) {
                    cout << "No filter created yet. Please create a filter first." << endl;
                    break;
                }
                addFilesFromList(*filter, insertedElements);
                break;
            }
            
            case 4: { // Check file
                if (!filter) {
                    cout << "No filter created yet. Please create a filter first." << endl;
                    break;
                }
                string filename = getStringInput("Enter filename to check: ");
                bool mightExist = filter->mightContain(filename);
                bool actuallyExists = find(insertedElements.begin(), insertedElements.end(), filename) != insertedElements.end();
                
                cout << "Bloom filter result: ";
                if (mightExist) {
                    if (actuallyExists) {
                        cout << "File exists in filter (true positive)";
                    } else {
                        cout << "File might exist (false positive)";
                        cout << "\nCurrent false positive probability: " << fixed << setprecision(4)
                             << (filter->getCurrentFalsePositiveRate(insertedElements.size()) * 100) << "%";
                    }
                } else {
                    cout << "File definitely does not exist";
                }
                cout << endl;
                break;
            }
            
            case 5: { // Display stats
                if (!filter) {
                    cout << "No filter created yet. Please create a filter first." << endl;
                    break;
                }
                cout << "\n===== Filter Statistics =====" << endl;
                cout << "Size: " << filter->getSize() << " bits (" 
                     << (filter->getSize() / 8) << " bytes)" << endl;
                cout << "Hash functions: " << filter->getNumHashes() << endl;
                cout << "Elements inserted: " << insertedElements.size() << endl;
                cout << "Current false positive rate: " << fixed << setprecision(4)
                     << (filter->getCurrentFalsePositiveRate(insertedElements.size()) * 100) << "%" << endl;
                cout << "Filter utilization: " << fixed << setprecision(2)
                     << ((double)insertedElements.size() / (filter->getSize() / filter->getNumHashes()) * 100) << "%" << endl;
                break;
            }
            
            case 6: { // Test FPR
                if (!filter) {
                    cout << "No filter created yet. Please create a filter first." << endl;
                    break;
                }
                testFalsePositiveRate(*filter, insertedElements);
                break;
            }
            
            case 7: { // Save filter
                if (!filter) {
                    cout << "No filter created yet. Please create a filter first." << endl;
                    break;
                }
                saveFilterToFile(*filter, insertedElements);
                break;
            }
            
            case 8: { // Load filter
                if (filter) {
                    delete filter;
                    filter = nullptr;
                }
                if (!loadFilterFromFile(filter, insertedElements)) {
                    cout << "Failed to load filter." << endl;
                }
                break;
            }
            
            case 9: { // Benchmark
                if (!filter) {
                    cout << "No filter created yet. Please create a filter first." << endl;
                    break;
                }
                benchmarkPerformance(*filter);
                break;
            }
            
            case 10: { // Clear filter
                if (!filter) {
                    cout << "No filter created yet. Please create a filter first." << endl;
                    break;
                }
                filter->clear();
                insertedElements.clear();
                cout << "Filter cleared." << endl;
                break;
            }
            
            case 11: { // Exit
                if (filter) {
                    delete filter;
                }
                cout << "Exiting program..." << endl;
                return 0;
            }
            
            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    }
}
