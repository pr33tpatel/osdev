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
    - [x] Command Registry
    - [ ] edit command buffer and shift data
    - [ ] command history
    - [ ] highlighting
    - [ ] copy and paste
         
    - [ ] Optional:
        - [ ] VIM keybinds?

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

- [ ] processes & threads:
    - [ ] assign userid and pid to each process created

- [ ] memory allocation 
    - [x] fix new and delete bug

- [ ] create an include/utils/
    - [ ] utils.h: cumulative utilties file that contains all other utilties
    - [x] print.h
    - [ ] math.h
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
