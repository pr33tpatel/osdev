#include <multitasking.h>

using namespace os;
using namespace os::common;
using namespace os::utils;


Task::Task(GlobalDescriptorTable* gdt, void entrypoint()) 
{

  cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));  // NOTE: (start of stack) + (size of stack) - (size of entrypoint)

  
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    
    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    cpustate -> error = 0;    
   
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    cpustate -> eflags= 0x202; // HACK: magicnumbers for the win i guess lmao
    cpustate -> esp = 0; // NOTE: if it multitasking stops working, the problem is most likely within this struct. comment out esp and ss and see what happens. 
    cpustate -> ss = 0;
}


Task::~Task() {

}

TaskManager::TaskManager() {
  numTasks = 0;
  currentTask = -1;
}


TaskManager::~TaskManager() {
}


bool TaskManager::AddTask(Task* task) {
  if(numTasks >= 256)
    return false;
  tasks[numTasks++] = task;
  return true;
}


CPUState* TaskManager::Schedule(CPUState* cpustate) {
  static int scheduleCount = 0;
  // PERFORMANCE: round-robin scheduling algorithm 
  if(numTasks <= 0)
    return cpustate;

  if(currentTask >= 0)
    tasks[currentTask]->cpustate = cpustate;

  if(++currentTask >= numTasks)
    currentTask %= numTasks;

  // TEST: prints the schedule count every 10 calls to the scheduling algorithm
  // scheduleCount++;
  // if (scheduleCount % 10 == 0) // print every 10th call
  // {   printf("\n");
  //   printByte(scheduleCount);
  //   printf("\n");
  // } 

  return tasks[currentTask]->cpustate;
}

