
/*************************************************************************/
/* Routines for reading and writing pbm, pgm, and ppm image file formats */
/* Last Update: 10/27/93                                                 */
/*************************************************************************/

#include <stdio.h>
#include <string.h>

#include  "pxm.h"
#include  "allocate.h"

#include <stdint.h>

/*************************************************************************/
/* Example calling routine                                               */
/*									 */
/* Read an image from any pbm, pgm, or ppm file.  If the file is pbm     */
/* or pgm, write the image to a Sun rasterfile.  If the file is ppm,     */
/* write the image to another ppm file (there is no 24-bit Sun format)   */
/*									 */
/* Takes two command line arguments: input file and output file		 */
/*************************************************************************/




/*************************************************************************/
/* write_pxm()                                                           */
/*  routine that takes in a binary, grayscale, or color image		 */
/*  and writes it out in pbm, pgm, or ppm format. It returns a           */
/*  value 1 if it was not able to write out the data. Otherwise, it      */
/*  returns 0. 							         */
/*************************************************************************/

int write_pxm (FILE *fout, struct pxm_img *img)

{
  int			i, j;
  uint8_t		junk = 0;
  int			status;
  int			magic_num;
  uint8_t		*temp;
  int			k, m;
  int			full = 0;
  int			count;
  int			bufsize;
  int			imgsize;
  static uint8_t	mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};
  
  status = 0;
  magic_num = 5;

  if ((img->pxm_type != 'p') && (img->pxm_type != 'g') &&
    (img->pxm_type != 'b'))
      return (1);

  if ((img->height <= 0) || (img->width <= 0))
    return (1);

  if (img->pxm_type == 'p')
    magic_num = 6;
  else if (img->pxm_type == 'g')
    magic_num = 5;
  else
    magic_num = 4;

  img->max_gray = 255;

/* write header info to output file */

  if (magic_num > 4) /* print max_gray if not a bitmap */ {
    if (fprintf (fout, "P%d\n%d\n%d\n%d\n", magic_num, img->width, 
      img->height, img->max_gray) == EOF) 
        status = 1;
  }
  else /* bitmap */ {
    if (fprintf (fout, "P%d\n%d\n%d\n", magic_num, img->width, 
      img->height) == EOF) 
        status = 1;
  }

/* write image to output file */

  if (magic_num < 6) /* not color */ {
    if (magic_num == 5) /* grayscale */ {
      for (i = 0; i < img->height; i++) 
        if (img->width != fwrite ((char *) img->mono[i], 
	  sizeof (uint8_t), img->width, fout)) 
            status = 1;
    }
    else /* bitmap */ {
      bufsize = img->width / 8;
      if (img->width % 8) 
        bufsize++;
      temp = (uint8_t *) malloc (bufsize * sizeof (uint8_t));
      for (i = 0; i < img->height; i++) {
	full = 0;
	j = 0;
        for (k = 0; k < bufsize; k++) {
          temp[k] = 0;
	  for (m = 0; m < 8; m++) {
	    if (!full) 
	      temp[k] |= ((img->mono[i][j]) ? 0 : mask[m]);
	    if (++j >= img->width)
              full = 1;
	  }
	}
        if (bufsize != fwrite ((uint8_t *) temp, sizeof (unsigned
          char), bufsize, fout)) {
            fprintf (stderr, "Failed to write %d bytes to output file.\n",
              bufsize);
            return (1);
        }
      }
      free (temp);
    }
  }
  else /* color */ {
    temp = (uint8_t *) malloc (sizeof (uint8_t) * 3 * 
      img->width);
    for (i = 0; i < img->height; i++) {
      m = 0;
      for (j = 0; j < img->width; j++)
        for (k = 0; k < 3; k++)
	  temp[m++] = img->color[k][i][j];

      if (3 * img->width != fwrite ((char *) temp, 
	sizeof (uint8_t), 3 * img->width, fout)) 
	  status = 1;
    }
    free (temp);
  }
  return(status);
}

/*************************************************************************/
/* read_pxm ()                                                           */
/*  routine that reads in an image from a file in  pbm, pgm or ppm 	 */
/*  format, allocates storage for it, and returns a pointer to the image */
/*  data structure.  It returns a value 1 if it was not able to read the */
/* data.  Otherwise, it returns 0.					 */
/*************************************************************************/

