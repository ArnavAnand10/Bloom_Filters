#ifndef MAIN_H
#define MAIN_H

#include "bloom_filter.h"
#include <iostream>

void displayMenu();
void createNewFilter(BloomFilter*& filter);
void addItem(BloomFilter* filter);
void checkItem(const BloomFilter* filter);
void displayFilterStats(const BloomFilter* filter);
void testFalsePositiveRate(BloomFilter* filter);

#endif // MAIN_H