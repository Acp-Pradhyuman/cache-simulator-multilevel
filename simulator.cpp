#include <iostream>
#include <vector>
#include <climits>
#include <queue>
#include <unordered_map>
#include <functional> // Include for std::function

class CacheBlock {
public:
    bool valid;
    bool dirty;
    int tag;
    int lastAccessTime;
    std::vector<long long> data; // 64-bit words

    CacheBlock(int blockSize = 16) { // Default constructor with default block size
        valid = false;
        dirty = false;
        tag = -1;
        lastAccessTime = 0;
        data.resize(blockSize);
    }
};

class Cache {
protected:
    std::vector<CacheBlock> cache;
    int numBlocks;
    int blockSize;
    int currentTime;
    int cacheMisses;
    int readMisses;
    int writeMisses;
    int cacheSearches;

public:
    Cache(int numBlocks, int blockSize) : numBlocks(numBlocks), blockSize(blockSize), currentTime(0),
        cacheMisses(0), readMisses(0), writeMisses(0), cacheSearches(0) {
        cache.resize(numBlocks, CacheBlock(blockSize));
    }

    virtual bool access(int memoryAddress, bool write) = 0; // Pure virtual function

    int getMisses() const {
        return cacheMisses;
    }

    int getSearches() const {
        return cacheSearches;
    }

    int getBlockSize() const {
        return blockSize;
    }

    void printStats(const std::string& cacheName) const {
        std::cout << cacheName << " Cache Stats:" << std::endl;
        std::cout << "Cache Misses: " << cacheMisses << std::endl;
        std::cout << "Cache Searches: " << cacheSearches << std::endl;
        std::cout << "Cache Hit Rate: " << (1.0 - (double)cacheMisses / cacheSearches) * 100 << "%" << std::endl;
        std::cout << "Read Misses: " << readMisses << std::endl;
        std::cout << "Write Misses: " << writeMisses << std::endl;
    }
};

class DirectMappedCache : public Cache {
public:
    std::function<void(const CacheBlock&)> onEvict; // Callback for eviction

    DirectMappedCache(int numBlocks, int blockSize) : Cache(numBlocks, blockSize) {}

    bool access(int memoryAddress, bool write) override {
        currentTime++;
        cacheSearches++;

        int blockOffsetBits = 4; // Block size is 16 words (64 bytes), so 4 bits for offset
        int index = (memoryAddress >> blockOffsetBits) % numBlocks;
        int tag = memoryAddress >> blockOffsetBits;

        if (cache[index].valid && cache[index].tag == tag) {
            // Cache hit
            cache[index].lastAccessTime = currentTime;
            if (write) {
                cache[index].dirty = true;
            }
            return true; // Hit
        } else {
            // Cache miss
            cacheMisses++;
            if (write) {
                writeMisses++;
            } else {
                readMisses++;
            }

            // Evict the current block (if valid, notify TwoLevelCache to add to victim cache)
            if (cache[index].valid && onEvict) {
                onEvict(cache[index]); // Trigger the eviction callback
            }

            // Replace the block
            cache[index].valid = true;
            cache[index].tag = tag;
            cache[index].lastAccessTime = currentTime;
            if (write) {
                cache[index].dirty = true;
            } else {
                cache[index].dirty = false;
            }
            return false; // Miss
        }
    }
};

class SetAssociativeCache : public Cache {
private:
    int ways;
    std::vector<std::vector<CacheBlock>> sets;
    std::vector<std::unordered_map<int, int>> tagToIndex; // Maps tag to index in the set
    std::unordered_map<int, int> accessFrequency; // Tracks access frequency for prefetching

public:
    SetAssociativeCache(int numBlocks, int blockSize, int ways) : Cache(numBlocks, blockSize), ways(ways) {
        int numSets = numBlocks / ways;
        sets.resize(numSets, std::vector<CacheBlock>(ways, CacheBlock(blockSize)));
        tagToIndex.resize(numSets);
    }

