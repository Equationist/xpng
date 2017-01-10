# XPNG
Program to lossily compress PNG images using adaptive quantization.

## About
This code preprocesses PNG images by making small changes to the images to make them easier to compress. The program uses adaptive quantization based on the local noisiness of the source image, taking advantage of noise masking. It is designed for the "Average" predictor in PNG - various predictors were tried, but "Average" worked out best as it smeared out any effects of quantization. The program quantizes to the highest power of 2 within the allowable noise margin, unless a recent value was used that also results in output within the allowable noise margin (to make use of DEFLATE's runlength encoding).

The nonlinear formula for allowable error at each compression and noise level was chosen based on careful trial and error and hand tuning for optimal visual perceptual effect.

The program ended up being similar to [lossypng](https://github.com/foobaz/lossypng), but the adaptive quantization used in xpng makes it better at low compression levels (achieving compression ratios of 5-10 compared to raw without significant visual artifacts, as shown in the sample images below). This minimal-artifact compression ratio of 5-10 for XPNG compares with roughly 15-20 for JPEG. However, XPNG performs much better than JPEG on images containing a mixture of photography and text or line art.

## Usage
`xpng inputfile.png outputfile.png compressionlevel` (compression level is a number from 0-40, with a typical value of 6)

Apply a png optimizer afterwards for full compression.

## Compiling
Requires libpng and zlib.

Compile with `gcc main.c -o xpng -lpng`

## Sample Images
Here is a diverse suite of test images fromm the [Kodak Image Suite](http://r0k.us/graphics/kodak/). Both original and compressed versions were run through PNGGauntlet for an optimized apples to apples comparison. A comprssion level of 6 was used. The compressed image follows the original.

### Original: 678 KB - Compressed: 187 KB (72% Compression)
![kodim01](https://cloud.githubusercontent.com/assets/8397050/21791560/6d455276-d698-11e6-8101-8533ebcf7249.png)
![k1](https://cloud.githubusercontent.com/assets/8397050/21791567/6d6cd7ec-d698-11e6-89ff-9231a28c48f3.png)

### Original: 594 KB - Compressed: 130 KB (78% Compression)
![kodim02](https://cloud.githubusercontent.com/assets/8397050/21791559/6d43cbb8-d698-11e6-9104-45c011d5416d.png)
![k2](https://cloud.githubusercontent.com/assets/8397050/21791566/6d6b0ce6-d698-11e6-9c4a-a8c2f419ccd3.png)

### Original: 742 KB - Compressed: 198 KB (73% Compression)
![kodim05](https://cloud.githubusercontent.com/assets/8397050/21791561/6d46d9fc-d698-11e6-8fbf-b1276c061c9d.png)
![k5](https://cloud.githubusercontent.com/assets/8397050/21791568/6d72ad48-d698-11e6-9b31-769daccd1bc6.png)

### Original: 590 KB - Compressed: 150 KB (75% Compression)
![kodim06](https://cloud.githubusercontent.com/assets/8397050/21791562/6d4a1504-d698-11e6-97bb-eb22d565eee2.png)
![k6](https://cloud.githubusercontent.com/assets/8397050/21791569/6d7e6124-d698-11e6-8e4e-868b4c0245c0.png)

### Original: 785 KB - Compressed: 224 KB (71% Compression)
![kodim13](https://cloud.githubusercontent.com/assets/8397050/21791563/6d5d94a8-d698-11e6-83ac-2ee6f77d9e13.jpg)
![k13](https://cloud.githubusercontent.com/assets/8397050/21791557/6d3b7472-d698-11e6-814e-24697ee57041.png)

### Original: 674 KB - Compressed: 199 KB (70% Compression)
![kodim22](https://cloud.githubusercontent.com/assets/8397050/21791565/6d66d702-d698-11e6-88ea-7c199ab33f90.png)
![k22](https://cloud.githubusercontent.com/assets/8397050/21791570/6d99ff4c-d698-11e6-87d3-c265e5ad6677.png)
