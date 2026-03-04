# What is it
A command line C declaration parser
from the 'Expert C Programming - Deep C Secrets' book Pg.81.

# How to use
Compile the code:
## On Windows (must have MSVC)
In the main directory run:
```Shell
.\make.bat .\Cdecl.c -b
```
And then run:
```Shell
.\build\Cdecl.exe "your declaration" 
```
For Example:
```Shell
.\build\Cdecl.exe "char *(*c[10])(int **p) " 
declaration = char *(*c[10])(int **p) 
c is array 0..9 of pointer to function returning pointer to char
```

## On Linux
In the main directory run:
```Shell
INPUT="your declaration" make Cdecl
```
For Example:
```Shell
INPUT="char *(*c[10])(int **p) " make Cdecl
declaration = char *(*c[10])(int **p) 
c is array 0..9 of pointer to function returning pointer to char
```

