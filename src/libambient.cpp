/**
 * LibAmbient - A library for calculating the ambient color of your screen.
 * 
 * While this library targets windows machines only,
 * it is possible to extend support for other operating systems in the future.
 * 
 * (c) 2020 Kraus David. All rights reserved.
 * 
 */

#include "libambient.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <tchar.h>
#include <Windows.h>

// Functions to convert between color spaces
void    RGBtoHSB(int r, int g, int b, float* dest);
COLOR   HSBtoRGB(float hue, float saturation, float brightness);

// Some global variables and buffers which are often used,
// so we allocate them once to increase performance.
int     g_bitmapWidth, g_bitmapHeight;
int     g_screenWidth, g_screenHeight;

// Array with HUE_RANGE (360) slots (1 for each hue, rounded),
// containing the occurrences of the hue.
#define HUE_RANGE 360 // Max hue in degrees
int*        g_hues;

// A buffer containing the color values of each pixel
// contained in the below bitmap.
// using ::GetDIBits()
COLORREF*   g_pixelBuffer; 

//Bitmap containing a snapshot of the screen
HBITMAP             g_hBitmap;
BITMAPINFOHEADER    g_bitmapInfoHeader = { 0 };
HDC                 g_hScreenDC;
HDC                 g_hMemoryDC;

void clear_buffers()
{
    memset(g_hues, 0, HUE_RANGE * sizeof(int));
}

/**
 * Initializes the ambient library.
 * 
 * This function allocates the required amount of 
 * memory depending on the specified screen size.
 * 
 * @param screenWidth The width of screen
 * @param screenHeight The height of the screen
 * @param bitmapWidth The width of the internal buffer containing the taken screenshot
 * @param bitmapWidth The height of the internal buffer containing the taken screenshot
 * 
 * Note:    Using a lower resolution for the bitmap will result in a better performance,
 *          as less points have to be sampled.
 */
AMBIENT_API void initialize(int screenWidth, int screenHeight, int bitmapWidth, int bitmapHeight)
{
    dbgInfo("Initializing Ambient Library ...");
    g_screenWidth       = screenWidth;
    g_screenHeight      = screenHeight;
    g_bitmapWidth       = bitmapWidth;
    g_bitmapHeight      = bitmapHeight;

    dbgInfo("==> Allocating resources ...")
    g_hues          = (int*)        malloc(HUE_RANGE * sizeof(int));
    g_pixelBuffer   = (COLORREF*)   malloc(bitmapWidth * bitmapHeight * sizeof(COLORREF));
    clear_buffers();

    dbgInfo("==> Preparing capture ...");
    g_hScreenDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
    g_hMemoryDC = CreateCompatibleDC(g_hScreenDC);
    g_hBitmap   = CreateCompatibleBitmap(g_hScreenDC, screenWidth, screenHeight);

    BITMAP bm;
    GetObject(g_hBitmap, sizeof(bm), &bm);
    g_bitmapInfoHeader.biSize           = sizeof(BITMAPINFOHEADER);
    g_bitmapInfoHeader.biPlanes         = bm.bmPlanes;
    g_bitmapInfoHeader.biBitCount       = bm.bmBitsPixel;
    g_bitmapInfoHeader.biWidth          = bitmapWidth;
    g_bitmapInfoHeader.biHeight         = bitmapHeight;
    g_bitmapInfoHeader.biCompression    = BI_RGB;
    g_bitmapInfoHeader.biSizeImage      = 0;

    dbgInfo("==> Done");
}

/**
 * Uninitializes the ambient library.
 * 
 * This function frees all previously allocated resources.
 */
AMBIENT_API void uninitialize()
{
    dbgInfo("Uninitializing Ambient Library ...");
    dbgInfo("==> Deallocating resources ...");

    free(g_hues);
    free(g_pixelBuffer);
    DeleteDC(g_hMemoryDC);
    DeleteDC(g_hScreenDC);
    dbgInfo("==> Done");
}

/**
 * Returns the current hue of the screen.
 * 
 * This function takes a screenshot of the entire screen and
 * determines the dominant hue by counting it's occurrences.
 * 
 * The returned hue can then be used to calculate a color
 * using the HSBtoRGB function.
 * 
 * Note:    It is advised to execute this function
 *          in a separate thread as it will most certainly block.
 */
