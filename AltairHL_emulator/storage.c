/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "storage.h"


/// <summary>
/// Write an integer to this application's persistent data file
/// </summary>
void write_channel_id_to_storage(int value) {
    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        Log_Debug("ERROR: Could not open mutable file:  %s (%d).\n", strerror(errno), errno);
        return;
    }
    
    ssize_t ret = pwrite(fd, &value, sizeof(value), 0);
    if (ret == -1) {
        // If the file has reached the maximum size specified in the application manifest,
        // then -1 will be returned with errno EDQUOT (122)
        Log_Debug("ERROR: An error occurred while writing to mutable file:  %s (%d).\n",
            strerror(errno), errno);
    } else if (ret < sizeof(value)) {
        // For simplicity, this sample logs an error here. In the general case, this should be
        // handled by retrying the write with the remaining data until all the data has been
        // written.
        Log_Debug("ERROR: Only wrote %d of %d bytes requested\n", ret, (int)sizeof(value));
    }
    close(fd);
}

/// <summary>
/// Read an integer from this application's persistent data file
/// </summary>
/// <returns>
/// The integer that was read from the file.  If the file is empty, this returns 0.  If the storage
/// API fails, this returns -1.
/// </returns>
int read_channel_id_from_storage(void) {
    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        Log_Debug("ERROR: Could not open mutable file:  %s (%d).\n", strerror(errno), errno);
        return -1;
    }
    int value = 0;
    ssize_t ret = read(fd, &value, sizeof(value));
    if (ret == -1) {
        Log_Debug("ERROR: An error occurred while reading file:  %s (%d).\n", strerror(errno),
            errno);
    }
    close(fd);

    return ret < sizeof(value) ? -1 : value;
}