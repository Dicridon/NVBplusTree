# B+ tree implemented using PMDK

A very simple B+ tree running on NVM implemented using PMDK. This B+ tree is derived from the DRAM B+ tree in my repo https://github.com/Dicridon/BPlusTreeInC
I have verified the correctness of the DRAM B+ tree, so perhaps this NVM B+ tree would just function correcly

For explanation of macros and interfaces I used, check pmem.io

# Some hints about PMDK
## "POINTERS"" are DISGUSTING
PMDK provides some interfaces for us to access persistent memory, e.g. D_RW, D_RO, pmemobj\_direct, etc. These interfaces takes PMEMoid-type variable, which is "pointer" in PMDK, as parameter and returns a real pointer to us. 
So somehow we may treat PMEMoid variables like two level pointers, thus we must be extremely cautious when manipulating them, since modifying two level pointers usually ends with undesirable results.

## Think CAREFULLY Before MODIFYING a TOID varaible
As I mentioned above, "pointers" and pointers in PMDK are disgusting, because what we get from those interfaces is two level pointers, while one level pointers are generally enough for development. So we must carefully distinguish our needs: do we want a value or an address???? If we want an address, is the address itself what we need or the value stored at that address? If we actually want the value stored at the address, just take value rather than a pointer. The misuse of addresses produces most bugs in programs using PMDK.

## Think CAREFULLY BEFORE Wrapping A Pointer Into TOID Data Structure
Wrapping a pointer into TOID type means that you get a two or even three level pointer. Troubles caused by such variable are usually hard to solve.

## KEEP MEMORY LAYOUT IN MIND
Being clear about what a variable may look like in NVM may greatly help you to locate incorret code. This is especially useful when you are dealing with type conversions and designing data structure. (It feels like that the virtual memory technology has not emerged, so we have to manipulate physical memory like those ancient programmers.)
