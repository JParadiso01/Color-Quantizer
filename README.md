# Color-Quantizer
A color quantizer using the windows API and stb image libraries

## How to use 
 
 ``` .\color-quantizer.exe <filepath to image> ```

Press "R" to go to the next iteration of K-means clustering.
Press "L" to create an ouptu image using the colors found by the K-means clustering.

### To compile
If you would like to build the program yourself this is the command.
``` gcc -o color-quantizer color-quantizer.c -lGdi32 -lUser32 ```