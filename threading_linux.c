#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/sysinfo.h>

#define ENFORCE_READ_WRITE_ORDERING asm volatile("" ::: "memory")
#define ATOMIC_ADD(Value, Numeric) __sync_fetch_and_add(&Value, Numeric)

#define ARRAY_LENGTH(Array) (sizeof(Array) / sizeof(Array[0]))

typedef struct {
   char* String;
} entry;

static sem_t    Semaphore;
static uint32_t volatile EntryCompletionCount;
static uint32_t volatile NextEntryToWork;
static uint32_t volatile EntryCount;
static entry Entries[256];

void PushString(char* String) {
   assert(EntryCount < ARRAY_LENGTH(Entries));
   
   entry* Entry = Entries + EntryCount;
   Entry->String = String;

   ENFORCE_READ_WRITE_ORDERING;
   EntryCount++;
   sem_post(&Semaphore);
}

void* ThreadWork(void* args) {
   int ThreadId = pthread_self();

   for(;;) {
      if(NextEntryToWork < EntryCount) {
         int EntryIndex = ATOMIC_ADD(NextEntryToWork, 1);
         ENFORCE_READ_WRITE_ORDERING;

         entry* Entry = Entries + EntryIndex;

         printf("%s, Thread:%d\n", Entry->String, ThreadId);

         ATOMIC_ADD(EntryCompletionCount, 1);
      } else {
         sem_wait(&Semaphore);
      }
   }
}

static int ProcessorCount;
static int ProcessorAvailableCount;

int main() {
   ProcessorCount = get_nprocs_conf();
   ProcessorAvailableCount = get_nprocs();
   printf("Processor Count           | %d\n", ProcessorCount);
   printf("Processor Available Count | %d\n", ProcessorAvailableCount);

   sem_init(&Semaphore, 0, 1);

   pthread_t Threads[ProcessorCount];
   for(short i = 0; i < ProcessorCount; ++i) {
      int ThreadResult = pthread_create(&Threads[i], NULL, &ThreadWork, NULL);
      switch(ThreadResult) {
         case 0:
            printf("Thread %d Created\n", i);
         break;
         default:
            printf("Error%d\n", ThreadResult);
         break;
      }     
   }
   PushString("String A0");
   PushString("String A1");
   PushString("String A2");
   PushString("String A3");
   PushString("String A4");
   PushString("String A5");
   PushString("String A6");
   PushString("String A7");
   PushString("String A8");

   sleep(1);

   PushString("String B0");
   PushString("String B1");
   PushString("String B2");
   PushString("String B3");
   PushString("String B4");
   PushString("String B5");
   PushString("String B6");
   PushString("String B7");
   PushString("String B8");

   while(EntryCompletionCount != EntryCount);
}
