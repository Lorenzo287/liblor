#if defined(WIN32)
    #include "libmill.h"
#elif defined(__linux__)
    #include <libmill.h>
#endif
#include <stdio.h>

// use Makefile inside WSL to compile
// library is installed at ~/opt/libmill

// Producer coroutine: sends numbers 0-9 to the channel, then signals completion.
coroutine void producer(chan ch) {
    printf("Producer: Starting...\n");
    for (int i = 0; i < 10; i++) {
        // Send the integer 'i' to the channel 'ch'.
        // 'chs' stands for "channel send".
        chs(ch, int, i);
        printf("Producer: Sent %d\n", i);
        // Briefly yield to allow other coroutines to run.
        // This is good practice in cooperative multitasking.
        yield();
    }
    // Send a special value (e.g., -1) to signal the consumer that no more data will follow.
    chs(ch, int, -1);
    printf("Producer: Sent termination signal (-1)\n");
    // Close the channel, indicating no more sends will occur from this end.
    chclose(ch);
    printf("Producer: Channel closed and finished.\n");
}

// Consumer coroutine: receives numbers from the channel until a termination signal is received.
coroutine void consumer(chan ch) {
    printf("Consumer: Starting...\n");
    while (1) {
        // Receive an integer from the channel 'ch'.
        // 'chr' stands for "channel receive".
        int received_value = chr(ch, int);
        // Check for the termination signal.
        if (received_value == -1) {
            printf("Consumer: Received termination signal. Exiting.\n");
            break; // Exit the loop and terminate the coroutine
        }
        printf("Consumer: Received %d\n", received_value);
        // Briefly yield to allow other coroutines to run.
        yield();
    }
    printf("Consumer: Finished.\n");
}

int main(void) {
    // Create a buffered channel for integers with a buffer size of 100.
    // 'chmake' creates a new channel.
    chan ch = chmake(int, 100);
    printf("Main: Channel created.\n");

    // Start the producer coroutine.
    // 'go' starts a new coroutine.
    go(producer(ch));
    printf("Main: Producer coroutine started.\n");

    // Start the consumer coroutine.
    go(consumer(ch));
    printf("Main: Consumer coroutine started.\n");

    // Sleep for a short duration to allow coroutines to complete.
    // 'msleep' puts the current coroutine (main) to sleep until a deadline.
    // 'now()' returns the current time in milliseconds.
    msleep(now() + 500); // Wait for 500 milliseconds
    printf("Main: Woke up from sleep.\n");

    // At this point, the coroutines should have completed their work.
    // In more complex scenarios, you might use additional channels or
    // other synchronization mechanisms to ensure completion before main exits.

    printf("Main: Exiting.\n");
    return 0;
}
