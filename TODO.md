TODO:
- [x] improve text mode:
    - [x] add down scrolling feature
    - [x] better printing functions
    - [x] implement more keyboard interrupts (full alpha-numeric, etc.)
    - [x] better fonts
    - [x] make variadic print functions to take multiple arguments 
    - [x] parse format specifiers (such as %d, %s, ...)

- [ ] CLI:
    - [ ] cursor navigation

- [ ] DracOS Network:
    - [x] ARP
    - [x] IP
    - [ ] UDP
    - [ ] TCP
    - [ ] basic html

- [ ] Processes & Threads:
    - [ ] assign userID and PID to each process created

- [ ] create an include/utils/
    - [ ] utils.h // cumulative utilties file that contains all other utilties
    - [ ] print.h
    - [ ] math.h
    - [ ] string.h
    - [ ] memory.h
    - [ ] buffer.h
    - [ ] bitmap.h
    
- [ ] create a test/
    - [ ] test_framework.h
    - [ ] test_runner.cc
    - [ ] hardware/test_{ata,keyboard,mouse,network}.cc
    - [ ] memory/test_{heap,memcpy}.cc
    - [ ] filesystem/test_{io,directory}.cc
    - [ ] system/test_{multitask,syscalls,interrupts}.cc
    - [ ] utils/test_{math,string,print}.cc
    - [ ] benchmarks/benchmarks_{disk,memory,task}.cc


- [ ] Graphics:
    - [ ] name: ... 

- [ ] change the hard drive driver design from PiO (program i/o) to DMA (direct memory access)
- [ ] make the mouse cursor a crosshair (center dot, outer circle, 4 tick marks in cardinal directions)

EXTRA:
- name maybe "DracOS" // pronounced dracos
- name the filesystem ScalesFS (for scalability ~hehe)
