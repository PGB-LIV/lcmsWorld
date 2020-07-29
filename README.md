# lcmsWorld
lcmsWorld is a 3d viewer for LC-MS data (it can load ```.mzml``` or Thermo ```.raw``` format files)


## Requirements:

A computer (Windows / Linux / Mac?) with at least 1gb RAM (more is better), OpenGL 3.0+.

Roughly the same amount of hard disk space as the .raw, .mzml being viewed. 

For large files, preferably loaded from fast hard disk - the viewable .lcms file is created on the same disk, and 
data is streamed from it during use.

## Using lcmsWorld:

For Microsoft Windows, go to the [releases](https://github.com/PGB-LIV/lcmsWorld/releases/) page and download the lcmsWorld .zip file from the 
latest version.
Copy the folder from inside the downloaded zip file to a hard disk, and double-click on lcmsWorld.exe to start the program.  
When lcmsWorld starts, you should select the 'File' menu, 'Load LC-MS' file, and change the file filter to either .raw or .mzml.  
The first time you load a file, lcmsWorld automatically converts this file and creates a corresponding .lcms file.  
In future, you can load this .lcms file to start viewing instantly.
	
Some example .raw files can be found at http://pgb.liv.ac.uk/lcmsWorld/testfiles  
You can also download identification files, which can be loaded via the lcmsWorld 'File' menu, and then viewed via
the 'Annotations' menu.  
Hold the left mouse button to rotate the viewpoint, and use the mouse wheel to zoom in and out.  
Hold the right mouse button to drag the viewpoint around.  
There are also some basic instructions in the 'Help' menu.

For linux or MacOS, you will currently need to download the source and build an lcmsWorld executable file yourself - see below.

## Build Instructions:

### MS Visual Studio 2019 (Community Edition)

Download / clone the whole folder structure, and open the lcmsWorld project file.

Compile & Run.  The Debug build is painfully slow for loading.

(Alternatively, use cmake as below, but this was only tested with the Microsoft Visual C++ compiler)

### Linux

To compile this project for Linux, you need to install ```cmake``` and ```xorg-dev```. If you're running Debian/Ubuntu you
can do this by running the following command:

```
sudo apt install cmake xorg-dev
```

Download / clone the whole folder structure:

```
git clone https://github.com/tonyatliv/lcmsWorld
```

Open a terminal in the lcmsWorld folder and run the following to compile a binary:

```
cmake . && cmake --build . --config Release
```

Once completed, you should now have a lcmsWorld binary in the root of the project folder. Run it using:

```
./lcmsWorld
```

### MacOS

As for Linux, but hasn't been tested for a long time, and is unlikely to work without changes.

## Usage Notes

- Camera controls are listed in the 'help' section.
- RawFileReader from Thermo is used to load .raw files.  This is a 64-bit only .NET application.
- You may need to install .NET Framework 4.7 on Windows to use this.

Identification files in ```.csv``` or tab-separated text formats are supported. The column names 
supported are listed below (The accepted alternative names are separated by commas):

- m/z  (required)  
- rt, retention time  (required)  
- intensity  (required)  
- peptide, sequence, modified sequence  
- ptm
- accession, protein accession  
- 10lgp, score, local confidence(%)  
							

## To do:
- See Issues (there will be lots, little error checking is performed)
- Test on various other input files.
- Make a sensible menu system.
- Better loading of identifications.
- Better display / management of identifications.
- Test Memory management system / better algorithm for switching detail levels.
- Headless mode for file conversions.


## Future features:
- Automatic alignment / adjustment over different views.
- Subtraction mode (maybe offline?)
- Ion mobility support.
- Web-based viewer (webGL).
- Defender mode.

