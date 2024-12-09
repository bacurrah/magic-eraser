## Included Folders

- **src/**
  - Contains the code related to this project.
- **images/**
  - Contains example images. Add images here to load them in the software.
- **build/**
  - Compilation binaries. See [Compiling](#compiling) for more information.

## Using the Application

1. Load an image from the list on the sidebar
2. Draw a selection onto the image using the mouse
3. Press the Generate button

### Application Options

- **Use Proxies**
  - Shows proxy images and uses the multiresolution algorithm. Disable if experiencing artifacts.
- **Live Update**
  - Updates the preview image in real time. Impacts performance.
- **Show Heatmap**
  - Highlight pixels that were selected by the algorithm. Impacts performance.

## Compiling

A precompiled binary has been provided for Windows, use `build_win64.exe`.\
For manual compilation, see [these instructions](https://github.com/raylib-extras/raylib-quickstart).

## Testing

Disregarding the initial randomness, this algorithm is deterministic.
However, as it generates images, its results are largely subjective.
This naturally makes it difficult to test and to create test cases for.\
Included with the application are 5 example images that can be loaded
and tested using the provided interface. While this is not rigorous,
it should allow for a good demonstration of the algorithm.
