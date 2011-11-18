//
//      ii_iostr.h   --   Useful structures for communicating from 
//                           drivers to dlls to applications
//

#ifndef __ii_iostr_h__
#define __ii_iostr_h__

#ifndef ULONG_DEFINED
typedef unsigned long ULONG;
#define ULONG_DEFINED
#endif

//================================================
//   Hardware Description Structures
//
//------------------------------------------------------------------------
//   struct MemoryBlock -- Memory Area description
//------------------------------------------------------------------------

typedef struct _MemoryBlock
{
    ULONG   Addr;       // Mapped Address of Block Base
    ULONG   DriverAddr; // Linear Address of Block Base
    ULONG   PhysAddr;   // Physical Address of Block Base
    ULONG   Size;       // Block Size in bytes
    ULONG   MemoryType; // Port or Memory Mapped
    ULONG   WasMapped;  // Was Mapped or not
} MemoryBlock, *PMemoryBlock;

//------------------------------------------------------------------------
//   struct IoPortBlock -- IO Port block description
//------------------------------------------------------------------------

typedef struct _IoPortBlock
{
    ULONG   Base;       // Base Port of Block
    ULONG   Size;       // Block Size in bytes
    ULONG   MemoryType; // Port or Memory Mapped
    ULONG   WasMapped;  // Was Mapped or not
} IoPortBlock, *PIoPortBlock;


#endif



