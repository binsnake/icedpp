# iced++

Iced++ is a C++ wrapper around https://github.com/icedland/iced/tree/master

Iced++ does not aim to directly re-implement the Rust SDK.

Work in progress, only basic functionality is implemented so far.

`icedpp_rust_lib` contains a Rust proxy module that runs the decoder and translates them into a C-structure.
`icedpp` is a header-only library providing a friendly wrapper around the decoder & instruction classes.

The code is formatted mostly in camelCase.

# Building

Building is done with CMake.
`cmake -B build`

If you want to build with a different build system, like MSBuild, you will have to compile the Rust library with cargo, link the `Iced_Wrapper.lib` with your executable and include the `icedpp` headers.
