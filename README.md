What is IFStile ?
===================

**IFStile** is a free ([GPL License](LICENSE.txt)) cross-platform application that can:

* build any affine directed graph iterated function system (IFS) in an Euclidean space of arbitrary dimension (as 2D or 3D section)
* fully automatically find interesting fractal shapes, rep-tiles, multi-tiles, irreptiles, carpets, dragons, etc
* extract the boundary of self-affine tiles as directed graph IFS
* compute dimensions of the boundary of self-affine tiles (numerically and analytically)
* export Apophysis .flame format
* effectively zoom IFS fractals
* render high resolution images (with batch rendering)
* render keyframe animation
* create and save 3D mesh

IFStile uses a special rendering algorithm that can unveil complex structures of the fractal.

To describe IFS sets, the declarative domain-specific language "AIFS" is used. JavaScript language can also be used to extend AIFS definitions.

See the [IFStile official site](https://ifstile.com/) for more information.

Supported OS
------------
Windows 10+, macOS 10.13+, Linux with Glibc 2.27+ (Ubuntu 18, etc), Android 7.0+, WebAssembly

Build IFStile
---------------

Please follow [build guide](BUILD.md) to build IFStile from source.
