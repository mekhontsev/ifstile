What is IFStile ?
===================

**IFStile** is a free ([GPL License](LICENSE)) cross-platform application for the discovery, visualization, and classification of self-similar sets and tessellations, bridging the gap between experimental mathematics and computer graphics.

**Features:**

* **Multidimensional IFS Construction**: Build affine directed graph [Iterated Function Systems](https://en.wikipedia.org/wiki/Iterated_function_system) in arbitrary Euclidean dimensions, with full support for 2D and 3D cross-sections.
* **Automated Fractal Discovery**: Leverage automated search to identify unique fractal structures, including [rep-tiles](https://en.wikipedia.org/wiki/Rep-tile), [carpets](https://en.wikipedia.org/wiki/Sierpi%C5%84ski_carpet), and [dragons](https://en.wikipedia.org/wiki/Dragon_curve).
* **Dimensional Analysis**: Compute the [dimensions of fractals and tile boundaries](https://en.wikipedia.org/wiki/List_of_fractals_by_Hausdorff_dimension) using both precise analytical methods and numerical approximations.
* **Geometric Analysis**: Compute centroids, moments, and diameters of fractal sets.
* **High-Performance Exploration**: Experience effective zooming into infinite fractal structures.
* **Professional Rendering**: Generate high-resolution images and keyframe animations with dedicated batch-rendering support.
* **Interoperability & Export**: Export to the Apophysis `.flame` format and generate 3D meshes for external creative workflows.

The application utilizes a declarative domain-specific language, [AIFS](manual.md), to define IFS sets, with full JavaScript integration for extending logic and creating custom definitions.

See the [IFStile official site](https://ifstile.com/) for more information.

Mathematical foundations can be found [here](https://epub.ub.uni-greifswald.de/frontdoor/deliver/index/docId/2479/file/MekhontsevElectronic.pdf).


Supported OS
------------
Windows 10+, macOS 10.13+, Linux with Glibc 2.27+ (Ubuntu 18, etc), Android 7.0+, WebAssembly.

Online version
------------
[Run IFStile in your browser](https://app.ifstile.com).

Build IFStile
---------------

Please follow [build guide](BUILD.md) to build IFStile from source.