    bool access(int memoryAddress, bool write) override {
        currentTime++;
        cacheSearches++;

        int blockOffsetBits = 4; // Block size is 16 words (64 bytes), so 4 bits for offset
        int setIndex = (memoryAddress >> blockOffsetBits) % sets.size();
        int tag = memoryAddress >> blockOffsetBits;

        if (tagToIndex[setIndex].find(tag) != tagToIndex[setIndex].end()) {
            // Cache hit
            int blockIndex = tagToIndex[setIndex][tag];
            sets[setIndex][blockIndex].lastAccessTime = currentTime;
            if (write) {
                sets[setIndex][blockIndex].dirty = true;
            }
            return true; // Hit
        } else {
            // Cache miss
            cacheMisses++;
            if (write) {
                writeMisses++;
            } else {
                readMisses++;
            }

            // Prefetch the next block
            int nextAddress = memoryAddress + (1 << blockOffsetBits);
            prefetch(nextAddress);

            // Find the LRU block in the set
            int lruIndex = 0;
            int minTime = INT_MAX;
            for (int i = 0; i < ways; ++i) {
                if (!sets[setIndex][i].valid || sets[setIndex][i].lastAccessTime < minTime) {
                    lruIndex = i;
                    minTime = sets[setIndex][i].lastAccessTime;
                }
            }

            // Replace the LRU block
            if (sets[setIndex][lruIndex].valid && sets[setIndex][lruIndex].dirty) {
                // Write back to memory if dirty
            }
            sets[setIndex][lruIndex].valid = true;
            sets[setIndex][lruIndex].tag = tag;
            sets[setIndex][lruIndex].lastAccessTime = currentTime;
            if (write) {
                sets[setIndex][lruIndex].dirty = true;
            } else {
                sets[setIndex][lruIndex].dirty = false;
            }
            tagToIndex[setIndex][tag] = lruIndex;
            return false; // Miss
        }
    }

    void prefetch(int memoryAddress) {
        int blockOffsetBits = 4; // Block size is 16 words (64 bytes), so 4 bits for offset
        int setIndex = (memoryAddress >> blockOffsetBits) % sets.size();
        int tag = memoryAddress >> blockOffsetBits;

        if (tagToIndex[setIndex].find(tag) == tagToIndex[setIndex].end()) {
            // Prefetch the block into the cache
            int lruIndex = 0;
            int minTime = INT_MAX;
            for (int i = 0; i < ways; ++i) {
                if (!sets[setIndex][i].valid || sets[setIndex][i].lastAccessTime < minTime) {
                    lruIndex = i;
                    minTime = sets[setIndex][i].lastAccessTime;
                }
            }

            // Replace the LRU block
            if (sets[setIndex][lruIndex].valid && sets[setIndex][lruIndex].dirty) {
                // Write back to memory if dirty
            }
            sets[setIndex][lruIndex].valid = true;
            sets[setIndex][lruIndex].tag = tag;
            sets[setIndex][lruIndex].lastAccessTime = currentTime;
            sets[setIndex][lruIndex].dirty = false;
            tagToIndex[setIndex][tag] = lruIndex;
        }
    }
};

class TwoLevelCache {
private:
    DirectMappedCache l1Cache;
    SetAssociativeCache l2Cache;
    std::queue<CacheBlock> writeBuffer;
    std::vector<CacheBlock> victimCache;
    std::vector<CacheBlock> prefetchCache;
    std::unordered_map<int, int> accessFrequency; // Tracks access frequency for prefetching

    int unifiedHits;
    int unifiedMisses;

    void addToVictimCache(const CacheBlock& block) {
        if (victimCache.size() >= 4) {
            // Evict the oldest block
            victimCache.erase(victimCache.begin());
        }
        victimCache.push_back(block);
    }

    void addToWriteBuffer(const CacheBlock& block) {
        if (writeBuffer.size() >= 4) {
            // Write back the oldest block to memory
            writeBuffer.pop();
        }
        writeBuffer.push(block);
    }

