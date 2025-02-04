#include "pin.H"
#include <mutex>
#include <algorithm>
#include <iostream>

struct MemoryOperation {
    uintptr_t address;
    size_t size;
    bool isWrite;
    uint32_t threadId;
};

static thread_local bool memstats_memory_tracing = true;

bool memstats_do_memory_tracing() {
    return memstats_memory_tracing;
}

std::vector<MemoryOperation> operations;
std::mutex opMutex;

VOID RecordMemoryRead(VOID* ip, VOID* addr, uint32_t size, uint32_t tid) {
    uintptr_t address = reinterpret_cast<uintptr_t>(addr);
    if (!memstats_do_memory_tracing())
        return;

    std::lock_guard<std::mutex> lock(opMutex);
    operations.push_back({address, size, false, tid});
}

VOID RecordMemoryWrite(VOID* ip, VOID* addr, uint32_t size, uint32_t tid) {
    uintptr_t address = reinterpret_cast<uintptr_t>(addr);
    if (!memstats_do_memory_tracing())
        return;

    std::lock_guard<std::mutex> lock(opMutex);
    operations.push_back({address, size, true, tid});
}

VOID Finalize(INT32 code, VOID* v) {
    // Print histogram of the collected memory read/writes, grouped by the memory address and seperated write/read operations
    std::sort(operations.begin(), operations.end(), [](const MemoryOperation& a, const MemoryOperation& b) {
        return a.address < b.address;
    });

    uintptr_t lastAddress = 0;
    size_t lastSize = 0;
    size_t readCount = 0;
    size_t writeCount = 0;
    for (const MemoryOperation& op : operations) {
        if (op.address != lastAddress) {
            if (lastSize > 0) {
                std::cout << "0x" << std::hex << lastAddress << std::dec << ": " << lastSize << " bytes, ";
                std::cout << readCount << " reads, " << writeCount << " writes" << std::endl;
            }
            lastAddress = op.address;
            lastSize = op.size;
            readCount = 0;
            writeCount = 0;
        }
        if (op.isWrite)
            writeCount++;
        else
            readCount++;
    }
    if (lastSize > 0) {
        std::cout << "0x" << std::hex << lastAddress << std::dec << ": " << lastSize << " bytes, ";
        std::cout << readCount << " reads, " << writeCount << " writes" << std::endl;
    }
}

INT32 Usage() {
    std::cerr << "This tool records memory read/write operations" << std::endl;
    return 1;
}

INT32 Init(int argc, char *argv[]) {
    PIN_InitSymbols();
    if (PIN_Init(argc, argv))
        return Usage();

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

const std::vector<MemoryOperation>& GetOperations() {
    return operations;
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
    return Init(argc, argv);
}