/************************************************************
 *  author: George Kerby
 *  This file to replace the tiff.c library.
 *  The new code uses the SGI TIFF library.
 *  Modified by Zhen He 6/15/01
 *************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiff.h"
#include "allocate.h"

#include <stdint.h>

int read_TIFF ( char * filename, TIFF_img *img )
{
    TIFF *intiff;
    short photo, planar, bps, spp, i, j;
    char *buff;

    TIFFSetWarningHandler (NULL);  /*  remove this line to enable stderr warnings from the SGI library  */
    
    if ((intiff = TIFFOpen (filename, "r")) == NULL) return ( ERROR );
    
    TIFFGetField (intiff, TIFFTAG_PHOTOMETRIC, &photo);
    TIFFGetField (intiff, TIFFTAG_PLANARCONFIG, &planar);
    TIFFGetField (intiff, TIFFTAG_BITSPERSAMPLE, &bps);
    TIFFGetField (intiff, TIFFTAG_SAMPLESPERPIXEL, &spp);
    TIFFGetField (intiff, TIFFTAG_IMAGEWIDTH, &(img->width));
    TIFFGetField (intiff, TIFFTAG_IMAGELENGTH, &(img->height));

    
    if (bps == 1) return ( ERROR );
    
    switch (photo) {
        case PHOTOMETRIC_MINISWHITE:
            if (bps == 8) img->TIFF_type = 'g';
            img->mono = ( uint8_t ** )
                get_img ( img->width, img->height, sizeof ( uint8_t ) );
            break;
        case PHOTOMETRIC_MINISBLACK:
            if (bps == 8) img->TIFF_type = 'g';
            img->mono = ( uint8_t ** )
                get_img ( img->width, img->height, sizeof ( uint8_t ) );
            break;
        case PHOTOMETRIC_RGB:
            img->TIFF_type = 'c';
            img->color = ( uint8_t *** )
                mget_spc ( 3, sizeof ( uint8_t ** ) );
            for ( i = 0; i < 3; i++ )
                img->color[i] = ( uint8_t ** )
                    get_img ( img->width, img->height,
                              sizeof ( uint8_t ) );
            break;
        case PHOTOMETRIC_PALETTE:
            img->TIFF_type = 'p';
            img->color = ( uint8_t *** )
                mget_spc ( 3, sizeof ( uint8_t ** ) );
            for ( i = 0; i < 3; i++ )
                img->color[i] = ( uint8_t ** )
                    get_img ( img->width, img->height,
                              sizeof ( uint8_t ) );
            break;
        default:
            return ( ERROR );
    }
    
    /*
     * read image data one strip at a time
     */
    buff = _TIFFmalloc(TIFFScanlineSize (intiff));
    for ( i = 0; i < img->height; i++ ) {
        TIFFReadScanline (intiff, buff, i, 0);
        if (img->TIFF_type == 'g') {
            memcpy (img->mono[i], buff, img->width);
            if(photo == PHOTOMETRIC_MINISWHITE)  /* enforce 0 to be black */
               for( j = 0; j<img->width; j++)
                  img->mono[i][j] = 255 -  img->mono[i][j];
            else {;}
        }
        else {
            if (planar == PLANARCONFIG_CONTIG) /*  RGBRGBRGBRGB...  */
                for (j=0; j< img->width ;j++) {
                    img->color[0][i][j] = buff[j*3];
                    img->color[1][i][j] = buff[j*3+1];
                    img->color[2][i][j] = buff[j*3+2];
                }
            else {  /*  RRRR...RRGGGG....GGBBBBBB..BB  */
                memcpy (img->color[0][i], buff, img->width);
                memcpy (img->color[1][i], &buff[img->width], img->width);
                memcpy (img->color[2][i], &buff[img->width*2], img->width);
            }
        }
    }
    _TIFFfree (buff);
    TIFFClose (intiff);
    
    return ( NO_ERROR );
}

/*
 *  New version of write_TIFF().  Uses the SGI library.
 */

