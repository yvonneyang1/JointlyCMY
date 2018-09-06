#include <libxml/parser.h>

typedef struct Config
{
  	int tog_flag; /* tog_flag = 1: enable toggle in DBS, else: disable toggle*/
  	int swap_flag; /* swap_flag = 1: enable swap in DBS, else: disable swap*/
  	double gamma; /* Gamma correction parameter. If it == 1, then correction has no affect */
  	char * infn; /* Input image file name */
  	char * output_fname; /* Output image file name prefix */
  	int	scale_factor;
  	int checkflag;
  	char *initHfname;
  	int Rand_Init;
  	int size3b; /* A constant equals to 13 and used for allocating Cpe */
} Config;