AMBIENT_API HUE getAmbientScreenHue()
{
    HBITMAP hOldBitmap = (HBITMAP) SelectObject(g_hMemoryDC, g_hBitmap);

    // Specify the resize mode.
    ::SetStretchBltMode(g_hMemoryDC, HALFTONE);

    // Copy and resize the image to a memory buffer
    StretchBlt(g_hMemoryDC, 0, 0, g_bitmapWidth, g_bitmapHeight, g_hScreenDC,
        0, g_screenHeight, g_screenWidth, -g_screenHeight, SRCCOPY);

    GetDIBits(g_hMemoryDC, g_hBitmap, 0, g_bitmapHeight, g_pixelBuffer,
        (BITMAPINFO*) &g_bitmapInfoHeader, DIB_RGB_COLORS);
    
    //Sample the screen quad
    long rSum = 0, gSum = 0, bSum = 0;
    for (int y = 0; y < g_bitmapHeight; y++)
    {
        for (int x = 0; x < g_bitmapWidth; x++)
        {
            COLORREF pixelColor = g_pixelBuffer[y * g_bitmapWidth + x];
            rSum += GetRValue(pixelColor);
            gSum += GetGValue(pixelColor);
            bSum += GetBValue(pixelColor);
        }
    }

    //Average result
    int pixelCount = g_bitmapWidth * g_bitmapHeight;
    rSum /= pixelCount;
    gSum /= pixelCount;
    bSum /= pixelCount;

    // Convert color to HSB to set the saturation
    // and brightness of the color to 100%
    float hsb[3];
    RGBtoHSB(bSum, gSum, rSum, hsb);

    //Clean up
    clear_buffers();

    return hsb[0];
}

/**
 * This function takes in three values ranging from 0 to 255 (red, green and blue)
 * and stores the values for hue, saturation and brightness inside the specified buffer.
 */
void RGBtoHSB(int r, int g, int b, float* dest)
{
    float hue, saturation, brightness;
    int cmax = (r > g) ? r : g;
    if (b > cmax) cmax = b;
    int cmin = (r < g) ? r : g;
    if (b < cmin) cmin = b;

    brightness = ((float) cmax) / 255.0f;

    if (cmax != 0)  saturation = ((float) (cmax - cmin)) / ((float) cmax);
    else            saturation = 0;

    if (saturation == 0)
    {
        hue = 0;
    }
    else
    {
        float redc      = ((float) (cmax - r)) / ((float)(cmax - cmin));
        float greenc    = ((float) (cmax - g)) / ((float)(cmax - cmin));
        float bluec     = ((float) (cmax - b)) / ((float)(cmax - cmin));

        if      (r == cmax) hue = bluec - greenc;
        else if (g == cmax) hue = 2.0f + redc - bluec;
        else                hue = 4.0f + greenc - redc;

        hue /= 6.0f;
        if (hue < 0) hue += 1.0f;
    }
    
    dest[0] = hue;
    dest[1] = saturation;
    dest[2] = brightness;
}

/**
 * This function takes in three values for hue, saturation and brightness
 * and returns the corresponding color as red, green and blue values
 * ranging from 0 to 255.
 */
COLOR HSBtoRGB(float hue, float saturation, float brightness)
{
    int r = 0, g = 0, b = 0;
    if (saturation == 0) {
        r = g = b = (int) (brightness * 255.0f + 0.5f);
    }
    else {
        float h = (hue - (float) floor(hue)) * 6.0f;
        float f = h - (float) floor(h);
        float p = brightness * (1.0f - saturation);
        float q = brightness * (1.0f - saturation * f);
        float t = brightness * (1.0f - (saturation * (1.0f - f)));
        switch ((int) h) {
        case 0:
            r = (int) (brightness * 255.0f + 0.5f);
            g = (int) (t * 255.0f + 0.5f);
            b = (int) (p * 255.0f + 0.5f);
            break;
        case 1:
            r = (int) (q * 255.0f + 0.5f);
            g = (int) (brightness * 255.0f + 0.5f);
            b = (int) (p * 255.0f + 0.5f);
            break;
        case 2:
            r = (int) (p * 255.0f + 0.5f);
            g = (int) (brightness * 255.0f + 0.5f);
            b = (int) (t * 255.0f + 0.5f);
            break;
        case 3:
            r = (int) (p * 255.0f + 0.5f);
            g = (int) (q * 255.0f + 0.5f);
            b = (int) (brightness * 255.0f + 0.5f);
            break;
        case 4:
            r = (int) (t * 255.0f + 0.5f);
            g = (int) (p * 255.0f + 0.5f);
            b = (int) (brightness * 255.0f + 0.5f);
            break;
        case 5:
            r = (int) (brightness * 255.0f + 0.5f);
            g = (int) (p * 255.0f + 0.5f);
            b = (int) (q * 255.0f + 0.5f);
            break;
        }
    }
    return 0xff000000 | (r << 16) | (g << 8) | (b << 0);
}