int read_pxm (FILE *fin, struct pxm_img *img)
{
  int			status = 0;
  int			i, j, k, m;
  uint8_t		*temp;
  static uint8_t	mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};
  int			full = 0;
  int			count;
  uint8_t		*ptr;
  int			imgsize;
  int			bufsize;

  status = read_pxm_header (fin, img);

  if (status) {
    fprintf (stderr, "Error reading header\n");
    exit (-1);
  }

  if (!status) {
    if (img->pxm_type == 'g') {
      img->mono = (uint8_t **)get_img (img->width, img->height, sizeof (uint8_t));
      for (i = 0; i < img->height; i++) {
        if (img->width != fread ((uint8_t *) img->mono[i], sizeof 
          (uint8_t), img->width, fin)) 
            status = 1;
      }
    }
    else if (img->pxm_type == 'b') {
      bufsize = img->width / 8;
      if (img->width % 8)
        bufsize++;
      temp = (uint8_t *) malloc (sizeof (uint8_t) * bufsize);
      img->mono = (uint8_t **)get_img (img->width, img->height, sizeof (uint8_t));

      for (i = 0; i < img->height; i++) {
        full = 0;
        j = 0;
	if (!(count = fread ((uint8_t *) temp, sizeof (unsigned
	  char), bufsize, fin)))
	    return (1);
        for (k = 0; k < bufsize; k++) {
	  for (m = 0; m < 8; m++) {
	    if (!full) 
	      img->mono[i][j] = (temp[k] & mask[m]) ? 0 : 255;
	    if (++j >= img->width) 
              full = 1;
	  }
	}
      }
      free (temp);
    }
    else  /* pixel map; pxm_type = 'p' */ {
      img->color = (uint8_t ***) malloc (sizeof (uint8_t **) * 3);
      for (i = 0; i < 3; i++)
        img->color[i] = (uint8_t **)get_img (img->width, img->height, 
          sizeof (uint8_t));
      temp = (uint8_t *) malloc (sizeof (uint8_t) * 3 * img->width);
      for (i = 0; i < img->height; i++) {
        if (img->width * 3 != fread ((uint8_t *) temp, sizeof (unsigned
          char), img->width * 3, fin))
            status = 1;
	m = 0;
        for (j = 0; j < img->width; j++)
	  for (k = 0; k < 3; k++)
	    img->color[k][i][j] = temp[m++];
      }
      free (temp);
    }
  }
  return (status);
}


int read_pxm_header (FILE *fin, struct pxm_img *img)
{
  char			str[75];
  char			*str2;
  char			magic_char;
  char			magic_num;
  int			num_items = 0;
  int			max_items;
  char			**items;

  items = (char **) malloc (sizeof (char *) * 4);
  if (fgets (str, 3, fin) == NULL) /* failed read or EOF */
    return (1);
  else if ((str[0] != 'P') || ((str[1] != '4') && (str[1] != '5')
    && (str[1] != '6')))  /* invalid "magic number" */
    return (1);

  if (str[1] == '4')
    img->pxm_type = 'b';
  else if (str[1] == '5')
    img->pxm_type = 'g';
  else
    img->pxm_type = 'p';

  max_items = (img->pxm_type == 'b') ? 2 : 3;

  while (num_items < max_items) {
    if (fgets (str, 72, fin) == NULL) /* failed read or EOF */
      return (1);
    if (strlen (str) > (size_t) 70) /* line too long */
      return (1);
    if ((str2 = strchr (str, '#')) != NULL) /* strip comments */
      str2[0] = '\0';
    str2 = strtok (str, " \t\n\r\f\v");
    while (str2 != NULL) {
      num_items++;
      if (num_items > max_items)
        return (1);
      items[num_items] = (char *) malloc (sizeof (char) * (strlen (str2)
        +1));
      strcpy (items[num_items], str2);
      str2 = strtok (NULL, " \t\n\r\f\v");
    }
  }

  img->width = atoi (items[1]);
  if (img->width <= 0)
    return (1);
  img->height = atoi (items[2]);
  if (img->height <= 0)
    return (1);
  if (img->pxm_type != 'b') {
    img->max_gray = atoi (items[3]);
    if ((img->max_gray < 0) || (img->max_gray > 255))
      return (1);
  }
  free (items);
  return (0);
}

void free_pxm (struct pxm_img *img)

{
  int			i;

  if (img->pxm_type != 'p')
    free_img ((void *)img->mono);
  else
    for (i = 0; i < 3; i++)
      free_img ((void *)img->color[i]);
}




