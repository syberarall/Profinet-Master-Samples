Profinet Master Samples — RTC1

This directory contains RTC1 sample applications for the SYBERA PROFINET Realtime Master Library.
These examples demonstrate usage scenarios, device integrations, and send/receive modes with
SYBERA’s Profinet master stack.

Note: These examples are primarily intended for Visual Studio C++ development on Windows®.

The SYBERA PROFINET Master Library enables deterministic, cyclic data exchange over standard
Ethernet interfaces, supporting PC-based real-time Profinet control without dedicated controller
hardware.

------------------------------------------------------------
DIRECTORY CONTENTS
------------------------------------------------------------

BCL348.cpp
CN8032.cpp
F5868.cpp
F5868_20x.cpp
Phoenix_FL_IL_24BK_PN_PAC.cpp
Phoenix_ILB_PN24_DI16_DIO16.cpp
PnioNetTst.cpp
PnioService.cpp
Pool_CE65M_FLIL.cpp
SendModeClocked.cpp
SendModeFlushed.cpp
SendModePll.cpp
TR_CE582M.cpp
TR_CE65M.cpp

Each file provides a different Profinet application example or test scenario, such as support
for specific hardware modules (Phoenix Contact, BCL, F5868, TR series), configuration of cyclic
data, and examples of different transmit modes (Clocked, Flushed, PLL). These samples serve
as starting points to integrate Profinet communication in your own applications using Visual
Studio C++.

------------------------------------------------------------
ABOUT SYBERA PROFINET MASTER LIBRARY
------------------------------------------------------------

The SYBERA PROFINET Master Library is a real-time controller stack for Windows® that allows
developers to implement Profinet IO Master functionality on a standard PC with standard
Ethernet adapters. Real-time communication is achieved through SYBERA’s realtime engine and
transport layer, enabling update cycles of up to ~250 µs and deterministic cyclic data exchange
without external controller hardware.

The library manages Profinet station configuration, cyclic IO data exchange, diagnostics,
acyclic services, and protocol handling (ARP, DCP, RPC, LLDP), providing a comprehensive
programming interface for machine control and automation tasks. The official documentation in
manual_pnt_clu_e.pdf details configuration formats for station lists, module and submodule
definitions, and IO object structures.

------------------------------------------------------------
KEY CONCEPTS
------------------------------------------------------------

PROFINET Realtime (RTC1):
- Deterministic cyclic process data exchange over Ethernet frames
- Communication without TCP/IP stack
- Managed via the SYBERA Master Library’s station and module configuration model

Real-Time Modes:
- Clocked — timed transmission with defined cycle intervals
- Flushed — ensures complete frame delivery before the next cycle
- PLL — phase-locked loop mode for tight synchronization

------------------------------------------------------------
BUILD & RUN (Visual Studio C++)
------------------------------------------------------------

Prerequisites:
- Visual Studio C++ (recommended)
- SYBERA Profinet Master library and headers
- Ethernet adapter or simulator for testing

Build Steps:
1. Open the Visual Studio solution (or create a new C++ project)
2. Add the source files from RTC1 to the project
3. Set library include paths and linker options for the SYBERA Master Library
4. Compile and run the application

Execute a Sample:
PnioNetTst.exe --config=config.yaml

Replace PnioNetTst.exe and the config file as appropriate. Many applications rely on
configuration files or command-line options to define network parameters and device lists.

------------------------------------------------------------
CONFIGURATION
------------------------------------------------------------

Per the official documentation (manual_pnt_clu_e.pdf), Profinet Master expects a
station list file defining:
- Station name and parameters
- Vendor and module identifiers
- MAC and IP parameters
- Module and submodule IO definitions

These entries allow the master library to correctly map and exchange cyclic IO data with each device.

------------------------------------------------------------
DOCUMENTATION
------------------------------------------------------------

For complete integration and detailed usage:

- SYBERA Profinet Realtime Master Library Manual (English):
  details API usage, station list format, IO object definitions, and real-time communication
  handling (manual_pnt_clu_e.pdf)
- Additional tools, training, and examples available on SYBERA.com

------------------------------------------------------------
LICENSE
------------------------------------------------------------

This directory follows the license terms of the main Profinet-Master-Samples repository.
Refer to the top-level LICENSE file for details.

------------------------------------------------------------
CONTRIBUTIONS
------------------------------------------------------------

Feedback, issues, and pull requests are welcome. Check the main repository’s CONTRIBUTING
guidelines for submission standards.
