#include "vectorize.h"

#include <math.h>
#include <stdlib.h>
#include "types/colour.h"
#include "tools.h"
#include <errno.h>

#ifndef NULL
#define NULL 0
#endif

void iterateImagePixels(int x, int y, image inputimage, node_map_options options, group_map output) {
    int x_offset = x * options.chunk_size;
    int y_offset = y * options.chunk_size;

    // Grab the pixelgroup
    DEBUG_PRINT("Accessing Nodes... \n");
    pixelgroup *outputnodes = &output.nodes[x + y * output.width];

    // Assigned the edge case pixelgroup dimensions
    int node_width = inputimage.width - x * options.chunk_size;
    int node_height = inputimage.height - y * options.chunk_size;
    
    // Check if not actually on the edge
    if (node_width > options.chunk_size)
        node_width = options.chunk_size;
    if (node_height > options.chunk_size)
        node_height = options.chunk_size;

    int count = node_width * node_height;
    
    // Gather all the pixels into this array
    colorf **node_data = malloc(sizeof(colorf*) * node_width);
    
    for (int i = 0; i < node_height; ++i)
        node_data[i] = malloc(sizeof(colorf) * node_height);
    DEBUG_PRINT("Iterating the Image Pixels... \n");

    for (int width_index = 0; width_index < node_width; ++width_index)
    {
        for (int height_index = 0; height_index < node_height; ++height_index)
        {
            int pixel_x = x * options.chunk_size + width_index;
            int pixel_y = y * options.chunk_size + height_index;

            DEBUG_PRINT("accessing image pixels... \n");
            float r = inputimage.pixels[pixel_x][pixel_y].r;
            float g = inputimage.pixels[pixel_x][pixel_y].g;
            float b = inputimage.pixels[pixel_x][pixel_y].b;

            DEBUG_PRINT("indexing node_data... \n");
            node_data[width_index][height_index].r = r;
            node_data[width_index][height_index].g = g;
            node_data[width_index][height_index].b = b;
        }
    }


    // Calculate the average of all these pixels
    colorf average = { 0.f, 0.f, 0.f };
    int average_r = 0, average_g = 0, average_b = 0;
    
    // Also calculate the Minimum and Maximum 'colors' (values of each color)
    DEBUG_PRINT("Generating Average \n");
    pixel min = { 255, 255, 255 }, max = { 0, 0, 0 };

    for (int x = 0; x < node_width; ++x)
    {
        for (int y = 0; y < node_height; ++y)
        {
            pixel *p = &inputimage.pixels[y_offset + y][x_offset + x];
            average_r += p->r;
            average_g += p->g;
            average_b += p->b;
            if (p->r < min.r)
                min.r = p->r;
            if (p->g < min.g)
                min.g = p->g;
            if (p->b < min.b)
                min.b = p->b;
            if (p->r > max.r)
                max.r = p->r;
            if (p->g > max.g)
                max.g = p->g;
            if (p->b > max.b)
                max.b = p->b;
        }
    }

    pixel average_p = { 
        (byte)((float)average_r / (float)count), 
        (byte)((float)average_g / (float)count), 
        (byte)((float)average_b / (float)count) 
    };
    outputnodes->color = average_p;

    DEBUG_PRINT("Accumulating Colors for Variance Calculation \n");
    pixel *node_pixels = malloc(sizeof(pixel) * node_width * node_height);
    for (int x = 0; x < node_width; ++x)
    {
        for (int y = 0; y < node_height; ++y)
        {
            node_pixels[x + y * node_width] = inputimage.pixels[y_offset + y][x_offset + x];
        }
    }
    DEBUG_PRINT("Calculating Color Variance \n");
    outputnodes->variance = calculate_pixel_variance(node_pixels, node_width * node_height);

    //only print 
    if ((x == y && x % 20 == 0) || (x == 0 && y == 0) || (x == (output.width - 1) && y == (output.height - 1)))
    {
        DEBUG_PRINT("pixelgroup (%d, %d) variance: (%g, %g, %g), average: (%d, %d, %d), node_width: %d, node_height %d, min: %d, %d, %d, max: %d, %d, %d\n", 
        x, y, 
        outputnodes->variance.r,
        outputnodes->variance.g,
        outputnodes->variance.b, 
        outputnodes->color.r, 
        outputnodes->color.g, 
        outputnodes->color.b, 
        node_width, 
        node_height, 
        min.r, min.g, min.b, max.r, max.g, max.b);
    }
    DEBUG_PRINT("pixelgroup complete \n");
}

