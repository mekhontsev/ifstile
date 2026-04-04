---
layout: page
title: Manual
permalink: /manual/
---

[//]:#(AIFS)

### General
# AIFS Format Specification

**AIFS** is a specialized text format designed for representing [Iterated Function Systems](en.wikipedia.org) (IFS).

### Core Features

*   **Block-Based Architecture**
    An AIFS file can contain an arbitrary number of blocks (scaling to millions). Typically, each block defines an individual IFS or some family of IFS.

*   **Hierarchical Structure & Inheritance**
    Every block follows the `@ID:ParentID` syntax. A block inherits all definitions from its parent and can selectively override them, enabling an efficient modular design.

*   **Unique Identifiers**
    The **ID** and **ParentID** serve as unique identifiers. These are optional:
    *   Omit the **ID** if the block is not referenced elsewhere.
    *   Omit the **ParentID** for root-level blocks.

*   **Functional Calls**
    Blocks can be invoked as functions via their **ID**. During a call, the block's initial variables are mapped to the provided arguments, and the final variable is treated as the return value.

*   **Comments**
    Use the `#` symbol for single-line comments. The parser will ignore all text following the symbol on that line.


When exporting to PNG, the application automatically embeds the full generation state - including IFS parameters, palettes, and coordinates—as metadata. Re-importing these images into the software fully restores the workspace.


### Mathematical operations
To perform calculation with numbers and affine maps it is possible to use ordinary operations like +,-,\*,/,^,% and brackets.\
For numbers **sin**, **cos**, **tan**, **asin**, **acos**, **atan**, **exp**, **log**, **floor**, **ceil**, **abs**, **arg** are defined.

### Identifiers
An identifier is a case-sensitive string that denotes a variable, an operator or a block. An identifier can consist of symbols \[a-z\], \[A-Z\], \[0-9\] and "\_", but cannot start with a digit.\
Identifiers beginning with $ and & symbols have a special meaning.\
Variable with a name beginning with '$' symbol is considered as built-in variable.\
Variable with a name beginning with '&' symbol is considered as substitution and recalculated in every place where used.\
Each variable can be assigned only once, see [static single assignment form](https://en.wikipedia.org/wiki/Static_single_assignment_form), and the order of the definitions is not important.

### Conditional operator
'**if**' function:
* if (cond, val1, val2) is equivalent to cond>0 ? val1 : val2 in C-like languages.
* if (cond) is a step function defined as if (cond, 1, 0).
* if (cond1, val1, cond2, val2, ..., val_else) is equivalent to
if (cond1, val1, if (cond2, val2, if(..., val_else)...)

It possible to use any array as a condition. In this case, the condition is met if all array elements are positive.

Also, there is a 2 arguments version, that defined as if (v1, v2):
* 0 for v1==v1
* -1 for v1<v2
* +1 for v1>v2

In this case, v1 and v2 can also be arrays, which we compare lexicographically.


### Arrays (Vectors)
Like in many other languages, v=\[a1, a2, .., an\] defines the array of elements (a1, a2, .., an).\
We can access to the elements of an array using square brackets, so a1 can be accessed as v\[0\], and an = v\[n-1\].\
It is possible to use negative indices: an = v[-1] and a1=v[-n].
An array of appropriate length can be automatically converted to a translation or to a matrix using [row-major order](https://en.wikipedia.org/wiki/Row-_and_column-major_order) .

### Array size
To get an array size, use an empty brackets:
```python
a = [1, 3, 5, 7]
b = a()  #b=4
```

### Array flattening
Converts nested arrays to a single (flat) array by concatenating sub-arrays.
For the flat operation, use an empty square brackets:
```python
a = [[1, 2], [3, 4]]
b = a[] #a=[1, 2, 3, 4]
```

### Array slicing
Extracts a subset of elements from an array and returns them as a new array. The syntax is similar to Python: a[from:to:step].
* a[1:4] - elements from index 1 up to (but not including) 4.
* a[2:] - elements from index 2 to the end.
* a[:3]  - elements from the beginning up to index 3.
* a[::2] - every second element from the whole sequence.
* a[-3:] - the last three elements.
* a[::-1] - all elements in reverse order.

### Array fancy indexing
Allows you to select non-contiguous elements or subsets of an array by passing an array of integers as indices.
```python
a = [10, 20, 30, 40, 50]
i = [0, 2, 4]
b = a[i]  # equal to: [10 30 50]
```

### Array lazy indexing
Enables on-demand access to elements, avoiding full array computation. Use 'arr(index)' for that.
```python
a = [sin(5), cos(5)](1) # equal to: cos(5)
```

### Array self-referencing
When creating an array, we can access the already created part of the array using a special function: $(level), where level = 0, 1, ...
* $(0) is a reference to the current array
* $(1) is a reference to the parent array
* And so on...

For example:
```python
a = [0, $(0)[0]+1, $(0)[1]+1] #a=[0,1,2]
```

Since $(0) is the most commonly used in generators, there is a shortcut: $=$(0), so the previous example can be simplified to
```python
a = [0, $[0]+1, $[1]+1] #a=[0,1,2]
```

Example of a reference to the parent array:
```python
a = [5, [6, $(1)[0]] #a=[5,[6,5]]
```

All array functions (sizing, slicing, etc.) are available for the $(n) array.
```python
a = [$(), $(), $()] # a=[0,1,2]
```

### Array generators
During array creation, if any element is equal to the array itself (referenced by $), the creation process switches to a special **generator** mode. In this mode, the element following the $ symbol is considered the loop condition, and the element following it is considered the loop body.
Thus, as long as the condition is true (condition expression > 0), the expression body will be recalculated and its value will be added to the generated array. The conditional expression can depend on the current array size: $() , so it will allow as to stop after a finite number of steps.

Simple example:
```python
a=[$,5-$(),$()] #a=[0,1,2,3,4]
b=[0,1,$,7-$(),$[-1]+$[-2]] #b=[0,1,1,2,3,5,8]
```

Advanced example (Mandelbrot orbit):
```python
@
c=[-0.75, 0.1]
N=20
m=[
[0,0,0],   # initial [x,y,mod^2]
$, if(N-$())*if(4-$[-1][2]), # N>iter && 4>mod^2
[
    $(1)[-1],           # $[0] = [x,y]
    $[0][0],            # $[1] = x
    $[0][1],            # $[2] = y
    $[1]^2-$[2]^2+c[0], # $[3] = x'=x^2-y^2+Re(c)
    2*$[1]*$[2]+c[1],   # $[4] = y'=2*x*y+Im(c)
    $[3]^2+$[4]^2       # $[5] = mod^2(x',y')
][-3:] #last 3 elements: [x',y',mod^2]
]
```

### Special variables
**$n=...**
User-defined name of the IFS

**$a=...**
Attributes, can be **c**(hecked) and/or **h**(idden)

**$dim=n**
Dimension of all affine maps defined in the block. If **$subspace** is not used, then equal to dimension of the Euclidean space R^n where IFS will be generated.

**$subspace=[M, i1, i2, ...]**
Invariant subspace of the matrix M where IFS will be generated
i1, i2, ...  - indices of the Jordan blocks from [Real Jordan Form](https://en.wikipedia.org/wiki/Jordan_normal_form#Real_matrices) of M. Resulting subspace is a product of the subspaces that corresponds to the blocks. If omitted, then whole space (**$dim**) will be used.


### Affine maps
When $dim=n the following definitions of maps in R^n are available:

[Translation](https://en.wikipedia.org/wiki/Translation_(geometry))
**\[t1,t2, .., tn\]**

|**x1'**|&nbsp;|**x1+t1**|
|-------|------|---------|
|**x2'**| =    |**x2+t2**|
| ...   |      |    ...  |
|**xn'**|      |**xn+tn**|

[Transformation matrix](https://en.wikipedia.org/wiki/Transformation_matrix) (row-major)
**\[a11,a12, .., a1n,  ..,  an1, an2, .., ann\]**

|**x1'**|&nbsp;|**a11**|**a12**|...|**a1n**|&nbsp;|**x1**|
|-------|------|-------|-------|---|-------|------|------|
|**x2'**| =    |**a21**|**a22**|...|**a2n**|      |**x2**|
|  ...  |      |  ...  |  ...  |...|  ...  |      |  ... |
|**xn'**|      |**an1**|**an2**|...|**ann**|      |**xn**|

[Companion matrix](https://en.wikipedia.org/wiki/Companion_matrix)
**$companion(\[a0, a1, a2,..., a(n-1)\])**
for the polynomial a0 + a1\*x + ... + a(n-1)x^(n-1) + x^n.

[Diagonal matrix](https://en.wikipedia.org/wiki/Diagonal_matrix)
**$diagonal(\[a1, a2, a3,..., an)\])**

[Exchange matrix](https://en.wikipedia.org/wiki/Exchange_matrix)
**$exchange()**

Any number **x** can be implicitly converted to a matrix **x*I** where **I** is an n-dimensional [Identity matrix](https://en.wikipedia.org/wiki/Identity_matrix).

### Other functions and constants

**$charpoly(A)** computes the [Characteristic polynomial](https://en.wikipedia.org/wiki/Characteristic_polynomial) (as an n-dimensional vector) for linear part of the affine map A.

**$numden(r)** returns [numerator(r), denominator(r)] for a rational number r.

**$numden(x, eps)** returns a rational approximation of a real number x to within eps in the form [numerator, denominator]. On failure, it returns [0,0].

[Pi constant](https://en.wikipedia.org/wiki/Pi)
**$pi**

### Operators
**Composition of the maps (or operators):**
**f1\*f2\*...\*fn**
x => f1(f2(..(fn(x))) 

Examples in R²:

2(z+\[1,0\]) == 2\*\[1,0\]\
2z+\[1,0\]   == \[1,0\]\*2

x'=a11\*x+a12\*y+b1\
y'=a21\*x+a22\*y+b2\
can be expressed as\
\[b1,b2\]\*\[a11,a12,a21,a22\]


**Union of the sets (or operators):**
**S1\|S2\|...\|Sm**
S1 U S2 U ... U Sm. 
Examples:
  * (f1\|f2)\*A means f1(A) U f2(A)
  * H=f1\|f2\|...\|fn is a [Hutchinson operator](https://en.wikipedia.org/wiki/Hutchinson_operator)
  * A=H\*A is a definition of the attractor A

**Power function**
**f^n**
Composition f with itself n times f(f(f(...)))

**Inverse map**
**f^-1**

**Empty set (empty union)**
**$e**

**Identity map (empty composition)**
**$i**

**$union(a)**
Create a union of compositions of union of ... etc from the nested input array a.

### Complete minimal example (Sierpiński Triangle):

```javascript
@
$dim=2
f1=[0.5,0,0,0.5]
f2=[0.5,0]*[0.5,0,0,0.5]
f3=[0.25,0.5]*[0.5,0,0,0.5]
S=(f1|f2|f3)*S
```

### Templates
Templates describe infinite sets of affine maps together with a random distribution that can be used in the search procedure and in the editor window.\
Templates can be used in the same places where affine maps appear: within compositions, unions, etc.

**$number(T)**
Real or integer number.\
T= **$integer** or **$real** : \[optional\] type and a random distribution of the number.

**$vector(L, T)**
Array of numbers,\
L = length of the array, L = 0 or L=empty means L = $dim.\
T= **$integer** or **$real** : \[optional\] type and a random distribution for the array entries.

**$permutation([n0, n1, ..., nk])**
The array represents a multiset of values {0,1,...k} with given counts {n0,n1,...nk}.

**$real(a, b)**, **$integer(a, b)** 
Type and uniform distribution for real or integer numbers from a to b.

**$real(v)**, **$integer(v)** 
Type and (half-)normal distribution for real or integer numbers.\
\|v\| - variance.\
if v < 0 then distribution is normal.\
if v > 0 then distribution is half-normal.

### Javascript
Any AIFS file may include a JavaScript module at the beginning. The end of this module is indicated by the @@ string at the start of a new line.

The module can export array named **$aifs**.
```javascript
export const $aifs =
[
    block1, block2, ....
]
```
Each element of the array is a JavaScript object that defines one IFS block.
```javascript
block =
{
    var1: expression 1,
    var2: expression 2,
}
```
Every string expression is considered as an ordinary AIFS definition. For example
```javascript
{
    a: "1+1",
}
```
will be converted to AIFS as
```python
@
a=1+1
```

JavaScript arrays will be converted to AIFS arrays, except those ending with '|', '*', '/' characters. Such arrays will be converted to unions, compositions, and rational numbers:
```javascript
{
    a: 1,
    v: ["a", "a", "a"]
    u: ["a", "a", "a", "|"]
    c: ["a", "a", "a", "*"]
    r: ["a", "a", "/"]
}
```
will be converted to AIFS as
```python
@
a=1
v=[a,a,a]
u= a|a|a
c: a*a*a
r: a/a
```

The module can also export a string **$info** with a description of the current file (using markdown).

The module can export a **$constructor**=[js_par, js_func] entry to define a parameters dialog that appears after a file is loaded. Once the user modifies the parameters, js_func is executed to generate and return blocks based on those settings. For details, see tree.js.aifs example.

All exported JavaScript functions and constants are available in the aifs blocks:
```
export function inc(arg){
	return arg+1;
}
export function sum(a1, a2){
	return a1+a2;
}
export function arr_length(a){
	return a.length;
}
export function create_arr(a){
	return [a, a+1];
}
export const imm=[11,12];
@@
#AIFS started here
@
a=inc(3)
b=sum(a, 2)
c=arr_length([4,5,7])
d=arr_length([4.5,5,7,9])
e=create_arr(6)
f = imm[1];
```

When saving a file containing a JavaScript header, the entire script is included by default. To exclude specific sections of the script from being saved, wrap them with the //AIFS_IGNORE_BEGIN and //AIFS_IGNORE_END markers.

For more information, see the source code of the examples in the JavaScript section of the examples.

### JS in the Console
To execute a multiline JavaScript snippet in the Console window, select "Execute JS" from the dropdown menu.
Some useful domain-specific functions are available:
* ifs.id(name or ID) - find a block with the given name or ID and return its integer block_id.
* ifs.id() - returns integer block_id for the current block.
* ifs.blocks() - returns an array of block_id for all blocks.
* ifs.get(block_id, ["col1", "col2", ...]) - returns an array [v1,v2,...] of column values ​​corresponding to the block_id in the "IFS List" window.
* ifs.set(block_id,  "col", v) - sets column "col" to the value v for the block block_id. "col" can be "CH" - for checked flag, "HD" - for hidden flag, "ID" - for @ID and "Name" - for $n.
* ifs.m - all exported elements of the module.

Example: check blocks with odd block_id, uncheck other:
```javascript
for (const id of ifs.blocks()) {
    ifs.set(id, "CH", id%2)
}
```

Example: rename all blocks
```javascript
for (const id of ifs.blocks()) {
    let [Name] = ifs.get(id, ["Name"])
    ifs.set(id, "Name", id + " # " + Name)
}
```

[//]:#(Editor)
### Editor
There are 5 editing modes:
1. Controls - block-specific parameters (if available, see Examples/IR3).
2. Special - additional information that will be stored in the block (camera, background, etc).
3. Source -  source code of the current block.
4. JS - Javascript code located at the beginning of the current file.
5. @IFStile - special block in the current file containing search and rendering parameters.

Buttons:
* Apply - write changes back to original block.
* +Block - add the modified copy to the main list.
* Rand - randomize available parameters.

Mouse drag modifiers for numeric controls:
* Shift - increase change magnitude.
* Alt - decrease change magnitude.
* Control -  switch to keyboard editing.


[//]:#(Animation)
### Animation
**Batch tab**

"Render" button - automatically renders checked blocks to a user defined folder.

**Interpolation tab**

"Create frames" button - creates intermediate blocks and adds them to the list - without rendering. It uses checked blocks as keyframes (see Examples/Animation).
Created blocks can be rendered later using "Batch" tab. 


[//]:#(IFS List)
### IFS List

The IFS list contains all currently visible blocks.
There are several ways to navigate through the list:
* Mouse clicks.
* Arrow up and Arrow down buttons on the toolbar.
* Arrow up and Arrow down keys on the keyboard.
* PgUp and PgDn on the keyboard.
* << and >> buttons just above the list.

The leftmost column is a check mark (selection) for each block and can be inverted by the user by mouse clicks.
You can invert several checkmarks at once:
* Click the first desired checkmark using the left mouse button, then hold the Shift button and click the last one.
* Click the first desired checkmark, then click the last one, then choose "Interval" from the dropbox at the top of the window, and finally, click the "Apply" button.

All checkmarks states will be stored to the AIFS file.

To delete blocks, check them, then select the "Delete" item in the dropbox and click the "Apply" button.

To invert checkmarks, select the "Invert" item in the dropbox and click the "Apply" button.

You can select visible columns (the "C" button above the list) and set conditions to filter the rows (the "R" button).

By default, most available columns will be undefined after loading the file. To initialize these columns, you need to run a search (in the "Finder" window).

All visible columns can be exported to a CSV file (Menu->File->Export CSV)

[//]:#(Location)
### Location
The location window can be used to control screen/camera position and section settings.

There are additional useful controls:
* Toolbar buttons: "Fit to screen" and "Zoom out".
* Tools menu: "Zoom box", "Rotate" and "Pan" items to map the corresponding action to the first mouse button. Other actions will be automatically mapped to other buttons.
* Mouse wheel: to move camera forward or backward.

At any time, you can quickly save the location settings to memory using the Save and Load buttons at the top of the window.

"Pick" button can be used to pick target point from the screen point by mouse click. Also, in the "pick" mode you can see point coordinates under the mouse cursor.

### 3D
There are 4 parameters that fully control the position:
* Camera location
* Target point
* Vertical direction
* Field of view 

Distance from the camera location to the target point is a very important for animation interpolation and 3D navigation. This distance can be considered as a "zoom factor". To support this, there are 2 modes: Zoom (default) and Fly. The distance to the target is locked in the 'Fly' mode. In simple animation scenarios, it is better to separate zoom and "Fly" at a fixed zoom.

When the mouse is hovered over a 3D point on the screen, the relative depth (Zt) of the point is displayed in the status bar. This value is equal to the distance from the camera plane to the point divided by the distance from the camera to the target.

In the 'Fly' mode, a 'Sensitivity' parameter can be used to adjust speed of move controls. Mouse click to any 3D point on the screen will automatically set this parameter (equal to 'Zt').


[//]:#(Creator)
#### Creator
Creator can be used to create/find a family of IFS.
You can open creator from the main menu "File"->"New" or from the toolbar.

Usually you should:
* Fill the "Graph" field
* Optionally change other parameters
* Press "Search" button
* Press "Stop" button
* Select a group (and polynomial) from the list 
* Press the "Create" button

After it the program will switch to the IFS search mode.

When you find sufficient number of IFS (that can have dimension less than you are expected), you can switch to another search mode (in the "Finder" window) by:
* setting "#var maps" field to >=1 
* decreasing "#empty maps" 

Sometimes you need many hours to find something interesting, sometimes only several seconds!

### Examples
[Chair or Square family](https://en.wikipedia.org/wiki/Chair_tiling)\
One set: "A" that consists from 4 smaller pieces of itself with the same coefficients k\
We denote graph as: a.a.a.a or 4a\
Group = [D4 Dihedral](https://en.wikipedia.org/wiki/Dihedral_group), 360/4=90 degree rotation + reflection, Poly=2

***

[Rauzy tile family](https://en.wikipedia.org/wiki/Rauzy_fractal)\
One set: A that consists from 3 smaller copies of itself with coefficients k,k^2,k^3\
We denote graph as: a.a2.a3\
Group = [C2 Cyclic](https://en.wikipedia.org/wiki/Cyclic_group), 360/2=180 degree rotation, Poly=x^3-x^2+x+1

*** 

[Robinson triangle family](https://tilings.math.uni-bielefeld.de/substitution/robinson-triangle)\
Two sets: A,B\
A consists from 3 smaller copies of A,A,B with coefficient k\
B consists from 2 smaller copies of A,B with coefficient k\
We denote graph as: a.a.b-a.b or 2a.b-a.b\
Group = [D10 Dihedral](https://en.wikipedia.org/wiki/Dihedral_group), 360/10=36 degree rotation + reflection, Poly=-x^3+x^2+1


[//]:#(Shortcuts)
### IFS List
* Home
scroll to the first IFS
* End
scroll to the last IFS
* PgUp
scroll one screen up
* PgDown
scroll one screen down
* Up
move one IFS up
* Down
move one IFS down
* Left
build the previous set of the current IFS
* Right
build the next set of the current IFS
* Shift+Up
move to the next unique IFS up (according to the latest sort column)
* Shift+Down
move to the next unique IFS down (according to the latest sort column)
  
### Other
* F1
show context help
* F11
switch full screen
* Esc
close/cancel current modal window
* Ctrl+Tab
switch two latest panes
* Ctrl+R
reload current file from disk
* Ctrl+S
save file
* Ctrl+O
open file
* Ctrl+=
add current view to the list
* Shift+F4
show/hide the current pane
* Ctrl+U
randomize current set (if supported)

### Numeric controls modifiers
* Shift - fast change
* Alt - very slow change
* Alt+Shift - slow change
* Ctrl - keyboard input