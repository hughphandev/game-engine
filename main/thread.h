#ifndef THREAD_H
#define THREAD_H


#define WORK_ENTRY_CALLBACK(name) void name(void* data)
typedef WORK_ENTRY_CALLBACK(work_entry_callback);


struct work_queue_entry
{
  work_entry_callback* callback;
  void* data;
};

struct platform_work_queue
{
  i32 volatile  completionGoal;
  i32 volatile completionCount;

  i32 volatile entryToRead;
  i32 volatile entryToWrite;
  void* semaphoreHandle;
  work_queue_entry entries[255];
};

#define PLATFORM_ADD_WORK_ENTRY(name) void name(platform_work_queue* queue, work_entry_callback callback, void* data)
typedef PLATFORM_ADD_WORK_ENTRY(platform_add_work_entry);
#define PLATFORM_COMPLETE_ALL_WORK(name) void name(platform_work_queue* queue)
typedef PLATFORM_COMPLETE_ALL_WORK(platform_complete_all_work);

static platform_add_work_entry* PlatformAddWorkEntry;
static platform_complete_all_work* PlatformCompleteAllWork;

#endif