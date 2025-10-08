Test that you can build the Altair 8800 project.

1. From a terminal window, go to the **Altair-8800-Emulator/src** folder that you cloned to your computer.
2. Run the following commands to compile the Altair project:

    ```bash
    mkdir -p build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    cmake --build . --target all -j"$(nproc)"
    ```

3. Check the build completion message to confirm a successful build. The build completion message will be similar to `[100%] Built target serializer`. If the build process fails, check that you installed all the required packages.
