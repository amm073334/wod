# wod
A C-like programming language for doing weird stuff in Wolf RPG Editor.

It's largely untested and has a lot of bugs, so use at your own risk.

The scanner, parser, and AST are largely based off of the Java implementation of Lox, which is here: https://github.com/munificent/craftinginterpreters

## Important bugs and unimplemented things
- Currently, only ASCII characters are properly supported in source text

- If you pass a literal into an inline function, a local copy is not created inside the function, so assigning to the parameter within the function will fail an assertion during code generation for assigning to a literal

- A `continue` statement in a for loop does not run the increment expression as expected, as for loops are currently implemented as purely a syntactic sugar for normal loops

- Compiling with `use-globals` will not work correctly unless all non-main functions are inline

- Using f-strings with string variables when `use-globals` is on doesn't work

- `import` statements don't work the way they probably should, and work more like `#include` directives; if you import the same file into different files the compiler will complain

## Running
```
.\wodc.exe <file> <output-dir> [use-globals]
```
`use-globals` can be anything; the presence of the last argument equates to `true`. When active, it switches variables to use v[] and s[] globals variables instead of CSelfs, which is useful if there aren't enough CSelfs to go around. It was mostly created as a stopgap measure, though, and breaks easily (as listed above).