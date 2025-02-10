#include "memorytracer.hh"
#include "pin.H"
#include <mutex>
#include <algorithm>
#include <array>
#include <iostream>
#include <unordered_map>

static bool memstats_memory_tracing = true;

bool memstats_do_memory_tracing() {
    return memstats_memory_tracing;
}

std::vector<MemoryOperation> MemoryTracer::operations;

int MemoryTracer::Init(int argc, char *argv[]) {

    if( PIN_Init(argc,argv) )
    {
        std::cout << "Error PIN_Init" << std::endl;
        return 1;
    }

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
    PIN_AddFiniFunction(MemoryTracer::Finalize, 0);
    PIN_StartProgram();

    return 0;
}

void MemoryTracer::Finalize(INT32 code, VOID* v) {
    static const std::array<const char *, 9> percentage_box{" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    static const int bins = 15;

    // Count the number of times each memory address is accessed seperately for reads and writes
    std::unordered_map<uintptr_t, std::pair<int, int>> access_count;
    for (const auto& op : operations) {
        if (op.isWrite) {
            access_count[op.address].second++;
        } else {
            access_count[op.address].first++;
        }
    }

    // Print a histogram of the number of times each memory address is accessed using the percentage box characters
    for (const auto& [address, count] : access_count) {
        std::cout << "0x" << std::hex << address << std::dec << ": ";
        for (int i = 0; i < bins; i++) {
            int total = count.first + count.second;
            int read_percentage = (count.first * bins) / total;
            int write_percentage = (count.second * bins) / total;
            if (i < read_percentage) {
                std::cout << percentage_box[std::min(read_percentage, 8)];
            } else if (i < write_percentage) {
                std::cout << percentage_box[std::min(write_percentage, 8)];
            } else {
                std::cout << " ";
            }
        }
        std::cout << " | " << count.first << " reads, " << count.second << " writes" << std::endl;
    }
}

const std::vector<MemoryOperation>& MemoryTracer::GetOperations() {
    return operations;
}

void MemoryTracer::RecordMemoryRead(void* ip, void* addr, uint32_t size, uint32_t tid) {
    auto address = reinterpret_cast<uintptr_t>(addr);
    if (!memstats_do_memory_tracing())
        return;

    operations.push_back({address, size, false, tid});
}

void MemoryTracer::RecordMemoryWrite(void* ip, void* addr, uint32_t size, uint32_t tid) {
    auto address = reinterpret_cast<uintptr_t>(addr);
    if (!memstats_do_memory_tracing())
        return;

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

int main(int argc, char *argv[]) {
    MemoryTracer::Init(argc, argv);
    return 0;
}