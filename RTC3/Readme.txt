Profinet Master Samples — RTC3

This directory contains RTC3 sample applications for the SYBERA PROFINET Realtime Master Library.
These examples demonstrate Isochronous Real-Time (IRT / RTC3) communication scenarios, device
integrations, and PROFIdrive-related functions using SYBERA’s Profinet master stack.

Note: These examples are primarily intended for Visual Studio C++ development on Windows®.

The SYBERA PROFINET Master Library supports deterministic cyclic and isochronous data exchange
over standard Ethernet interfaces, enabling PC-based real-time control for high-performance
automation tasks.

------------------------------------------------------------
DIRECTORY CONTENTS
------------------------------------------------------------

CN8032_F5868_S5868.cpp
Ertec.cpp
F5868.cpp
ProfiDriveChange.cpp
ProfiDriveGlobal.cpp
ProfiDriveRead.cpp
ProfiDrive_CU320.cpp
ProfiDrive_SI6.cpp
TR_CE582M.cpp
TR_CE65M.cpp

Each file demonstrates a different RTC3 application or test scenario, such as device-specific
examples (CN8032, F5868, TR series, Ertec), PROFIdrive services (Change, Global, Read, CU320,
SI6), and IRT cycle configuration. These examples illustrate how to configure and communicate
with PROFINET IRT devices using Visual Studio C++.

------------------------------------------------------------
ABOUT SYBERA PROFINET MASTER LIBRARY (RTC3)
------------------------------------------------------------

The SYBERA PROFINET Master Library is a real-time controller stack for Windows® that implements
Profinet IO Master functionality, including Real-Time Class 3 (Isochronous). It provides:

- Deterministic cyclic IO communication
- Isochronous real-time synchronization for drives and actuators
- Configuration of station lists, modules, and submodules
- PROFIdrive service access (read, write, global, change)

The official documentation in manual_pnt_clu_e.pdf describes how to define station lists,
module/submodule structures, and IO objects to support RTC3 communication.

------------------------------------------------------------
KEY CONCEPTS
------------------------------------------------------------

PROFINET Realtime Class 3 (RTC3 / IRT):
- Isochronous, time-critical cyclic process data exchange over Ethernet
- Required for high-speed servo, drive, and motion control applications
- Managed via SYBERA Master Library station/module/submodule configuration

PROFIdrive Services:
- ProfiDriveChange — Modify parameters on the device
- ProfiDriveGlobal — Global commands for multiple drives
- ProfiDriveRead — Read parameters or status from devices
- ProfiDrive_CU320 / ProfiDrive_SI6 — Device-specific PROFIdrive examples

Real-Time Synchronization Modes:
- RTC3 ensures strict cycle time adherence, suitable for drives requiring deterministic IO
  and phase-synchronized updates.

------------------------------------------------------------
BUILD & RUN (Visual Studio C++)
------------------------------------------------------------

Prerequisites:
- Visual Studio C++ (recommended)
- SYBERA Profinet Master library and headers
- Ethernet adapter or simulator for testing

Build Steps:
1. Open the Visual Studio solution (or create a new C++ project)
2. Add the RTC3 source files to the project
3. Set include paths and linker options for the SYBERA Master Library
4. Compile and run the application

Replace the executable and config file as appropriate. Many applications rely on configuration
files or command-line options for station, module, and device parameters.

------------------------------------------------------------
CONFIGURATION
------------------------------------------------------------

The SYBERA Master Library requires a station list file defining:
- Station names and properties
- Vendor and module identifiers
- MAC/IP addresses
- Module/submodule IO mappings

These definitions enable the master library to exchange cyclic and isochronous data correctly
with all connected devices. Refer to manual_pnt_clu_e.pdf for detailed structure.

------------------------------------------------------------
DOCUMENTATION
------------------------------------------------------------

For detailed integration and API usage:

- SYBERA Profinet Realtime Master Library Manual (English) (manual_pnt_clu_e.pdf)
- PROFIdrive examples and configuration guidelines
- Additional tools and training at SYBERA.com

------------------------------------------------------------
LICENSE
------------------------------------------------------------

This directory follows the license terms of the main Profinet-Master-Samples repository.
See the top-level LICENSE file for details.

------------------------------------------------------------
CONTRIBUTIONS
------------------------------------------------------------

Feedback, issues, and pull requests are welcome. Check the main repository’s CONTRIBUTING guidelines.
