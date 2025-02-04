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
    static void Finalize(INT32 code, VOID *v);
    static const std::vector<MemoryOperation>& GetOperations();

private:
    static void RecordMemoryRead(void* ip, void* addr, uint32_t size, uint32_t tid);
    static void RecordMemoryWrite(void* ip, void* addr, uint32_t size, uint32_t tid);

    static std::vector<MemoryOperation> operations;
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

public:
    ~MemoryTracerGuard() {
        if (was_memory_trace)
            memstats_enable_memory_tracer();
    }
};


#endif //MEMSTATS_MEMORYTRACER_HH
