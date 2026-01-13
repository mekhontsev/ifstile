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
To perform calculation with numbers and affine maps it is possible to use ordinary operations like +,-,\*,/,^ and brackets.\
For numbers **sin**, **cos**, **tan**, **asin**, **acos**, **atan**, **exp**, **log**, **floor**, **ceil**, **arg** are defined.\
Also '**if**' function defined: if (cond, val1, val2) is equivalent to (cond > 0)?val1 : val2 in C-like languages.

### Identifiers
An identifier is a case-sensitive string that denotes a variable, an operator or a block. An identifier can consist of symbols \[a-z\], \[A-Z\], \[0-9\] and "\_", but cannot start with a digit.\
Identifiers beginning with $ and & symbols have a special meaning.\
Variable with a name beginning with '$' symbol is considered as built-in variable and has special meaning.\
Variable with a name beginning with '&' symbol is considered as substitution and recalculated in every place where used.\
Each variable can be assigned only once, see [Static single assignment form](https://en.wikipedia.org/wiki/Static_single_assignment_form), but the order of the definitions is not important.

### Vectors
Like in many other languages, v=\[a1, a2, .., an\] defines the vector of elements (a1, a2, .., an).\
We can access to the elements of a vector using square brackets, so a1 can be accessed as v\[0\], and an = v\[n-1\].\
A vector of appropriate length can be automatically converted to a matrix or to a translation.

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

**$charpoly(A)** computes the [Characteristic polynomial](https://en.wikipedia.org/wiki/Characteristic_polynomial) (as an n-dimensional vector) for linear part of the affine map A.

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

**Empty set**
**$e**

### Templates
Templates describe infinite sets of affine maps together with a random distribution that can be used in the search procedure and in the editor window.\
Templates can be used in the same places where affine maps appear: within compositions, unions, etc.

**$number(T)**
Real or integer number.\
T= **$integer** or **$real** : \[optional\] type and a random distribution of the number.

**$vector(L, T)**
Vector of numbers,\
L = length of the vector, L = 0 or L=empty means L = $dim.\
T= **$integer** or **$real** : \[optional\] type and a random distribution for the vector entries.

**$binary(L, T)** 
Vector of empty sets ($e) and identity maps ($i).\
L = length of the vector, L = 0 or L=empty means L = $dim.\
T= **$integer** - represents the number of empty ($e) components.

**$semigroup(\[g1, g2,...,gm\], T)** 
Element of the [semigroup](https://en.wikipedia.org/wiki/Semigroup) (possibly infinite) generated by the affine maps g1, g2, ..., gm.\
T=**$integer** - \[optional, for infinite semigroups only\] represents normal distribution for the length of compositions of the generators.

**$real(a, b)**, **$integer(a, b)** 
Type and uniform distribution for real or integer numbers from a to b.

**$real(v)**, **$integer(v)** 
Type and (half-)normal distribution for real or integer numbers.\
\|v\| - variance.\
if v < 0 then distribution is normal.\
if v > 0 then distribution is half-normal.

### Javascript
Any AIFS file may contain JavaScript module at the beginning.
The module must export array named '$aifs'.

export const $aifs =
[
	block1, block2, ....
]

Each element of the array is a JavaScript object that defines one
IFS block. 

The module can also export a string "$info" with a description of the current file (using markdown).


For more information, see examples in the JavaScript drop-down list.




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

"Render" button - automatically renders specific blocks to a user defined folder.

**Interpolation tab**

"Create frames" button - creates intermediate blocks and adds them to the list - without rendering. It uses checked blocks as keyframes (see Examples/Animation).
Created blocks can be rendered later using "Batch" tab. 

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