pcmcia_diag
-----------

This file contains:
1. An overview of the pcmcia_diag sample.
2. Instructions for building the pcmcia_diag application.
3. Guidelines for converting pcmcia_diag from a console-mode (CUI)
   application to a graphical (GUI) application.


1. Overview
   =========
   The pcmcia_diag/ directory contains the C source code of a diagnostics 
   WinDriver PCMCIA library and a user-mode console application (pcmcia_diag), 
   which uses the sample library API. For a list of the files provided in this 
   directory, refer to the pcmcia_diag/files.txt file.

   The sample code demonstrates how to use WinDriver's WDC library to 
   communicate with a PCMCIA device, including:
   -- Scanning the PCMCIA bus to locate a specific device and retrieve its 
      resources information
   -- Reading/writing from/to a specific address or register
   -- Reading from the PCMCIA attribute space
   -- Handling the interrupts of a PCMCIA device
   -- Registering to receive notifications for Plug and Play and power 
      management events for the device

   Note: WinDriver's PCMCIA API is currently supported on Windows 
   98/Me/NT/2000/XP/Server 2003.


2. Building the sample
   ====================
   To compile and build the pcmci_diag application you need an appropriate
   C/C++ compiler for your development platform.

   - If you are using one of the supported MSDEV or Borland C++ Builder IDEs
     (see files.txt): open the pcmcia_diag project/workspace/solution file from
     the relevant compiler sub-directory and simply build the project.

   - If you are using a different C/C++ compiler or make utility:
    - Create a new console mode project/makefile for your compiler.
    - Add the following files to the project/makefile:
      WinDriver/samples//pcmcia_diag/pcmcia_diag.c
      WinDriver/samples//pcmcia_diag/pcmcia_lib.c
      WinDriver/samples/shared/wdc_diag_lib.c
      WinDriver/samples/shared/diag_lib.c
    - Link your project with WinDriver/lib/wdapi<version>.lib, OR add the
      following files (which are exported in the wdapi<version> DLL) to your
      project/makefile:
      WinDriver/src/wdapi/utils.c
      WinDriver/src/wdapi/windrvr_int_thread.c
      WinDriver/src/wdapi/windrvr_events.c
      WinDriver/src/wdapi/status_strings.c
      WinDriver/src/wdapi/wdc_general.c
      WinDriver/src/wdapi/wdc_cfg.c
      WinDriver/src/wdapi/wdc_mem_io.c
      WinDriver/src/wdapi/wdc_ints.c
      WinDriver/src/wdapi/wdc_events.c
      WinDriver/src/wdapi/wdc_err.c
      WinDriver/src/wdapi/wdc_dma.c
    - Build your project.


3. Converting to a GUI application
   ================================
   This sample was written as a console mode application (rather than a GUI 
   application) that uses standard input and standard output.
   This was done in order to simplify the source code.
   You can change the sample into a GUI application by replacing all calls to 
   <stdio.h> functions such as printf(), sprintf(), scanf(), etc. with relevant
   GUI functions and replacing the main() function in pcmcia_diag.c with a GUI 
   equivalent.

