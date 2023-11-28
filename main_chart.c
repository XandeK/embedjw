#include "lwip/apps/httpd.h"
#include "lwip/ip_addr.h" // Include the lwIP IP address header
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
#include "hardware/adc.h"
#include "ssi.h"
#include "cgi.h"
#include "lwip/netif.h" // Include the lwIP network interface header

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"
#include "graphic/ezdib.h"
#include <math.h>

#define PI		( (double)3.141592654 )
#define PI2		( (double)2 * PI )
// Labels for the pie chart segments
#define LABELS_COUNT 5 // Number of labels to display

// Label texts for each side
const char *labels[LABELS_COUNT] = {
    "A",
    "B",
    "C",
    "D",
    "E"
};
const char* colorNames[] = {
    "Red",
    "Green",
    "Blue",
    "Yellow",
    "Purple"
};
int pie_graph( HEZDIMAGE x_hDib, int x, int y, int rad,
			   int nDataType, void *pData, int nDataSize, int *pCols, int nCols )
{
	int i, c;
	double v, pos, dMin, dMax, dTotal;

	// Sanity checks
	if ( !pData || 0 >= nDataSize || !pCols || !nCols )
		return 0;

	// Draw chart outline
	ezd_circle( x_hDib, x, y, rad, *pCols );

	// Get the range of the data set
	ezd_calc_range( nDataType, pData, nDataSize, &dMin, &dMax, &dTotal );

	// Draw the pie slices
	pos = 0; c = 0;
	ezd_line( x_hDib, x, y, x + rad, y, *pCols );
	for ( i = 0; i < nDataSize; i++ )
	{
		if ( ++c >= nCols )
			c = 1;

		// Get the value for this element
		v = ezd_scale_value( i, nDataType, pData, 0, dTotal, 0, PI2 );

		ezd_line( x_hDib, x, y,
						  x + (int)( (double)rad * cos( pos + v ) ),
						  y + (int)( (double)rad * sin( pos + v ) ),
						  *pCols );

		ezd_flood_fill( x_hDib, x + (int)( (double)rad / (double)2 * cos( pos + v / 2 ) ),
								y + (int)( (double)rad / (double)2 * sin( pos + v / 2 ) ),
								*pCols, pCols[ c ] );

		pos += v;

	} // end for

	return 1;
}

int bar_graph(HEZDIMAGE x_hDib, int x1, int y1, int x2, int y2,
              int nDataType, void *pData, int nDataSize, int *pCols, int nCols)
{
    int i, c, bw;
    double v, dMin, dMax, dRMin, dRMax;

    if (!pData || 0 >= nDataSize || !pCols || !nCols)
        return 0;

    ezd_calc_range(nDataType, pData, nDataSize, &dMin, &dMax, 0);

    dRMin = dMin - (dMax - dMin) / 10;
    dRMax = dMax + (dMax - dMin) / 10;

    ezd_line(x_hDib, x1, y1, x2, y1, *pCols);  // Draw baseline from top to bottom

    bw = (x2 - x1 - nDataSize * 2) / nDataSize;

    c = 0;
    for (i = 0; i < nDataSize; i++)
    {
        if (++c >= nCols)
            c = 1;

        v = ezd_scale_value(i, nDataType, pData, dRMin, dRMax - dRMin, 0, y2 - y1 - 2);

        ezd_fill_rect(x_hDib, x1 + i + (bw + 1) * i, y1,  // Adjust the y-coordinate
                      x1 + i + (bw + 1) * i + bw, y1 + (int)v, pCols[c]);

        ezd_rect(x_hDib, x1 + i + (bw + 1) * i, y1,        // Adjust the y-coordinate
                 x1 + i + (bw + 1) * i + bw, y1 + (int)v, *pCols);
    }

    return 1;
}

// WIFI Credentials - take care if pushing to github!
const char WIFI_SSID[] = "";
const char WIFI_PASSWORD[] = "";

