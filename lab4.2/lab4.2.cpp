#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <string>
using namespace std;

const int ITERATIONS = 10000000;
int v_unsafe = 0;
int v_mutex = 0;
atomic<int> v_atomic(0);
mutex mtx;

void increment_unsafe() {
    for (int i = 0; i < ITERATIONS; ++i) {
        v_unsafe = v_unsafe + 1;
    }
}

void increment_mutex() {
    for (int i = 0; i < ITERATIONS; ++i) {
        lock_guard<mutex> lock(mtx);
        v_mutex = v_mutex + 1;
    }
}

void increment_fast_correct() {
    int local_v = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        local_v++;
    }
    lock_guard<mutex> lock(mtx);
    v_mutex += local_v;
}

int v_sync = 0;
mutex sync_mtx;
condition_variable cv;
int arrived = 0;
int generation = 0;
const int NUM_SYNC_THREADS = 2;

void increment_lockstep() {
    for (int i = 0; i < 1000; ++i) {
        unique_lock<mutex> lock(sync_mtx);
        int gen = generation;
        if (++arrived == NUM_SYNC_THREADS) {
            v_sync += 1;
            arrived = 0;
            ++generation;
            cv.notify_all();
        } else {
            cv.wait(lock, [&] { return gen != generation; });
        }
    }
}

template <typename Func>
void run_and_measure(string label, Func thread_func, int expected_val, int& result_var) {
    auto start = chrono::high_resolution_clock::now();
    thread t1(thread_func);
    thread t2(thread_func);
    t1.join();
    t2.join();
    auto end = chrono::high_resolution_clock::now();
    cout << label << ":" << endl;
    cout << "  Result: " << result_var << " (Expected: " << expected_val << ")" << endl;
    cout << "  Time  : " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms\n" << endl;
}

int main() {
    cout << "Task 2: Shared Variable Access \n" << endl;

    run_and_measure("1. Unsafe (Race Condition)", increment_unsafe, ITERATIONS * 2, v_unsafe);

    v_mutex = 0;
    run_and_measure("2. Mutex (Safe but slow)", increment_mutex, ITERATIONS * 2, v_mutex);

    v_mutex = 0;
    run_and_measure("3. Fast & Correct (Local variables reduction)", increment_fast_correct, ITERATIONS * 2, v_mutex);

    auto start_sync = chrono::high_resolution_clock::now();
    thread t3(increment_lockstep);
    thread t4(increment_lockstep);
    t3.join();
    t4.join();
    auto end_sync = chrono::high_resolution_clock::now();
    cout << "4. Lockstep Sync (Both increment but reach 1000):" << endl;
    cout << "  Result: " << v_sync << " (Expected: 1000)" << endl;
    cout << "  Time  : " << chrono::duration_cast<chrono::milliseconds>(end_sync - start_sync).count() << " ms" << endl;
    return 0;
}
