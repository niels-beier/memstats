#include "memorytracer.hh"
#include "pin.H"
#include <mutex>
#include <algorithm>

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
    // Cleanup (optional)
}

const std::vector<MemoryOperation>& MemoryTracer::GetOperations() {
    return operations;
}

void MemoryTracer::RecordMemoryRead(void* ip, void* addr, uint32_t size, uint32_t tid) {
    uintptr_t address = reinterpret_cast<uintptr_t>(addr);
    // TODO: add pintool instrumentation guard

    std::lock_guard<std::mutex> lock(opMutex);
    operations.push_back({address, size, false, tid});
}

void MemoryTracer::RecordMemoryWrite(void* ip, void* addr, uint32_t size, uint32_t tid) {
    uintptr_t address = reinterpret_cast<uintptr_t>(addr);
    // TODO: add pintool instrumentation guard

    std::lock_guard<std::mutex> lock(opMutex);
    operations.push_back({address, size, true, tid});
}