int main()
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    int ret;
    char buf[100];
    char filename[] = "test02.txt";

    // Initialize chosen serial port
    stdio_init_all();
    sleep_ms(10000);

    // test codes for creating image
    int width = 100; // Width of the image in pixels
    int height = 100; // Height of the image in pixels
    
     // Draw some text
    HEZDFONT hFont = ezd_load_font( EZD_FONT_TYPE_SMALL, 0, 0 );

    HEZDIMAGE hDib = ezd_create(width, height, 24, 0); // 24 bits per pixel for RGB
    if (!hDib) {
        fprintf(stderr, "Failed to create image\n");
        return 1;
    }

    // Fill in the background
    ezd_fill( hDib, 0x000000 );

    // Draw graphs
    
    int centerX = width / 2;
    int centerY = height / 2;
    int radius = (width < height ? width : height) / 3;

    int data[] = {1, 2, 3,4,5}; // Example data: 4 segments totaling 100%
    int dataCount = sizeof(data) / sizeof(data[0]);
    printf("%d",dataCount);
    int colors[] = {0xFFFFFF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xf403fc}; // Example colors: Red, Green, Blue, Yellow, Purple

    // Draw the pie chart using the pie_graph function
    pie_graph(hDib, centerX, centerY, radius, EZD_TYPE_INT, data, sizeof(data) / sizeof(data[0]), colors, dataCount+1);
    
    // Angles for positioning the labels (in radians) - these angles correspond to the midpoints of the pie chart segments
    float labelAngles[LABELS_COUNT] = {0}; // Initialize the labelAngles array

    // Radius for the labels (distance from the center of the pie chart)
    int labelRadius = radius + 20; // Adjust the distance as needed

    // Calculate total for adjusting percentages to sum up to 100
    int total = 0;
    for (int i = 0; i < LABELS_COUNT; ++i) {
        total += data[i];
    }
    // Calculate percentages for the segments
    float segmentPercentages[LABELS_COUNT] = {0};
    for (int i = 0; i < LABELS_COUNT; ++i) {
        segmentPercentages[i] = (float)data[i] / total * 100;
    }

    // Calculate angles for label positions based on segment percentages
    float angleSum = 0;
    for (int i = 0; i < LABELS_COUNT; ++i) {
        angleSum += (float)segmentPercentages[i] / 100 * PI2;
        labelAngles[i] = angleSum - ((float)segmentPercentages[i] / 100 * PI);
    }
        
    // Add labels to the image at calculated angles
    for (int i = 0; i < LABELS_COUNT; ++i) {
        int labelX = centerX + (int)((radius + labelRadius) / 2 * cos(labelAngles[i])); // X-coordinate calculation
        int labelY = centerY + (int)((radius + labelRadius) / 2 * sin(labelAngles[i])); // Y-coordinate calculation
        ezd_text(hDib, hFont, labels[i], -1, labelX, labelY, 0xFFFFFF); // Draw label on the image
    }

    // Initialize SD card
    if (!sd_init_driver())
    {
        printf("ERROR: Could not initialize SD card\r\n");
        while (true)
            ;
    }

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK)
    {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        while (true)
            ;
    }

    // Open file for writing ()
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true)
            ;
    }

    // Write something to file
    for(int i =0; i <LABELS_COUNT; ++i)
    {
        
        ret = f_printf(&fil, "Color : %s\r\n", colorNames[i]);
        if (ret < 0)
        {
            printf("ERROR: Could not write to file (%d)\r\n", ret);
            f_close(&fil);
            while (true)
                ;
        }
        ret = f_printf(&fil, "Label %s : %d\r\n", labels[i], data[i]);
        if (ret < 0) {
            printf("ERROR: Could not write to file (%d)\r\n", ret);
            f_close(&fil);
            while (true)
                ;
        }
        ret = f_printf(&fil, "01:23 Button pressed!\n");
    }

    ezd_save(hDib, "output.bmp");
    
    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK)
    {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        while (true)
            ;
    }
    ezd_destroy(hDib);
    ezd_destroy_font( hFont );
    
    // Open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK)
    {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        while (true)
            ;
    }

    // Print every line in file over serial
    printf("Reading from file '%s':\r\n", filename);
    printf("---\r\n");
    while (f_gets(buf, sizeof(buf), &fil))
    {
        printf(buf);
    }
    printf("\r\n---\r\n");

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK)
    {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        while (true)
            ;
    }
    // Draw the bar graph using the bar_graph function
    int barData[] = {10, 20, 30, 40, 50}; // Example bar graph data
    int barDataCount = sizeof(barData) / sizeof(barData[0]);
    int barColors[] = {0xFFFFFF,0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xF403FC}; // Example bar colors
    ezd_rect( hDib, 35, 295, 605, 445, barColors[ 0 ] );
    int barGraphX1 = 10;
    int barGraphY1 = 10;
    int barGraphX2 = width - 10;
    int barGraphY2 = height - 20;
   
    HEZDIMAGE hBarDib = ezd_create(width, height, 24, 0);
    if (!hBarDib) {
        fprintf(stderr, "Failed to create bar graph image\n");
        return 1;
    }

    ezd_fill(hBarDib, 0x000000);
    bar_graph(hBarDib, barGraphX1, barGraphY1, barGraphX2, barGraphY2, EZD_TYPE_INT, barData, barDataCount, barColors, sizeof(barColors) / sizeof(barColors[0]));

    // Save the bar graph image
    ezd_save(hBarDib, "bar_graph.bmp");
    ezd_destroy(hBarDib);

    cyw43_arch_init();

    cyw43_arch_enable_sta_mode();

    // Connect to the WiFI network - loop until connected
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0)
    {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected
    // printf("Connected! \n");

    // Retrieve the default network interface
    struct netif *netif = netif_default;
    if (netif != NULL)
    {
        // Retrieve the IP address associated with this network interface
        const ip_addr_t *ipaddr = netif_ip_addr4(netif); // Get the IPv4 address of this interface
        // Print the IP address
        if (!ip_addr_isany(ipaddr)) // Check if the IP is set
        {
            printf("IP Address: %s\n", ip4addr_ntoa(ipaddr));
        }
        else
        {
            printf("No IP Address assigned!\n");
        }
    }
    else
    {
        printf("No default network interface!\n");
    }

    // Initialise web server
    httpd_init();
    printf("Http server initialised\n");

    // Configure SSI and CGI handler
    ssi_init();
    printf("SSI Handler initialised\n");
    cgi_init();
    printf("CGI Handler initialised\n");

    // Infinite loop
    while (1)
        ;
}