todo:
- [x] improve text mode:
    - [x] add down scrolling feature
    - [x] better printing functions
    - [x] implement more keyboard interrupts (full alpha-numeric, etc.)
    - [x] better fonts
    - [x] make variadic print functions to take multiple arguments 
    - [x] parse format specifiers (such as %d, %s, ...)
    - [x] add width and better formatting to specifiers

- [ ] cli:
    - [x] cursor navigation
    - [x] Command Struct
        - [x] update Command Struct to use hashmap for mapping name to Command* objects
    - [x] Command Registry
        - [x] update Command Registry to use hashmap for mapping name to dependency objects 
    - [ ] edit command buffer and shift data
    - [ ] command history
    - [ ] highlighting
    - [ ] copy and paste
         
    - [ ] Optional:
        - [ ] VIM keybinds?

- [ ] CIU: 
    - [x] add <subsystem, color> map
    - [x] iterate over and print metadata available
    - [ ] add separate Central Intellgience Terminal

- [ ] CommandList: 
    - [ ] System:
        - [x] echo
        - [x] clear
        - [ ] stats: display system information and metrics
        - [ ] heap:  display heap stats
        - [ ] stats: display system information and metrics
        - [ ] lspci: display pci info

    - [ ] Network:
        - [x] ping
        - [ ] packetShark: trace packets and analyze by printing packet metadata and contents
        - [ ] curl

    - [ ] Filesystem:
        - [ ] ls
        - [ ] cat
        - [ ] type

    - [ ] Process:
        - [ ] ps
        - [ ] top

- [ ] dracos network:
    - [x] arp
    - [x] ip
    - [x] icmp
    - [ ] udp
    - [ ] tcp
    - [ ] basic html
    - [ ] better display and parsing for packets

- [ ] data structures:
    - [ ] map (rewrite to balanced binary tree (red-black) for asc. order, O(log n) access)
    - [x] hashmap (unordered, uses hash table, ammoritized O(1))
        - [x] use template specialization to allow different hashing for different types
    - [ ] dynamic array (pls dont call it "vector"), (accessible by index)
    - [x] list (not accessible by index)
    - [x] pair
    - [ ] stack
    - [ ] queue
    - [ ] set

- [ ] processes & threads:
    - [ ] assign userid and pid to each process created

- [ ] memory allocation 
    - [x] fix new and delete bug

- [ ] create an include/utils/
    - [ ] utils.h: cumulative utilties file that contains all other utilties
    - [x] print.h
    - [ ] math.h
    - [x] hash.h
    - [x] string.h
    - [x] memory.h
    - [ ] random.h
    - [ ] buffer.h
    - [ ] bitmap.h

- [ ] hardware
    - [x] pit timer
    
- [ ] create a test/
    - [ ] testSkeleton.h
    - [ ] testRunner.cc
    - [ ] hardware/test_{ata,keyboard,mouse,network}.cc
    - [ ] memory/test_{heap,memcpy}.cc
    - [ ] filesystem/test_{io,directory}.cc
    - [ ] system/test_{multitask,syscalls,interrupts}.cc
    - [ ] utils/test_{math,string,print}.cc
    - [ ] benchmarks/benchmarks_{disk,memory,task}.cc


- [ ] graphics (v2):
    - [ ] name: ... 
    - [ ] make the mouse cursor a crosshair (center dot, outer circle, 4 tick marks in cardinal directions)
    - [ ] bitmap font (reference & inspiration: https://www.dafont.com/alagard.font, give credit)
    - [ ] windowing

- [ ] change the hard drive driver design from pio (program i/o) to dma (direct memory access)

extra:
- name maybe "dracos" // pronounced dracos
- name the filesystem scalesfs (for scalability ~hehe)
