/*----------------------------------------------------------------------------

  Command line interface for Smooth Contours (an unsupervised method
  for detecting smooth contours in digital images)

  Copyright (c) 2016 rafael grompone von gioi <grompone@gmail.com>,
                     Gregory Randall <randall@fing.edu.uy>

  Smooth Contours is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include "smooth_contours.h"

/*----------------------------------------------------------------------------*/
/* print usage and exit
 */
static void usage(void)
{
  fprintf(stderr,"Smooth Contours %s\n",SMOOTH_CONTOURS_VERSION);
  fprintf(stderr,"Copyright (c) 2016 ");
  fprintf(stderr,"Rafael Grompone von Gioi and Gregory Randall\n\n");
  fprintf(stderr,"This program and its source code are part of the following ");
  fprintf(stderr,"publication.\nIf you use it, please cite:\n\n");
  fprintf(stderr,"  \"Unsupervised Smooth Contour Detection\"\n");
  fprintf(stderr,"  by Rafael Grompone von Gioi and Gregory Randall,\n");
  fprintf(stderr,"  Image Processing On Line, 2016.\n");
  fprintf(stderr,"  http://dx.doi.org/10.5201/ipol.2016.175\n\n");
  fprintf(stderr,"usage: smooth_contours <image> ");
  fprintf(stderr,"[-q Q] [-w W] [-t T] [-p P]\n\n");
  fprintf(stderr,"image  PGM or ASC file formats are handled\n");
  fprintf(stderr,"-q Q   set pixel quantization step to Q (default Q=2.0)\n");
  fprintf(stderr,"-w W   set line width in PDF output to W (default W=1.3)\n");
  fprintf(stderr,"-t T   write TXT output to file T\n");
  fprintf(stderr,"-p P   write PDF output to file P\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"examples: smooth_contours input.pgm -p output.pdf\n");
  fprintf(stderr,"          smooth_contours input.pgm -p output.pdf");
  fprintf(stderr," -t output.txt -w 0\n");

  exit(EXIT_FAILURE);
}

/*----------------------------------------------------------------------------*/
/* get an optional parameter from arguments.

   if found, the value is returned and it is removed from the list of arguments.
   adapted from pick_option by Enric Meinhardt-Llopis.

   example: if arguments are "command -p 123 input.txt",
            char * p = get_option(&argc,&argv,"-p","0");
            will give "123" in p a leave arguments as "command input.txt"
 */
static char * get_option(int * argc, char *** argv, char * opt, char * def)
{
  int i,j;

  for(i=0; i<(*argc-1); i++) /* last argument cannot have an optional value */
    if( strcmp( (*argv)[i], opt ) == 0 )  /* option opt found */
      {
        char * r = (*argv)[i+1];     /* save the optional value to return   */
        for(j=i; j < (*argc-2); j++) /* shift arguments to remove opt+value */
          (*argv)[j] = (*argv)[j+2];
        *argc -= 2;     /* decrease the number of arguments in 2, opt+value */
        return r;  /* return the value found for option opt */
      }
  return def; /* option not found, return the default value */
}

/*----------------------------------------------------------------------------*/
/*                                    main                                    */
/*----------------------------------------------------------------------------*/
int main(int argc, char ** argv)
{
  double * image;      /* image of size X,Y */
  double * x;          /* x[n] y[n] coordinates of result contour point n */
  double * y;
  int * curve_limits;  /* limits of the curves in the x[] and y[] */
  int X,Y,N,M;         /* result: N contour points, forming M curves */
  double Q = atof(get_option(&argc,&argv,"-q","2.0")); /* default Q=2        */
  double W = atof(get_option(&argc,&argv,"-w","1.3")); /* PDF line width 1.3 */
  char * txt_out = get_option(&argc,&argv,"-t",NULL);  /* read pdf filename  */
  char * pdf_out = get_option(&argc,&argv,"-p",NULL);  /* read txt filename  */

  /* read input */
  if( argc != 2 ) usage();
  image = read_image(argv[1],&X,&Y);

  /* call Smooth Contours */
  smooth_contours(&x, &y, &N, &curve_limits, &M, image, X, Y, Q);

  /* write required outputs, TXT and/or PDF */
  if( txt_out != NULL ) write_curves_txt(x,y,curve_limits,M,txt_out);
  if( pdf_out != NULL ) write_curves_pdf(x,y,curve_limits,M,pdf_out,X,Y,W);

  /* free memory */
  free( (void *) image );
  free( (void *) curve_limits );
  free( (void *) x );
  free( (void *) y );

  return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/
