#include "memorytracer.hh"
#include "pin.H"
#include <mutex>
#include <algorithm>
#include <cstring>
#include <iostream>

bool init_memstats_memory_tracing() {
    if (char *ptr = std::getenv("MEMSTATS_MEMORY_TRACING")) {
        if (std::strcmp(ptr, "true") == 0 or std::strcmp(ptr, "1") == 0)
            return true;
        if (std::strcmp(ptr, "false") == 0 or std::strcmp(ptr, "0") == 0)
            return false;
        std::cerr << "Option 'MEMSTATS_MEMORY_TRACING=" << ptr
                  << "' not known. Fallback on default 'false'\n";
    }
    return false;
}

static thread_local bool memstats_memory_tracing = init_memstats_memory_tracing();

bool memstats_do_memory_tracing() {
    return memstats_memory_tracing;
}

std::vector<MemoryOperation> MemoryTracer::operations;
std::set<std::pair<uintptr_t, uintptr_t>> MemoryTracer::excludedRanges;
std::mutex opMutex;

void MemoryTracer::Init() {
    PIN_InitSymbols();
    INS_AddInstrumentFunction([](INS ins, VOID* v) {
        if (INS_IsMemoryRead(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemoryRead,
                           IARG_INST_PTR, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_THREAD_ID, IARG_END);
        }
        if (INS_IsMemoryWrite(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemoryWrite,
                           IARG_INST_PTR, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_THREAD_ID, IARG_END);
        }
    }, nullptr);
    PIN_StartProgram();
}

void MemoryTracer::Finalize() {
    // TODO: Implement report generation

}

const std::vector<MemoryOperation>& MemoryTracer::GetOperations() {
    return operations;
}

void MemoryTracer::RecordMemoryRead(void* ip, void* addr, uint32_t size, uint32_t tid) {
    uintptr_t address = reinterpret_cast<uintptr_t>(addr);
    if (!memstats_do_memory_tracing())
        return;

    std::lock_guard<std::mutex> lock(opMutex);
    operations.push_back({address, size, false, tid});
}

void MemoryTracer::RecordMemoryWrite(void* ip, void* addr, uint32_t size, uint32_t tid) {
    uintptr_t address = reinterpret_cast<uintptr_t>(addr);
    if (!memstats_do_memory_tracing())
        return;

    std::lock_guard<std::mutex> lock(opMutex);
    operations.push_back({address, size, true, tid});
}

template<class T, class U = T>
T exchange(T &obj, U &&new_value) {
    T old_value = std::move(obj);
    obj = std::forward<U>(new_value);
    return old_value;
}

bool memstats_enable_memory_tracer() {
    return exchange(memstats_memory_tracing, true);
}

bool memstats_disable_memory_tracer() {
    return exchange(memstats_memory_tracing, false);
}