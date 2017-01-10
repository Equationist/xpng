/* gcc main.c -o xpng -lpng */
/* Requires libpng and zlib */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <png.h>
#include <zlib.h>
#include <math.h>
#include <stdint.h>

#define BLOCKSIZE 3

static int32_t h;
static int32_t w;

/* Returns pointer to a certain address in a 24 bit datafield */
png_bytep pix(int32_t i, int32_t j, int c, png_bytep data)
{
    return &(data[i*w*3+j*3+c]);
}

/* Returns the difference between two bytes for error calculation */
uint8_t err(png_byte x1, png_byte x2)
{
    uint8_t a1 = x1; uint8_t a2 = x2;
    return a1 > a2 ? a1 - a2 : a2 - a1;
}

/* Average predictor for PNG */
png_byte avg(int32_t i, int32_t j, int c, png_bytep data)
{
    png_byte left = 0;
    png_byte up = 0;
    
    if (i > 0) up = *pix(i-1,j,c,data);
    if (j > 0) left = *pix(i,j-1,c,data);

    int16_t p = up + left;
    return p/2;
}

/* Top predictor for PNG - unused */
png_byte top(int32_t i, int32_t j, int c, png_bytep data)
{
    png_byte up = 0;
    if (i > 0) up = *pix(i-1,j,c,data);
    return up;
}

/* Left predictor for PNG - used in first row */
png_byte sub (int32_t i, int32_t j, int c, png_bytep data)
{
    png_byte left = 0;
    if (j > 0) left = *pix(i,j-1,c,data);
    return left;
}

/* Paeth predictor for PNG - unused */
png_byte paeth(int32_t i, int32_t j, int c, png_bytep data)
{
    png_byte left = 0;
    png_byte up = 0;
    png_byte topleft = 0;
    
    if (i > 0) up = *pix(i-1,j,c,data);
    if (j > 0) left = *pix(i,j-1,c,data);
    if (i > 0 && j > 0) topleft = *pix(i-1,j-1,c,data);

    int16_t p = left;
    p += up;
    p -= topleft;
    if (p < 0) p = 0;
    if (p > 255) p = 255;
    
    uint8_t dist = 0;
    png_byte base;

    base = left;
    dist = err((png_byte) p, left);

    if (err((png_byte) p, up) < dist)
    {
        dist = err((png_byte) p, up);
        base = up;
    }

    if (err((png_byte) p, topleft) < dist)
    {
        dist = err((png_byte) p, topleft);
        base = topleft;
    }

    return base;
}

/* Calculates noisiness around a certain location - used for adaptive quantization */
png_byte calc_noise(int32_t i, int32_t j, int32_t c, png_bytep data)
{
    int32_t N = 0;
    int64_t X = 0;
    int64_t X2 = 0;
    for (int32_t ii = i - BLOCKSIZE / 2; ii <= i + BLOCKSIZE / 2; ii++)
    {
        for (int32_t jj = j - BLOCKSIZE / 2; jj <= j + BLOCKSIZE / 2; jj++)
        {
            if (ii >= 0 && ii < h && jj >= 0 && jj < w)
            {
                X += *pix(ii, jj, c, data);
                X2 += *pix(ii, jj, c, data) *
                     *pix(ii, jj, c, data);
                N += 1;
            }
        }
    }
    return sqrt((X2 - X*X / N) / (N-1));
}

