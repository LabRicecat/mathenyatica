# !! MOVED TO CODEBERG.ORG !!
**This is an old version, please visit the current version at [codeberg.org](https://codeberg.org/LabRicecat/mathenyatica)!**

# Mathenyatica

## Installation
For this project, you need to install the [CatCaretaker](https://github.com/labricecat/catcaretaker) for dependency management. (The `catcare` command below)  
as well as the `readline` library.  
To build, type: 
```
$ git clone https://github.com/labricecat/mathenyatica
$ cd mathenyatica
$ catcare sync
$ mkdir build && cd build
$ cmake .. && make install
$ mathenyatica
```

## Usage
```
$> fn <function definition>
$> eval <function>(parameter)
$> info <function>
$> file <file.mny>
$> mathenyatica
```

## Syntax
```
FUNCTIONARGS STATEMENT, STATEMENT, ...
```
### Examples
```
; imports different files
% file.mny file2.mny ...

; function t with no arguments that returns true
t TRUE

f FALSE

; function f1 with one parameter that returns its parameter
f1 1

; function f2 that returns the first argument AND the second argument
f2 1 & 2

; function "or2" with 2 parameters
or2 !(!1 & !2)

; function that takes 3 parameters and return arg1 OR arg2 OR arg3
or3 or2(or2(1 2) 3)

; function if3 that returns its second parameter if the first is true, otherwise the third
if3 or3((1 & 2) (2 & 3) (!1 & 3))

; function sw1 with 2 return values
sw1 if3(1 t f), if3(!1 t f)

; defines a function foo2 which uses the first and second return value of sw1
foo2 sw1(1) & sw<2>(!2)
```
