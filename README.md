# Mimic Preprocessor

## Purpose

When a scene is rendered during the prepass or basepass of a graphics pipeline, certain meshes are covered or occluded by other meshes and do not contribute to the final image but their triangles are still processed. Occlusion culling is a technique that
calculates which meshes are covered and skips rendering them. The difficulty faced by occlusion culling techniques is obtaining an initial list of occluders (meshes that occlude other meshes), some engines use the previous frames visible meshes as occluders for the current frame, other engines have artist tagged occluder meshes that are used every frame.

This tool, given a scenes mesh data calculates and ranks meshes based on a occluder score. The score is calculated based on a heuristic derived from triangle cluster refinements of the mesh.

## Requirements

CMake (3.13)

Windows 10 SDK (10.0.18362.0)

## Building

The project is written in C++ (17) with DirectX 12 and Win32 API. It can be built using CMake or directly opened in Visual Studio provided you have CMake integration.

## Features

The project currently supports the following:

* Obj file format with valid mtl file
* A graph viewer to view statistics of the scenes meshes (Distribution of the length scale or occluder score of the scene)
* Ability to mutate the scenes distribution so that best occluders are selected
* View occluders selected and write their ID's to a file

## Upcoming Features

* Occlusion efficiency score of the currently selected occluders as you traverse the scene.

## Credits

Uses the following third party software:

* meshoptimizer. Copyright (c) 2016-2026, Arseny Kapoulkine

* DirectXTex. Copyright (c) Microsoft Corporation

* DirectXShaderCompiler. Copyright (c) 2003-2015 University of Illinois at Urbana-Champaign

* ImGui. Copyright (c) 2014-2026 Omar Cornut

* ImPlot. Copyright (c) 2020 Evan Pezent

* DirectX Headers. Copyright (c) Microsoft Corporation

## License

This tool is available to anybody free of charge, under the terms of [MIT License](LICENSE).
