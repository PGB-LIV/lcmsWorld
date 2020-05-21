# lcmsWorld
lcmsWorld is a 3d viewer for LC-MS data (it can load .mzml or Thermo .raw format files)


Requirements:
A computer (Windows / Linux / Mac?) with at least 1gb Ram (more is better), OpenGL 3.0+.
Roughly the same amount of hard disk space as the .raw, .mzml being viewed. 
For large files, preferably loaded from fast hard disk - the viewable .lcms file is created on the same disk, and 
data is streamed from it during use.

Build Instructions:
MS Visual Studio 2019 (Community Edition)
Download / clone the whole folder structure, and open the lcmsWorld project file.
Compile & Run.  The Debug build is painfully slow for loading.
(Alternatively, use cmake as below, but this was only tested with the Microsoft Visual C++ compiler)

Linux
With cmake / gcc (and  xorg-dev installed)
Download / clone the whole folder structure.
Open a terminal in the lcmsWorld folder and run the following:-
cmake .
cmake --build . --config Release
And you should generate a ./lcmsWorld program that can be executed from the terminal.

Mac
As for Linux, but hasn't been tested for a long time, and is unlikely to work without changes.

Usage Notes:
Camera controls are listed in the 'help' section.
RawFileReader from Thermo is used to load .raw files.  This is a 64-bit only .Net application.
  You may need to install .Net Framework 4.7 (Windows) or Mono (Mac / Linux) to use this.
A few different text-based identification formats are supported, just make sure it has columns for peptide,m/z,rt

To do:
See Issues (there will be lots, very little error checking is performed)
Test on various other input files.
Make a sensible menu system.
Better loading of identifications.
Better display / management of identifications.
Test Memory management system / better algorithm for switching detail levels.
Headless mode for file conversions.


Future features:
Automatic alignment / adjustment over different views.
Subtraction mode (maybe offline?)
Ion mobility support.
Web-based viewer (webGL).
Defender mode.

