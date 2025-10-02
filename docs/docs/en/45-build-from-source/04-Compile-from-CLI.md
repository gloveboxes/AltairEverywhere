---
sidebar_position: 5
---

# Build from the command line

Test that you can build the Altair 8800 project.

1. From a terminal window, go to the **AltairEverywhere/AltairHL_emulator** folder that you cloned to your computer.
1. If you are have a Raspberry Pi Sense HAT, then enable the Pi Sense HAT in the CMakeLists.txt file. Uncomment:

    ```cmake
    # set(ALTAIR_FRONT_PI_SENSE_HAT TRUE "Enable Raspberry Pi Sense HAT")
    ```

1. Run the following commands to compile the Altair project:

    ```bash
    mkdir -p build && \
    cmake -B build && \
    cmake --build build --config release --target all -j 4
    ```

1. Check the build completion message to confirm a successful build. The build completion message will be similar to `[100%] Built target serializer`. If the build process fails, check that you installed all the required packages.
