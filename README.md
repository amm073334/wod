internals are largely based off of jlox

## bugs
- if you pass a literal into an inline function, a copy is not created, so assigning to the parameter within the function will fail an assertion

- a `continue` statement in a for loop does not run the increment expression as expected, as for loops are currently implemented as purely a syntactic sugar for normal loops

- compiling with v[] global variables instead of cselfs will not work correctly unless all non-main functions are inline