volatile void * do_not_optimize;
volatile int do_not_optimize2;

int main() {
    // Allocate array of arrays
    int** ptr = new int*[5];
    do_not_optimize = ptr;
    for (int i = 0; i < 5; i++) {
        ptr[i] = new int[20];
        do_not_optimize = ptr[i];
    }

    ptr[1][1] = 42;

    do_not_optimize2 = ptr[1][1];

    return 0;
}