    void addToPrefetchCache(const CacheBlock& block) {
        if (prefetchCache.size() >= 4) {
            // Evict the least recently used block
            prefetchCache.erase(prefetchCache.begin());
        }
        prefetchCache.push_back(block);
    }

public:
    TwoLevelCache(int l1NumBlocks, int l1BlockSize, int l2NumBlocks, int l2BlockSize, int l2Ways)
        : l1Cache(l1NumBlocks, l1BlockSize), l2Cache(l2NumBlocks, l2BlockSize, l2Ways),
          unifiedHits(0), unifiedMisses(0) {
        victimCache.resize(0);
        prefetchCache.resize(0);

        // Set up the eviction callback for L1 cache
        l1Cache.onEvict = [this](const CacheBlock& block) {
            addToVictimCache(block);
        };
    }

    void access(int memoryAddress, bool write) {
        bool isUnifiedHit = false;

        // Check L1 cache
        if (l1Cache.access(memoryAddress, write)) {
            isUnifiedHit = true;
        } else {
            // Check victim cache
            for (auto& block : victimCache) {
                if (block.valid && block.tag == (memoryAddress >> 4)) {
                    isUnifiedHit = true;
                    break;
                }
            }

            if (!isUnifiedHit) {
                // Check write buffer
                std::queue<CacheBlock> tempQueue = writeBuffer;
                while (!tempQueue.empty()) {
                    if (tempQueue.front().valid && tempQueue.front().tag == (memoryAddress >> 4)) {
                        isUnifiedHit = true;
                        break;
                    }
                    tempQueue.pop();
                }

                if (!isUnifiedHit) {
                    // Check prefetch cache
                    for (auto& block : prefetchCache) {
                        if (block.valid && block.tag == (memoryAddress >> 4)) {
                            isUnifiedHit = true;
                            break;
                        }
                    }

                    if (!isUnifiedHit) {
                        // Check L2 cache
                        if (l2Cache.access(memoryAddress, write)) {
                            isUnifiedHit = true;
                        }
                    }
                }
            }
        }

        // Update access frequency for prefetching
        accessFrequency[memoryAddress >> 4]++;
        if (accessFrequency[memoryAddress >> 4] >= 2) {
            // Add to prefetch cache if accessed 2 or more times
            CacheBlock block(l1Cache.getBlockSize());
            block.valid = true;
            block.tag = memoryAddress >> 4;
            addToPrefetchCache(block);
        }

        // Handle write misses
        if (!isUnifiedHit && write) {
            CacheBlock block(l1Cache.getBlockSize());
            block.valid = true;
            block.tag = memoryAddress >> 4;
            block.dirty = true;
            addToWriteBuffer(block);
        }

        if (isUnifiedHit) {
            unifiedHits++;
        } else {
            unifiedMisses++;
        }
    }

    void printStats() const {
        // std::cout << "L1 Cache Stats:" << std::endl;
        l1Cache.printStats("L1");

        // std::cout << "L2 Cache Stats:" << std::endl;
        l2Cache.printStats("L2");

        std::cout << "Overall Unified Cache Stats:" << std::endl;
        std::cout << "Unified Hits: " << unifiedHits << std::endl;
        std::cout << "Unified Misses: " << unifiedMisses << std::endl;
        std::cout << "Unified Hit Rate: " << (double)unifiedHits / (unifiedHits + unifiedMisses) * 100 << "%" << std::endl;
    }
};

