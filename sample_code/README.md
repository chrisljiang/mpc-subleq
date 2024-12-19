# File names explained

* `program_name.sq`
    - actual runnable subleq program
* `program_name.asq`
    - subleq assembly code
* `program_name - annotated.txt`
    - program_name.txt (or very close to) with annotations like data indices written out
* `program_name - partial.txt`
    - relatively "human readable" subleq code with some cleanup done versus some condensed syntax
* `program_name - rough.txt`
    - rough work related to deriving the final program

# Program names explained

* `constant_time`
    - `constant_time` indicates program was written specifically to avoid timing side channels

* `uniform` vs `nonuniform`
    - `uniform` indicates program which allows code to be modified during run
    - `nonuniform` indicates program which does not allow code to be modified during run

* `3` vs `4`
    - `3` indicates program where each instruction contains 3 addresses ([A, B, C] where A and B are data addresses and C is an instruction address)
    - `4` indicates program where each instruction contains 4 addresses ([A, B, C, D] where A and B are data addresses and C and D are instruction addresses)

* `subble`
    - `subble` indicates file is formatted so that it can be easily split up into seperate program and data files to be run with subble

* `dawnos`
    - `dawnos` indicates program was compiled using DawnOS's cross compiler

* `higher`
    - `higher` indicates program was compiled using Higher Subleq
