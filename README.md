# JubeatMenu

A launcher menu for Jubeat that uses the 16-button panel to select games.

Includes a Visual Studio 2008 solution that compiles to a static DLL which runs with launcher.exe on XPE, suitable for use on a Jubeat cabinet.

The ini file format is simple. For each game, there should be a section which is the game name. For each section, there should be a "launch" key which points to the full path of the executable or batch file to execute when selecting this option. An example is below:

```
[Saucer]
launch=D:\L44-saucer\contents\gamestart.bat

[Saucer Fulfill]
launch=D:\L44-saucer-fulfill\contents\gamestart.bat

[Prop]
launch=D:\L44-prop\contents\gamestart.bat
```

To correctly execute the built code, run the DLL with the game launcher.exe. It will load games from `games.ini` in the same directory as the DLL. This DLL makes use of the original game's driver code to read inputs, so it should be run from a directory containing a working installation. An example invocation is as follows:

```
launcher.exe JubeatMenu.dll
```