int main() {
    int l1NumBlocks = 128; // 2K words / 16 words per block
    int l1BlockSize = 16;
    int l2NumBlocks = 1024; // 16K words / 16 words per block
    int l2BlockSize = 16;
    int l2Ways = 8; // Increased from 4-way to 8-way

    TwoLevelCache cache(l1NumBlocks, l1BlockSize, l2NumBlocks, l2BlockSize, l2Ways);

        // std::cout<<std::endl<<"Before : "<<std::endl;
    // cache.printCacheState();
    // Simulating spatial access patterns (read and write)
    std::cout << "Simulating Spatial Access - Read:" << std::endl;
    for (int i = 0; i < 1000; i++) {
        cache.access(i, false);  // Read access
    }
    // std::cout<<std::endl<<"After : "<<std::endl;
    // cache.printCacheState();

    // std::cout<<std::endl<<"Before : "<<std::endl;
    // cache.printCacheState();
    cache.printStats();
    // std::cout<<std::endl<<"After : "<<std::endl;
    // cache.printCacheState();

    std::cout << "Simulating Spatial Access - Write:" << std::endl;
    for (int i = 0; i < 2000; i++) {
        cache.access(i, true);  // Write access
    }

    // std::cout<<std::endl<<"Before : "<<std::endl;
    // cache.printCacheState();
    cache.printStats();
    // std::cout<<std::endl<<"After : "<<std::endl;
    // cache.printCacheState();

    std::cout << "Simulating Temporal Access - Read:" << std::endl;
    // Access a subset of memory addresses multiple times to simulate temporal locality
    for (int i = 0; i < 1000; i++) {
        cache.access(i, false);  // Read access to addresses 0 to 999
    }

    for (int i = 0; i < 1000; i++) {
        cache.access(i, false);  // Read access again to the same addresses
    }

    for (int i = 1000; i < 2000; i++) {
        cache.access(i, false);  // Read access to addresses 1000 to 1999
    }

    for (int i = 1000; i < 2000; i++) {
        cache.access(i, false);  // Read access again to the same addresses
    }

    // std::cout<<std::endl<<"Before : "<<std::endl;
    // cache.printCacheState();
    cache.printStats();
    // std::cout<<std::endl<<"After : "<<std::endl;
    // cache.printCacheState();

    std::cout << "Simulating Temporal Access - Write:" << std::endl;
    for (int i = 0; i < 4000; i++) {
        cache.access(i, true);  // Write access
    }

    for (int i = 0; i < 4000; i++) {
        cache.access(i, true);  // Write access again to the same addresses
    }

    for (int i = 1000; i < 2000; i++) {
        cache.access(i, true);  // Write access to addresses 1000 to 1999
    }

    for (int i = 1000; i < 2000; i++) {
        cache.access(i, true);  // Write access again to the same addresses
    }

    // std::cout<<std::endl<<"Before : "<<std::endl;
    // cache.printCacheState();
    cache.printStats();
    // std::cout<<std::endl<<"After : "<<std::endl;
    // cache.printCacheState();

    std::cout << "Simulating Mixed Access - Read:" << std::endl;
    for (int i = 0; i < 100; i++) {
        cache.access(i, false);  // Read access
    }

    for (int i = 500; i < 3000; i++) {
        cache.access(i, false);  // Read access
    }

    for (int i = 500; i < 3000; i++) {
        cache.access(i, false);  // Read access (temporal)
    }

    // std::cout<<std::endl<<"Before : "<<std::endl;
    // cache.printCacheState();
    cache.printStats();
    // std::cout<<std::endl<<"After : "<<std::endl;
    // cache.printCacheState();

    std::cout << "Simulating Mixed Access - Write:" << std::endl;
    for (int i = 0; i < 1000; i++) {
        cache.access(i, true);  // Write access
    }

    for (int i = 0; i < 1000; i++) {
        cache.access(i, true);  // Write access (temporal)
    }

    for (int i = 2000; i < 6000; i++) {
        cache.access(i, true);  // Write access
    }

    cache.printStats();

    std::cout << "Simulating Mixed Access - Read & Write:" << std::endl;
    // assumption changing array contents from arr[i] to x*arr[i]
    // and "x" here can be a constant or a variable attained from
    // previous read and write operations (above)
    // Hence first we need to read then write
    for (int i = 0; i < 1000; i++) {
        cache.access(i, false); // Read access
        cache.access(i, true);  // Write access
    }

    for (int i = 0; i < 1000; i++) {
        cache.access(i, false); // Read access (temporal)
        cache.access(i, true);  // Write access (temporal)
    }

    for (int i = 2000; i < 6000; i++) {
        cache.access(i, false); // Read access
        cache.access(i, true);  // Write access
    }

    // std::cout<<std::endl<<"Before : "<<std::endl;
    // cache.printCacheState();
    cache.printStats();
    // std::cout<<std::endl<<"After : "<<std::endl;
    // cache.printCacheState();

    return 0;
}