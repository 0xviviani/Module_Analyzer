# Module_Analyzer
Module_Analyzer is a module for the PSP which analyzes the attributes of custom modules (i.e. game modules, homebrew modules) which are loaded during the boot process of a PSP Program. It maps the imported sce functions to their stub addresses and does segment mapping to give you an entry point when you want to start reversing a game/program. 
\
This fork aims to ease the usage of this module by creating a separate folder for each game or homebrew that is ran. Below is a list explaining what Module_Analyzer monitors and logs.


* Modules loaded during the boot process (official Sony modules and custom ones)
* The .text section containing the segment of executable code (only for custom modules)
* The .stub section, which points to the imported libraries (only for custom modules)
* The main entry point of the custom module
* The global pointer ($gp) value
* The SHA-1 hash of the imported function name and their addresses
* The exported functions/variables of the program in user land
\
\
![Imgur](https://i.imgur.com/a21NA2F.png)