group_map generate_group_map(image inputimage, node_map_options options)
{
    if (!inputimage.pixels)
    {
        DEBUG_PRINT("Invalid image input \n");
        return (group_map){ NULL, 0, 0 };
    }

    if (inputimage.width < 1 || inputimage.height < 1 || !inputimage.pixels)
        return (group_map){ NULL, 0, 0 };

    DEBUG_PRINT("Begin Generation of pixelgroup Map\n");
    if (options.chunk_size < 2)
        options.chunk_size = 2;
    
    group_map output;
    output.width = (int)ceilf((float)inputimage.width / (float)options.chunk_size);
    output.height = (int)ceilf((float)inputimage.height / (float)options.chunk_size);

    DEBUG_PRINT("Allocating pixelgroup map with %dx%d\n", output.width, output.height);
    output.nodes = malloc(sizeof(pixelgroup) * output.width * output.height);

    DEBUG_PRINT("Iterating through the nodes\n");
    for (int x = 0; x < output.width; ++x)
    {
        for (int y = 0; y < output.height; ++y)
        {
            iterateImagePixels(x, y, inputimage, options, output);
        }
    }

    return output;
}

node_variance calculate_pixel_variance(pixel *pixels, int num_pixels)
{
    if (num_pixels < 2)
        return (node_variance){0.f, 0.f, 0.f};

    double Kr = pixels[0].r;
    double Kg = pixels[0].g;
    double Kb = pixels[0].b;
    int n = 0;
    double Exr = 0.f, Ex2r = 0.f;
    double Exg = 0.f, Ex2g = 0.f;
    double Exb = 0.f, Ex2b = 0.f;

    for (int i = 0; i < num_pixels; ++i)
    {
        ++n;
        colorf col = convert_pixel_to_colorf(pixels[i]);
        Exr += (double)col.r - Kr;
        Exg += (double)col.g - Kg;
        Exb += (double)col.b - Kb;
        Ex2r += ((double)col.r - Kr) * ((double)col.r - Kr);
        Ex2g += ((double)col.g - Kg) * ((double)col.g - Kg);
        Ex2b += ((double)col.b - Kb) * ((double)col.b - Kb);
    }
    node_variance variance;
    variance.r = (float)((Ex2r - (Exr * Exr) / (double)n) / (double)(n - 1));
    variance.g = (float)((Ex2g - (Exg * Exg) / (double)n) / (double)(n - 1));
    variance.b = (float)((Ex2b - (Exb * Exb) / (double)n) / (double)(n - 1));
    if (fabsf(variance.r) < 0.000000001)
        variance.r = 0.f;
    if (fabsf(variance.g) < 0.000000001)
        variance.g = 0.f;
    if (fabsf(variance.b) < 0.000000001)
        variance.b = 0.f;
    return variance;
}

float shifted_data_variance(float *data, int data_len)
{
    if (data_len < 2)
        return 0.f;
    
    float K = data[0];
    int n = 0;
    float Ex = 0.f, Ex2 = 0.f;

    for (int i = 0; i < data_len; ++i)
    {
        ++n;
        Ex += data[i] - K;
        Ex2 += (data[i] - K) * (data[i] - K);
    }
    float variance = (Ex2 - (Ex * Ex) / (float)n) / (float)(n - 1);

    return variance;
}
