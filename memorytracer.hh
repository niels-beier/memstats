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

/** @brief Enable memory tracing for reads and writes of all memory blocks.
 * @details Thread-local. Do not call during static- or dynamic-initialization phase.
 * @return Whether memory tracing was enabled before to this call
 */
bool memstats_enable_memory_tracer();

/** @brief Disable memory tracing for reads and writes of all memory blocks.
 * @details Thread-local. Do not call during static- or dynamic-initialization phase.
 * @return Whether memory tracing was enabled before to this call
 */
bool memstats_disable_memory_tracer();

bool memstats_do_memory_tracing();

class MemoryTracerGuard {
    const bool was_memory_trace = memstats_disable_memory_tracer();

    ~MemoryTracerGuard() {
        if (was_memory_trace)
            memstats_enable_memory_tracer();
    }
};

int main(int argc, char *argv[]) {
    MemoryTracer::Init();
    MemoryTracer::Finalize();
}


#endif //MEMSTATS_MEMORYTRACER_HH
