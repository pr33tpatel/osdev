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
    - [ ] VIM keybinds?

- [ ] dracos network:
    - [x] arp
    - [x] ip
    - [ ] udp
    - [ ] tcp
    - [ ] basic html

- [ ] processes & threads:
    - [ ] assign userid and pid to each process created

- [ ] create an include/utils/
    - [ ] utils.h // cumulative utilties file that contains all other utilties
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
    - [ ] test_framework.h
    - [ ] test_runner.cc
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
