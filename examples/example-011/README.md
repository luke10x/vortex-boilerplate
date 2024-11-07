Lines and Splines
=================

Implements some non-reusable (for now)
way to draw line primitive.
It seems it does not support changing width neither on web or mac,
therefore not really good for productions, just for dev code.

Potential way to implemetnt width changing in splines are:
    - thin quads;
    - 1D texture with a gradient that simulates line's thickness
    - Geometry shader is NOT availabe for OpenGL ES 300

For splines it uses a simple Catmull-Rom method.