int write_TIFF ( char *filename, TIFF_img *img )
{
    TIFF *out_tif;
    int i, j, size;
    char *buff;
    
    if ((out_tif = TIFFOpen (filename, "w")) == NULL) {
        return ( ERROR );
    }
    else {
        TIFFSetField (out_tif, TIFFTAG_IMAGEWIDTH, img->width);
        TIFFSetField (out_tif, TIFFTAG_IMAGELENGTH, img->height);
        switch (img->TIFF_type) {
            case 'g':
                TIFFSetField (out_tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
                TIFFSetField (out_tif, TIFFTAG_BITSPERSAMPLE, 8);
                TIFFSetField (out_tif, TIFFTAG_SAMPLESPERPIXEL, 1);
                TIFFSetField (out_tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                size = TIFFScanlineSize(out_tif);
                break;
            case 'c':
                TIFFSetField (out_tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
                TIFFSetField (out_tif, TIFFTAG_BITSPERSAMPLE, 8);
                TIFFSetField (out_tif, TIFFTAG_SAMPLESPERPIXEL, 3);
                TIFFSetField (out_tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                size = TIFFScanlineSize(out_tif) * 3;
                break;
            case 'p':
                TIFFSetField (out_tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
                TIFFSetField (out_tif, TIFFTAG_BITSPERSAMPLE, 8);
                TIFFSetField (out_tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                TIFFSetField (out_tif, TIFFTAG_SAMPLESPERPIXEL, 3);
                size = TIFFScanlineSize(out_tif) * 3;
                break;
            default:
                return ( ERROR );
        }
        TIFFSetField (out_tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        buff = _TIFFmalloc (size);
        for (i=0; i<img->height ;i++) {
            if (img->TIFF_type == 'g') {
                memcpy (buff, img->mono[i], img->width);
            }
            else {
              for (j=0; j< img->width ;j++) {
                  buff[j*3] = img->color[0][i][j];
                  buff[j*3+1] = img->color[1][i][j];
                  buff[j*3+2] = img->color[2][i][j];
              }
            }
            TIFFWriteScanline (out_tif, buff, i, 0);
        }
        _TIFFfree (buff);
        TIFFClose (out_tif);
    }
  return ( NO_ERROR );
}

int get_TIFF ( TIFF_img *img, int height, int width , char TIFF_type )
{
  int i;

  /* set height, width, TIFF_type, and compress_type */
  img->height = height;
  img->width = width;
  img->TIFF_type = TIFF_type;
  img->compress_type = 'u';

  /* check image height/width */
  if ( ( img->height <= 0 ) || ( img->width <= 0 ) ) {
    fprintf ( stderr, "tiff.c:  function get_TIFF:\n" );
    fprintf ( stderr, "image height and width must be positive\n" );
    return ( ERROR );
  }

  /* if image is grayscale */
  if ( img->TIFF_type == 'g' ) {
    img->mono = ( uint8_t ** )
                get_img ( img->width, img->height, sizeof ( uint8_t ) );
    return ( NO_ERROR );
  }

  /* if image is palette-color */
  if ( img->TIFF_type == 'p' ) {
    img->mono = ( uint8_t ** )
                get_img ( img->width, img->height, sizeof ( uint8_t ) );
    img->cmap = ( uint8_t ** ) get_img ( 3, 256, sizeof ( uint8_t ) );
    return ( NO_ERROR );
  }

  /* if image is full color */
  if ( img->TIFF_type == 'c' ) {
    img->color = ( uint8_t *** )
                 mget_spc ( 3, sizeof ( uint8_t ** ) );
    for ( i = 0; i < 3; i++ )
      img->color[i] = ( uint8_t ** )
                      get_img ( img->width, img->height,
                                sizeof ( uint8_t ) );
    return ( NO_ERROR );
  }

  fprintf ( stderr, "tiff.c:  function get_TIFF:\n" );
  fprintf ( stderr, "allocation for image of type %c is not supported", 
            img->TIFF_type );
  return ( ERROR );
}


/*
 *   free_TIFF is unmodified and must still be called to free()
 *   the memory for TIFF_img that was allocated in the new read_TIFF()
 */

void free_TIFF ( TIFF_img *img )
{
  int i;

  /* grayscale */
  if ( img->TIFF_type == 'g' ) {
    free_img ( ( void ** ) ( img->mono ) );
  }

  /* palette-color */
  if ( img->TIFF_type == 'p' ) {
    free_img ( ( void ** ) ( img->mono ) );
    free_img ( ( void ** ) ( img->cmap ) );
  }

  /* full color */
  if ( img->TIFF_type == 'c' ) {
    for ( i = 0; i < 3; i++ ) free_img ( ( void ** ) ( img->color[i] ) );
  }
}
