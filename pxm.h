#include <stdio.h>
#include <stdint.h>

struct pxm_img
{
  int			height;
  int			width;
  int			max_gray;
  char			pxm_type;	/* 'b' = bitmap; 'g' = graymap;  */
							/* 'p' = pixel map               */
  uint8_t		**mono;
  uint8_t		***color;
};

int write_pxm (FILE *fout, struct pxm_img *img);
int read_pxm_header (FILE *fin, struct pxm_img *img);
int read_pxm (FILE *fin, struct pxm_img *img);
void free_pxm (struct pxm_img *img);

