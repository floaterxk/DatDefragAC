# DatDefragAC
A defragmentation tool for Asheron's Call .DAT files.

# Compatibility
This tool was updated to work with Asheron's Call: Throne of Destiny. Tested with the client_portal.dat client_cell_1.dat files.

# Summary
The Asheron's Call: Throne of Destiny .DAT files are large files that consist of many embedded objects within them. These embedded objects may be added, removed, or modified with game patches to update or change content. As a result of these changes, these .DAT files can be become "fragmented" and less than optimized.

This tool will open the .DAT files from Asheron's Call: Throne of Destiny and place all index information about the embedded objects at the beginning of the files, and make all game object data contiguous throughout the file ("defragmented"). On hard disk drives in particular, this can impact disk performance significantly.