int main(int argc, const char **argv)
{
    if (argc != 4)
	{
		fprintf(stderr, "xpng: usage: xpng inputfile outputfile level\n");
		exit(1);
	}
	
	unsigned int clevel = atoi(argv[3]);
	png_image image; /* The control structure used by libpng */

	/* Initialize png_image structure */
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;

	/* The first argument is the input file */
	if (png_image_begin_read_from_file(&image, argv[1]) == 0)
	{
        fprintf(stderr, "xpng: error: %s\n", image.message);
        exit (1);
    }
    
    png_bytep buffer;
    png_bytep buffer2;
    png_bytep data;
    png_bytep diff;
    png_bytep noise;

    image.format = PNG_FORMAT_RGB;

    buffer = malloc(PNG_IMAGE_SIZE(image));
    buffer2 = malloc(PNG_IMAGE_SIZE(image));
    diff = malloc(PNG_IMAGE_SIZE(image));
    noise = malloc(PNG_IMAGE_SIZE(image));
    

    if (buffer == NULL || buffer2 == NULL)
    {
        fprintf(stderr, "xpng: error: insufficient memory\n");
        exit(1);
    }
    
    memset(buffer, 0, PNG_IMAGE_SIZE(image));
    memset(buffer2, 0, PNG_IMAGE_SIZE(image));

    if (buffer == NULL ||
        png_image_finish_read(&image, NULL/*bg*/, buffer,
        0/*row_stride*/, NULL/*colormap*/) == 0)
    {
        fprintf(stderr, "xpng: error: %s\n", image.message);
        exit (1);
    }
       
    h = image.height;
    w = image.width;

    for (int32_t i = 0; i < h; i++)
    {
        for (int32_t j = 0; j < w; j++)
        {
            for (int c = 0; c < 3; c++)
            {
                noise[i*w*3+j*3+c] = calc_noise(i,j,c,buffer);
            }
        }
    }

    uint64_t freq[256] = {};
    for (uint16_t jumpsize = 1; jumpsize <= 128; jumpsize*=2)
    {
        for (int16_t i = -128; i <= 128; i+=jumpsize)
        {
            freq[(uint8_t)i] = jumpsize;
        }
    }
    for (int32_t i = 0; i < h; i++)
    {
        int32_t refdist = 1;
        for (int32_t j = 0; j < w; j++)
        {
            for (int32_t c = 0; c < 3; c++)
            {
                png_byte target;
                png_byte base;
                if (i == 0 && j > 0)
                {
                    target = *pix(i,j,c,buffer);
                    base = sub(i,j,c,buffer2);
                }
                else
                {
                    target = *pix(i,j,c,buffer);
                    base = avg(i,j,c,buffer2);
                }

                uint8_t delta = clevel
                              +(uint16_t) *pix(i,j,c,noise)*clevel/(10+clevel);

                png_byte ltarget = 0;
                png_byte rtarget = 255;
                if (target > delta) ltarget = target-delta;
                if (255-target > delta) rtarget = target+delta;

                *pix(i,j,c,buffer2) = target;
                png_byte approx = target;
                uint64_t f = 0;
                png_byte a = ltarget;
                do
                {
                    uint8_t d = (png_byte)a-(png_byte)base;
                    if (freq[d] > f)
                    {
                        approx = a;
                        f = freq[d];
                    }
                } while (a++ != rtarget);

                *pix(i,j,c,diff) = approx-base;
                /*freq[*pix(i,j,c,diff)]++;*/
                *pix(i,j,c,buffer2) = approx;

                if (err(target,base+*(pix(i,j,c,diff)-refdist))
                        < delta)
                {
                    *pix(i,j,c,diff) = *(pix(i,j,c,diff)-refdist);
                    *pix(i,j,c,buffer2) = base 
                                        + *pix(i,j,c,diff);
                    continue;
                }
                for (size_t rd=1; rd<=j*3+c && rd<=4;rd++)
                {
                    if (err(target,base+*(pix(i,j,c,diff)-rd))
                        < delta)
                    {
                        refdist = rd;
                        *pix(i,j,c,diff) = *(pix(i,j,c,diff)-rd);
                        *pix(i,j,c,buffer2) = base 
                                            + *pix(i,j,c,diff);
                        break;
                    }
                }
            }
        }
    }
    if (png_image_write_to_file(&image, argv[2], 0/*already_8bit*/,
        buffer2, 0/*row_stride*/, NULL/*colormap*/) == 0)
    {
        fprintf(stderr, "xpng: error: %s\n", image.message);
        exit (1);
    }
    exit(0);
}

    
