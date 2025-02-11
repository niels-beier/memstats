#include "memorytracer.hh"
#include "pin.H"
#include <algorithm>
#include <array>
#include <iostream>
#include <unordered_map>

static bool memstats_memory_tracing = true;

bool memstats_do_memory_tracing() {
    return memstats_memory_tracing;
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

std::vector<MemoryOperation> MemoryTracer::operations;

VOID Image(IMG img, VOID *v) {
    RTN disableRtn = RTN_FindByName(img, "disable_memory_tracer");
    if (RTN_Valid(disableRtn)) {
        RTN_Open(disableRtn);
        RTN_InsertCall(disableRtn, IPOINT_BEFORE, AFUNPTR(memstats_disable_memory_tracer), IARG_END);
        RTN_Close(disableRtn);
    }

    RTN enableRtn = RTN_FindByName(img, "enable_memory_tracer");
    if (RTN_Valid(enableRtn)) {
        RTN_Open(enableRtn);
        RTN_InsertCall(enableRtn, IPOINT_BEFORE, AFUNPTR(memstats_enable_memory_tracer), IARG_END);
        RTN_Close(enableRtn);
    }
}

int MemoryTracer::Init(int argc, char *argv[]) {

    if( PIN_Init(argc,argv) )
    {
        std::cout << "Error PIN_Init" << std::endl;
        return 1;
    }

    PIN_InitSymbols();
    IMG_AddInstrumentFunction(Image, 0);
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
    PIN_AddFiniFunction(Finalize, 0);
    PIN_StartProgram();

    return 0;
}

void MemoryTracer::Finalize(INT32 code, VOID* v) {
    // print number of read/write operations and their total size per address
    std::unordered_map<uintptr_t, std::array<size_t, 2>> stats;
    std::unordered_map<uintptr_t, std::array<size_t, 2>> stats_size;
    for (const auto& op : operations) {
        stats[op.address][op.isWrite]++;
        stats_size[op.address][op.isWrite] += op.size;
    }
    // sort stats and stats_size by address
    std::vector<uintptr_t> addresses;
    for (const auto& [address, _] : stats) {
        addresses.push_back(address);
    }
    std::sort(addresses.begin(), addresses.end());

    for (const auto& address : addresses) {
        const auto& counts = stats[address];

        // print address, number of reads, number of writes, total size of reads, total size of writes
        std::cout << std::hex << address << std::dec << ": " << counts[0] << " reads, " << counts[1] << " writes, "
                  << stats_size[address][0] << " bytes read, " << stats_size[address][1] << " bytes written" << std::endl;
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

int main(int argc, char *argv[]) {
    MemoryTracer::Init(argc, argv);
    return 0;
}