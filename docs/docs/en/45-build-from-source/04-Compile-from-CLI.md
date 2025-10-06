Test that you can build the Altair 8800 project.

1. From a terminal window, go to the **Altair-8800-Emulator/src** folder that you cloned to your computer.
2. Run the following commands to compile the Altair project:

    ```bash
    mkdir -p build && \
    cmake -B build && \
    cmake --build build --config release --target all -j 4
    ```

3. Check the build completion message to confirm a successful build. The build completion message will be similar to `[100%] Built target serializer`. If the build process fails, check that you installed all the required packages.
