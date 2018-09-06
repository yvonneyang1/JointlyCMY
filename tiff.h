/************************************************************
 *  author: George Kerby
 *  This file to replace the tiff.h.
 *  The new code uses the SGI TIFF library.
 *************************************************************/

#include "tiffio.h"
#include <stdint.h>

/* other useful defs */
#define	ERROR		1
#define	NO_ERROR	0

typedef struct {
  int			height;
  int			width;
  char			TIFF_type;  	/* 'g' = grayscale;               */
								/* 'p' = palette-color;           */
								/* 'c' = RGB full color           */

  uint8_t		**mono;			/* monochrome data, or indices    */
								/* into color-map; indexed as     */
								/* mono[row][col]                 */

  uint8_t		***color;		/* full-color RGB data; indexed   */
								/* as color[plane][row][col],     */
								/* with planes 0, 1, 2 being red, */
								/* green, and blue, respectively  */

  char 			compress_type;	/* 'u' = uncompressed             */

  uint8_t		**cmap;			/* for palette-color images;      */
								/* for writing, this array MUST   */
								/* have been allocated with       */
								/* height=256 and width=3         */
} TIFF_img;


/* For the following routines: 1 = error reading file; 0 = success  */
int read_TIFF(
  char *fp,
  TIFF_img *img);

int write_TIFF(
  char *fp,
  TIFF_img *img);

int get_TIFF(
  TIFF_img *img,
  int height,
  int width,
  char TIFF_type);

void free_TIFF(
  TIFF_img *img);
