#ifndef MEMSTATS_MEMORYTRACER_HH
#define MEMSTATS_MEMORYTRACER_HH

#include <vector>
#include <mutex>
#include <set>

struct MemoryOperation {
    uintptr_t address;
    size_t size;
    bool isWrite;
    uint32_t threadId;
};

class MemoryTracer {
public:
    static void Init();
    static void Finalize();
    static const std::vector<MemoryOperation>& GetOperations();

private:
    static void RecordMemoryRead(void* ip, void* addr, uint32_t size, uint32_t tid);
    static void RecordMemoryWrite(void* ip, void* addr, uint32_t size, uint32_t tid);

    static std::vector<MemoryOperation> operations;
    static std::set<std::pair<uintptr_t, uintptr_t>> excludedRanges;
    static std::mutex opMutex;
};


#endif //MEMSTATS_MEMORYTRACER_HH
