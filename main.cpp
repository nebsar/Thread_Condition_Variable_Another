/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: Eagleye
 *
 * Created on September 11, 2018, 11:34 PM
 */

#include <mutex>
#include <iostream>
#include <thread>
#include <condition_variable>

using namespace std;

struct BoundedBuffer {
    int* buffer;
    int capacity;

    int front;
    int rear;
    int count;

    std::mutex lock;


    std::condition_variable not_full;
    std::condition_variable not_empty;

    BoundedBuffer(int capacity) : capacity(capacity), front(0), rear(0), count(0) {
        buffer = new int[capacity];
    }

    ~BoundedBuffer() {
        delete[] buffer;
    }

    /* The mutexes are managed by a std::unique_lock. 
     * It is a wrapper to manage a lock. 
     * This is necessary to be used with the condition variables. 
     * To wake up a thread that is waiting on a condition variable, the notify_one() function is used.
     * The wait function is a bit special.
     * It takes as the first argument the unique lock and a the second one a predicate.
     * The predicate must return false when the waiting must be continued
     * (it is equivalent to while(!pred()){cv.wait(l);}).
     * The rest of the example has nothing special.
     * We can use this structure to fix multiple consumers / multiple producers problem.
     * This problem is very common in concurrent programming.
     * Several threads (consumers) are waiting from data produced by another several threads (producers).
     * Here is an example with several threads using the structure:    
     */

    void deposit(int data) {
        std::unique_lock<std::mutex> locker(lock);

        not_full.wait(locker, [this]() {
            return count != capacity; });

        buffer[rear] = data;
        rear = (rear + 1) % capacity;

        cout << "from deposit: rear : " << rear << '\n';
        ++count;

        not_empty.notify_one();
    }

    int fetch() {
        std::unique_lock<std::mutex> locker(lock);

        not_empty.wait(locker, [this]() {
            return count != 0; });

        int result = buffer[front];
        front = (front + 1) % capacity;

        cout << "from fetch: front : " << front << '\n';
        --count;

        not_full.notify_one();

        return result;
    }
};

void consumer(int id, BoundedBuffer& buffer) {
    for (int i = 0; i < 50; ++i) {
        int value = buffer.fetch();
        std::cout << "Consumer " << id << " fetched " << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void producer(int id, BoundedBuffer& buffer) {
    for (int i = 0; i < 75; ++i) {
        buffer.deposit(i);
        std::cout << "Produced " << id << " produced " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/*
 * Three consumer threads and two producer threads are created and query the structure constantly.
 * An interesting thing about this example is the use of std::ref to pass the buffer by reference,
 * it is necessary to avoid a copy of the buffer.
 */

int main() {
    BoundedBuffer buffer(200);

    std::thread c1(consumer, 0, std::ref(buffer));
    std::thread c2(consumer, 1, std::ref(buffer));
    std::thread c3(consumer, 2, std::ref(buffer));
    std::thread p1(producer, 0, std::ref(buffer));
    std::thread p2(producer, 1, std::ref(buffer));

    c1.join();
    c2.join();
    c3.join();
    p1.join();
    p2.join();

    return 0;
}


