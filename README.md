# Thanks for vising the Lunar repository! <br>
Version: 0
**As of now, this README will only contain the syntax of the language. I intend to build upon this further more, and very soon.**<br>
**SOME CONSTRUCTS MAY BE PARSED BUT NOT YET EXECUTABLE AT SUCH AN EARLY STAGE. YOU HAVE BEEN WARNED!**
<hr>

## Source Files:
Lunar source files use the `.lr` extension.<br>
As of right now, top-level execution statements are not allowed<sup>(temporarily)</sup>.<br>


## Whitespaces:
Lunar uses identation-based blocks, similar to <a href='https://www.python.org/downloads/'>Python</a>.<br>
Blocks start after `:` followed by a newline and an increased indent.<br>
An example of the following:
```
if x == 1:
    print(1);
    print(2);
``` 

## Comments:
Lunar supports both single and double line comments.<br>
```
// single-line comment

/*  
    multi-line
    comment
*/
```

## Types and typedefs:
Current types/identifiers available are: `int`, `bool`, `string`<br>
Container types use bracket syntax: `list[int]` or `map[string]` as examples. Nested containers: `list[list[int]]` are valid and work.

## Function decl:
```
<return_type> func <name>(<params>?):
        <statements>
```
A valid function example:
```
int func main(x: int, y: int):
    return x+y;
```
Writing only `return;` returns 0.

## Variable decl:
`let <name>: <type> = <expression>;`<br>
Example (with mutable):
`let mut count: int = 0`

## If statement:
```
if <condition>:
    <statements>
else:
    <statements>
```

## While loop:
```
while <condition>:
    <statements>
```

## Expressions:
Literals:
```
123
true
false
"hello"
```
Identifiers:
```
x
total
my_value
```
Unary operators:
```
-x
!x
```
Binary operators:
```
+   | addition
-   | subtraction
*   | multiplication
/   | division
==  | equality
!=  | inequality
<   | less than
<=  | less than or equal
>   | greater than
>=  | greater than or equal   
```

## Func calls:
```
print(42);
add(1,2);
```

## Built-in funcs:
As of right now, only `print(<value>)` is a builtin function.

## Program entry:
Program entry point must be in a function named `main`:
```
int func main(<args (optional)>):
    return 0;
```

### All statements must end with a semicolon.

End of file.