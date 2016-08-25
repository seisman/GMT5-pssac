/*--------------------------------------------------------------------
 *
 *  Copyright (c) 2016 by Dongdong Tian
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 3 or any later version.
 *
 *  This program is distrdeibuted in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  Contact info: seisman.info@gmail.com
 *  Project Home: https://github.com/seisman/pssac
 *--------------------------------------------------------------------*/
/*
 * Brief synopsis: pssac will plot seismogram in SAC format on maps
 */

#define THIS_MODULE_NAME	"pssac"
#define THIS_MODULE_LIB		"sac"
#define THIS_MODULE_PURPOSE	"Plot seismograms in SAC format on maps"
#define THIS_MODULE_KEYS	"<DI,CCi,T-i,>XO,RG-"

#include "gmt_dev.h"
#include "sacio.h"

#define GMT_PROG_OPTIONS "->BJKOPRUVXYcht"

/* Control structure for pssac */

struct PSSAC_CTRL {
    struct PSSAC_In {   /* Input files */
        bool active;
        char **file;
        unsigned int n;
    } In;
    struct PSSAC_C {    /* -C<t0>/<t1> */
        bool active;
        double t0, t1;
    } C;
	struct PSSAC_D {	/* -D<dx>/<dy> */
		bool active;
		double dx, dy;
	} D;
    struct PSSAC_E {    /* -Ea|b|d|k|n<n>|u<n> */
        bool active;
        char keys[2];
    } E;
    struct PSSAC_F {    /* -Fiqr */
        bool active;
        char keys[GMT_LEN256];
    } F;
    struct PSSAC_G {    /* -G[p|n]+g<fill>+z<zero>+t<t0>/<t1> */
        bool active[2];
        struct GMT_FILL fill[2];
        float zero[2];
        bool cut[2];
        float t0[2];
        float t1[2];
    } G;
    struct PSSAC_M {    /* -M<size>/<alpha> */
        bool active;
        double size;
        bool norm;      /* true if -M<size> */
        bool scaleALL;  /* true if alpha=0 */
        double alpha;
        bool dist_scaling; /* true if alpha>=0 */
    } M;
    struct PSSAC_T {   /* -T+t<n>+r<reduce_vel>+s<shift> */
        bool active;
        bool align;
        int tmark;
        bool reduce;
        double reduce_vel;
        double shift;
    } T;
	struct PSSAC_W {	/* -W<pen> */
		struct GMT_PEN pen;
	} W;
    struct PSSAC_m {
        bool active;
        double sec_per_measure;
    } m;
    struct PSSAC_v {
        bool active;
    } v;
};

struct SAC_LIST {
    char *file;
    bool position;
    double x;
    double y;
    bool custom_pen;
    struct GMT_PEN pen;
};


void *New_pssac_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct PSSAC_CTRL *C;

	C = GMT_memory (GMT, NULL, 1, struct PSSAC_CTRL);

	/* Initialize values whose defaults are not 0/false/NULL */
	C->W.pen = GMT->current.setting.map_default_pen;

	return (C);
}

void Free_pssac_Ctrl (struct GMT_CTRL *GMT, struct PSSAC_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	GMT_freepen (GMT, &C->W.pen);
	GMT_free (GMT, C);
}

int GMT_pssac_usage (struct GMTAPI_CTRL *API, int level)
{
	/* This displays the pssac synopsis and optionally full usage information */

	GMT_show_name_and_purpose (API, THIS_MODULE_LIB, THIS_MODULE_NAME, THIS_MODULE_PURPOSE);
	if (level == GMT_MODULE_PURPOSE) return (GMT_NOERROR);
	GMT_Message (API, GMT_TIME_NONE, "usage: pssac <saclist>|<sacfiles> %s %s\n", GMT_J_OPT, GMT_Rgeoz_OPT);
    GMT_Message (API, GMT_TIME_NONE, "\t[%s] [-C[<t0>/<t1>]] [-D<dx>[/<dy>]] [-Ea|b|k|d|n[<n>]|u[<n>]] [-F[i][q][r]]\n", GMT_B_OPT);
    GMT_Message (API, GMT_TIME_NONE, "\t[-G[p|n][+g<fill>][+t<t0>/<t1>][+z<zero>]] [-K] [-M<size>/<alpha>] [-O] [-P]\n");
    GMT_Message (API, GMT_TIME_NONE, "\t[-T[+t<tmark>][+r<reduce_vel>][+s<shift>]] [%s] [%s] \n", GMT_U_OPT, GMT_V_OPT);
    GMT_Message (API, GMT_TIME_NONE, "\t[-W<pen>] [%s] [%s] [%s] \n\t[%s] [%s] [-m<sec_per_measuer>] [-v]\n", GMT_X_OPT, GMT_Y_OPT, GMT_c_OPT, GMT_h_OPT, GMT_t_OPT);
    GMT_Message (API, GMT_TIME_NONE, "\n");

    if (level == GMT_SYNOPSIS) return (EXIT_FAILURE);

    GMT_Message (API, GMT_TIME_NONE, "\t<sacfiles> are the name of SAC files to plot on maps. Only evenly spaced SAC data is supported.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t<saclist> is an ASCII file (or stdin) which contains the name of SAC files to plot and controlling parameters.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   Each record has 1, 3 or 4 items:  <filename> [<X> <Y>] [<pen>]. \n");
    GMT_Message (API, GMT_TIME_NONE, "\t   <filename> is the name of SAC file to plot.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   <X> and <Y> are the location of the trace on the plot.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t      On linear plots, the default <X> is the begin time of SAC file, which will be adjusted if -T option is used, \n");
    GMT_Message (API, GMT_TIME_NONE, "\t      the default <Y> will be adjusted if -E option is used.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t      On geographic plots, the default <X> and <Y> are determinted by stlo and stla from SAC header.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t      The <X> and <Y> given here will override the position determined by command line options.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   If <pen> is given, it will override the pen from -W option for current SAC file only.\n");
    GMT_Option (API, "J-Z,R");
    GMT_Message (API, GMT_TIME_NONE, "\n\tOPTIONS:\n");
    GMT_Option (API, "B-");
    GMT_Message (API, GMT_TIME_NONE, "\t-C Only read and plot data between t0 and t1. The reference time of t0 and t1 is determined by -T option\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   Default to read and plot the whole trace. If only -C is used, t0 and t1 are determined from -R option\n");
    GMT_Message (API, GMT_TIME_NONE, "\t-D Offset all traces by <dx>/<dy>. PROJ_LENGTH_UNIT is used if unit is not specified.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t-E Determine profile type (Y axis). \n");
    GMT_Message (API, GMT_TIME_NONE, "\t   a: azimuth profile\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   b: back-azimuth profile\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   k: epicentral distance (in km) profile\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   d: epicentral distance (in degree) profile \n");
    GMT_Message (API, GMT_TIME_NONE, "\t   n: traces are numbered from <n> to <n>+N in y-axis, default value of <n> is 0\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   u: Y location is determined from SAC header user<n>, default using user0\n");
    GMT_Message (API, GMT_TIME_NONE, "\t-F Data processing before plotting.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   i: integral\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   q: square\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   r: remove mean value\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   i|q|r can repeat mutiple times. -Frii will convert accerate to displacement.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   The order of i|q|r controls the order of the data processing.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t-G Paint postive or negative portion of traces.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   If only -G is used, default to fill the positive portion black.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   [p|n] controls the painting of postive portion or negative portion. Repeat -G option to specify fills for pos/neg portion, respectively.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   +g<fill>: color to fill\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   +t<t0>/<t1>: paint traces between t0 and t1 only. The reference time of t0 and t1 is determined by -T option.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   +z<zero>: define zero line. From <zero> to top is positive portion, from <zero> to bottom is negative portion.\n");
    GMT_Option (API, "K");
    GMT_Message (API, GMT_TIME_NONE, "\t-M Vertical scaling\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   <size>: each trace will scaled to <size>[u]. The default unit is PROJ_LENGTH_UNIT.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t      The scale factor is defined as yscale = size*(north-south)/(depmax-depmin)/map_height \n");
    GMT_Message (API, GMT_TIME_NONE, "\t   <size>/<alpha>: \n");
    GMT_Message (API, GMT_TIME_NONE, "\t      <alpha> < 0, use the same scale factor for all trace. The scale factor scale the first trace to <size>[u]\n");
    GMT_Message (API, GMT_TIME_NONE, "\t      <alpha> = 0, yscale=size, no unit is allowed. \n");
    GMT_Message (API, GMT_TIME_NONE, "\t      <alpha> > 0, yscale=size*r^alpha, r is the distance range in km.\n");
    GMT_Option (API, "O,P");
    GMT_Message (API, GMT_TIME_NONE, "\t-T Time alignment. \n");
    GMT_Message (API, GMT_TIME_NONE, "\t   +t<tmark> align all trace along time mark. Choose <tmark> from -5(b), -3(o), -2(a), 0-9(t0-t9).\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   +r<reduce_vel> reduce velocity in km/s.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   +s<shift> shift all traces by <shift> seconds.\n");
    GMT_Option (API, "U,V");
    GMT_pen_syntax (API->GMT, 'W', "Set pen attributes [Default pen is %s]:", 0);
    GMT_Option (API, "X,c,h,t");
    GMT_Message (API, GMT_TIME_NONE, "\t-m Time scaling while plotting on geographic plots.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t   <sec_per_measure> is in second per inch.\n");
    GMT_Message (API, GMT_TIME_NONE, "\t-v Plot traces vertically.\n");
    GMT_Option (API, ".");

	return (EXIT_FAILURE);
}

int GMT_pssac_parse (struct GMT_CTRL *GMT, struct PSSAC_CTRL *Ctrl, struct GMT_OPTION *options)
{
	/* This parses the options provided to pssac and sets parameters in Ctrl.
	 * Note Ctrl has already been initialized and non-zero default values set.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0;
	char txt_a[GMT_LEN256] = {""}, txt_b[GMT_LEN256] = {""};
	struct GMT_OPTION *opt = NULL;
	struct GMTAPI_CTRL *API = GMT->parent;

	int j, k;
    char p[GMT_BUFSIZ] = {""};
    size_t n_alloc = 0;
    unsigned int pos = 0;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Collect input files */
                Ctrl->In.active = true;
                if (n_alloc <= Ctrl->In.n)  Ctrl->In.file = GMT_memory (GMT, Ctrl->In.file, n_alloc += GMT_SMALL_CHUNK, char *);
                if (GMT_check_filearg (GMT, '<', opt->arg, GMT_IN, GMT_IS_DATASET))
                    Ctrl->In.file[Ctrl->In.n++] = strdup (opt->arg);
                else
                    n_errors++;
				break;

			/* Processes program-specific parameters */

            case 'C':
                Ctrl->C.active = true;
                if ((j = sscanf (opt->arg, "%lf/%lf", &Ctrl->C.t0, &Ctrl->C.t1)) != 2) {
                    Ctrl->C.t0 = GMT->common.R.wesn[XLO];
                    Ctrl->C.t1 = GMT->common.R.wesn[XHI];
                }
                break;

			case 'D':
				if ((j = sscanf (opt->arg, "%[^/]/%s", txt_a, txt_b)) < 1) {
					GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -D option: Give x [and y] offsets\n");
					n_errors++;
				} else {
					Ctrl->D.active = true;
					Ctrl->D.dx = GMT_to_inch (GMT, txt_a);
					Ctrl->D.dy = (j == 2) ? GMT_to_inch (GMT, txt_b) : Ctrl->D.dx;
				}
				break;

            case 'E':
                Ctrl->E.active = true;
                strcpy(Ctrl->E.keys, &opt->arg[0]);
                break;

            case 'F':
                Ctrl->F.active = true;
                strcpy(Ctrl->F.keys, &opt->arg[0]);
                break;

            case 'G':      /* phase painting */
                switch (opt->arg[0]) {
                    case 'p': j = 1, k = 0; break;
                    case 'n': j = 1, k = 1; break;
                    default : j = 0, k = 0; break;
                }
                Ctrl->G.active[k] = true;
                pos = j;
                while (GMT_getmodopt (GMT, opt->arg, "gtz", &pos, p)) {
                    switch (p[0]) {
                        case 'g':  /* fill */
                            if (GMT_getfill (GMT, &p[1], &Ctrl->G.fill[k])) {
                                GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -G+g<fill> option.\n");
                                n_errors++;
                            }
                            break;
                        case 'z':  /* zero */
                            Ctrl->G.zero[k] = atof (&p[1]);
                            break;
                        case 't':  /* +t<t0>/<t1> */
                            Ctrl->G.cut[k] = true;
                            if (sscanf (&p[1], "%f/%f", &Ctrl->G.t0[k], &Ctrl->G.t1[k]) != 2) {
                                GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -G+t<t0>/<t1> option.\n");
                                n_errors++;
                            }
                            break;
                        default:
                            GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -G: -G+g<fill>+z<zero>+t<t0>/<t1>\n");
                            break;
                    }
                }
                break;

            case 'M':
                Ctrl->M.active = true;
                j = sscanf(opt->arg, "%[^/]/%s", txt_a, txt_b);
                if (j == 1) { /* -Msize */
                    Ctrl->M.norm = true;
                    Ctrl->M.size = GMT_to_inch (GMT, txt_a);
                } else if (j == 2) {
                    if (strcmp(txt_b, "s") == 0 ) {   /* -Msize/s */
                        // TODO
                    } else if (strcmp(txt_b, "b") == 0) {  /* -Msize/b */
                        // TODO
                    } else {  /* -Msize/alpha */
                        Ctrl->M.alpha = atof (txt_b);
                        if (Ctrl->M.alpha < 0) {
                            Ctrl->M.scaleALL = true;
                            Ctrl->M.size = GMT_to_inch (GMT, txt_a);
                        } else {
                            Ctrl->M.dist_scaling = true;
                            Ctrl->M.size = atof (txt_a);
                        }
                    }
                } else {
                    GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -M option: -M<size>[/<alpha>]\n");
                    n_errors++;
                }

                if (GMT_IS_LINEAR(GMT) && (Ctrl->M.norm || Ctrl->M.scaleALL))
                    Ctrl->M.size *= fabs((GMT->common.R.wesn[YHI]-GMT->common.R.wesn[YLO])/GMT->current.proj.pars[1]);
                break;

            case 'T':
                pos = 0;
                Ctrl->T.active = true;
                Ctrl->T.shift = 0.0;  /* default no shift */
                while (GMT_getmodopt (GMT, opt->arg, "trs", &pos, p)) {
                    switch (p[0]) {
                        case 't':
                            Ctrl->T.align = true;
                            Ctrl->T.tmark = atoi (&p[1]);
                            break;
                        case 'r':
                            Ctrl->T.reduce = true;
                            Ctrl->T.reduce_vel = atof (&p[1]);
                            break;
                        case 's':
                            Ctrl->T.shift = atof (&p[1]);
                            break;
                        default:
                            GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -T option: -T+t<align>+r<reduce_vel>+s<shift>");
                            n_errors++;
                            break;
                    }
                }
                break;

			case 'W':		/* Set line attributes */
				if (opt->arg[0] && GMT_getpen (GMT, &opt->arg[0], &Ctrl->W.pen)) {
					GMT_pen_syntax (GMT, 'W', "sets pen attributes [Default pen is %s]:", 3);
					n_errors++;
				}
				break;

            case 'v':
                Ctrl->v.active = true;
                break;

            case 'm':
                Ctrl->m.active = true;
                Ctrl->m.sec_per_measure = atof(opt->arg);
                break;

			default:	/* Report bad options */
				n_errors += GMT_default_error (GMT, opt->option);
				break;
		}
	}

	/* Check that the options selected are mutually consistent */

	n_errors += GMT_check_condition (GMT, !GMT->common.R.active, "Syntax error: Must specify -R option\n");
	n_errors += GMT_check_condition (GMT, !GMT->common.J.active, "Syntax error: Must specify a map projection with the -J option\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_OK);
}

double linear_interpolate_x (double x0, double y0, double x1, double y1, double y) {
    if (y<y0 || y>y1) return x1;  // no extrapolation
    if (y1==y0) return x0;
    return (x1-x0)/(y1-y0)*(y-y0) + x0;
}

double linear_interpolate_y (double x0, double y0, double x1, double y1, double x) {
    if (x<x0 || x>x1) return y1;  // no extrapolation
    if (x1==x0) return y0;
    return (y1-y0)/(x1-x0)*(x-x0) + y0;
}

#define bailout(code) {GMT_Free_Options (mode); return (code);}
#define Return(code) {Free_pssac_Ctrl (GMT, Ctrl); GMT_end_module (GMT, GMT_cpy); bailout (code);}

void paint_phase(struct GMT_CTRL *GMT, struct PSSAC_CTRL *Ctrl, struct PSL_CTRL *PSL, double *x, double *y, int n, double zero, double t0, double t1, int mode)
{
    /* mode=0: paint positive phase */
    /* mode=1: paint negative phase */
    int i, ii;
    double *xx = NULL, *yy = NULL;

    if (Ctrl->v.active) {
        double *tmp;
        tmp = x;
        x = y;
        y = tmp;
    }

    xx = GMT_memory (GMT, 0, n+2, double);
    yy = GMT_memory (GMT, 0, n+2, double);

    for (i=0; i<n; i++) {
        if ((x[i]>=t0) && ((mode==0 && y[i]>=zero) || (mode==1 && y[i]<=zero))) {
            ii = 0;
            /* first point of polygon */
            yy[ii] = zero;
            if (i==0)
                xx[ii] = x[i];
            else
                xx[ii] = linear_interpolate_x(x[i-1], y[i-1], x[i], y[i], yy[ii]);
            ii++;

            while((i<n) && (x[i]<=t1) && ((mode==0 && y[i]>=zero) || (mode==1 && y[i]<=zero))) {
                xx[ii] = x[i];
                yy[ii] = y[i];
                i++;
                ii++;
            }

            /* last point of polygon */
            yy[ii] = zero;
            if (i==n)
                xx[ii] = x[i-1];
            else
                xx[ii] = linear_interpolate_x(x[i], y[i], x[i-1], y[i-1], yy[ii]);
            ii++;

            if (Ctrl->v.active) {
                double *tmp;
                tmp = xx;
                xx = yy;
                yy = tmp;
            }

            double *xp, *yp;
            int npts;
            if (GMT_IS_LINEAR(GMT)) {
                if ((GMT->current.plot.n = GMT_geo_to_xy_line(GMT, xx, yy, ii)) < 3) continue;
                xp = GMT->current.plot.x;
                yp = GMT->current.plot.y;
                npts = GMT->current.plot.n;
            } else {
                xp = xx;
                yp = yy;
                npts = ii;
            }
            GMT_setfill(GMT, &Ctrl->G.fill[mode], false);

            PSL_plotpolygon(PSL, xp, yp, npts);
        }
    }
    GMT_free (GMT, xx);
    GMT_free (GMT, yy);
}

void integral (double *y, double delta, int n)
{
    int i;
    y[0] = (y[0]+y[1])*delta/2.0;

    for (i=1; i<n-1; i++)
        y[i] = y[i-1] + (y[i] + y[i+1]) * delta / 2.0;
}

void rmean (double *y, int n)
{
    int i;
    double depmen = 0.0;
    for (i=0; i<n; i++) depmen += y[i];
    depmen /= n;

    for (i=0; i<n; i++) y[i] -= depmen;
}

void sqr (double *y, int n) {
    int i;
    for (i=0; i<n; i++) y[i] *= y[i];
}

int init_sac_list (struct GMT_CTRL *GMT, char **files, unsigned int n_files, struct SAC_LIST **list)
{
    unsigned int n = 0, nr;

    struct SAC_LIST *L = NULL;

    /* Got a bunch of SAC files or one file in SAC format */
    if (n_files > 1 || (n_files==1 && issac(files[0]))) {
        L = GMT_memory (GMT, NULL, n_files, struct SAC_LIST) ;
        for (n = 0; n < n_files; n++) {
            L[n].file = strdup (files[n]);
            L[n].position = false;
            L[n].custom_pen = false;
        }
    } else {    /* Must read a list file */
        size_t n_alloc = 0;
        char *line = NULL, pen[GMT_LEN256] = {""}, file[GMT_LEN256] = {""};
        double x, y;
        GMT_set_meminc (GMT, GMT_SMALL_CHUNK);
        do {
            if ((line = GMT_Get_Record(GMT->parent, GMT_READ_TEXT, NULL)) == NULL) {
                if (GMT_REC_IS_ERROR (GMT))   /* Bail if there are any read error */
                    return (GMT_RUNTIME_ERROR);
                if (GMT_REC_IS_ANY_HEADER (GMT)) /* skip headers */
                    continue;
                if (GMT_REC_IS_EOF(GMT))  /* Reached end of file */
                    break;
            }

            nr = sscanf (line, "%s %lf %lf %s", file, &x, &y, pen);
            if (nr < 1) {
                GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Read error for sac list file near row %d\n", n);
                return (EXIT_FAILURE);
            }

            if (n == n_alloc) L = GMT_malloc (GMT, L, n, &n_alloc, struct SAC_LIST);
            L[n].file = strdup (file);
            if (nr>=3) {
                L[n].position = true;
                L[n].x = x;
                L[n].y = y;
            }
            if (nr==4) {
                L[n].custom_pen = true;
				if (GMT_getpen (GMT, pen, &L[n].pen)) {
					GMT_pen_syntax (GMT, 'W', "sets pen attributes [Default pen is %s]:", 3);
				}
            }
            n++;
        } while(true);
        GMT_reset_meminc (GMT);
        n_files = n;
    }
    *list = L;

    return n_files;
}

int GMT_pssac (void *V_API, int mode, void *args)
{	/* High-level function that implements the pssac task */
	bool old_is_world;
	int error = GMT_NOERROR;

	struct GMT_PEN current_pen;
	struct PSSAC_CTRL *Ctrl = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;		/* General GMT interal parameters */
	struct GMT_OPTION *options = NULL;
	struct PSL_CTRL *PSL = NULL;		/* General PSL interal parameters */
	struct GMTAPI_CTRL *API = GMT_get_API_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

    struct SAC_LIST *L = NULL;
    unsigned int n_files;
    double yscale = 1.0;
    double y0 = 0.0, x0;
    bool read_from_ascii;
    int n, i;
    SACHEAD hd;
    float *data = NULL;
    double *x = NULL, *y = NULL;
    double tref;

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_NOT_A_SESSION);
	if (mode == GMT_MODULE_PURPOSE) return (GMT_pssac_usage (API, GMT_MODULE_PURPOSE));	/* Return the purpose of program */
	options = GMT_Create_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if (!options || options->option == GMT_OPT_USAGE) bailout (GMT_pssac_usage (API, GMT_USAGE));	/* Return the usage message */
	if (options->option == GMT_OPT_SYNOPSIS) bailout (GMT_pssac_usage (API, GMT_SYNOPSIS));	/* Return the synopsis */

	/* Parse the command-line arguments; return if errors are encountered */

	GMT = GMT_begin_module (API, THIS_MODULE_LIB, THIS_MODULE_NAME, &GMT_cpy); /* Save current state */
	if (GMT_Parse_Common (API, GMT_PROG_OPTIONS, options)) Return (API->error);

	Ctrl = New_pssac_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = GMT_pssac_parse (GMT, Ctrl, options)) != 0) Return (error);


	/*---------------------------- This is the pssac main code ----------------------------*/

	current_pen = Ctrl->W.pen;

	if (GMT_err_pass (GMT, GMT_map_setup (GMT, GMT->common.R.wesn), "")) Return (GMT_PROJECTION_ERROR);

	if ((PSL = GMT_plotinit (GMT, options)) == NULL) Return (GMT_RUNTIME_ERROR);

	GMT_plane_perspective (GMT, GMT->current.proj.z_project.view_plane, GMT->current.proj.z_level);
	GMT_plotcanvas (GMT);	/* Fill canvas if requested */

	GMT_setpen (GMT, &current_pen);

	if (Ctrl->D.active) PSL_setorigin (PSL, Ctrl->D.dx, Ctrl->D.dy, 0.0, PSL_FWD);	/* Shift plot a bit */

	old_is_world = GMT->current.map.is_world;

    read_from_ascii = (Ctrl->In.n == 0) || (Ctrl->In.n == 1 && !issac(Ctrl->In.file[0]));
    if (read_from_ascii) {      /* Got a ASCII file or read from stdin */
        GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading input file\n");
        if (GMT_Init_IO (API, GMT_IS_TEXTSET, GMT_IS_NONE, GMT_IN, GMT_ADD_DEFAULT, 0, options) != GMT_OK) {    /* Register data input */
            Return (API->error);
        }
        if (GMT_Begin_IO (API, GMT_IS_TEXTSET, GMT_IN, GMT_HEADER_ON) != GMT_OK) {  /* Enables data input and sets access mode */
            Return (API->error);
        }
    }
    n_files = init_sac_list (GMT, Ctrl->In.file, Ctrl->In.n, &L);

    if (read_from_ascii && GMT_End_IO (API, GMT_IN, 0) != GMT_OK) { /* Disables further data input */
        Return (API->error);
    }
	GMT_Report (API, GMT_MSG_VERBOSE, "Collecting %ld SAC files to plot.\n", n_files);

    for (n=0; n < n_files; n++) {  /* Loop over all SAC files */
	    GMT_Report (API, GMT_MSG_VERBOSE, "Plotting SAC file %d: %s\n", n, L[n].file);

        if ((read_sac_head (L[n].file, &hd))) {  /* skip or not */
            GMT_Report (API, GMT_MSG_NORMAL, "Warning: unable to read SAC file %s, skipped.", L[n].file);
            continue;
        }

        /* -T: determine the reference time for all times in pssac */
        tref = 0.0;
        if (Ctrl->T.active) {
            if (Ctrl->T.align) tref += *((float *)&hd + TMARK + Ctrl->T.tmark);
            if (Ctrl->T.reduce) tref += fabs(hd.dist)/Ctrl->T.reduce_vel;
            tref -= Ctrl->T.shift;
        }
        GMT_Report (API, GMT_MSG_VERBOSE, "=> %s: reference time is %lf\n", L[n].file, tref);

        /* read data and determine X position */
        if (!Ctrl->C.active) {
            if ((data = read_sac (L[n].file, &hd)) == NULL) {
                GMT_Report (API, GMT_MSG_NORMAL, "Warning: unable to read SAC file %s, skipped.", L[n].file);
                continue;
            }
        } else {
            if ((data = read_sac_pdw (L[n].file, &hd, 10, tref+Ctrl->C.t0, tref+Ctrl->C.t1)) == NULL) {
                GMT_Report (API, GMT_MSG_NORMAL, "Warning: unable to read SAC file %s, skipped.", L[n].file);
                continue;
            }
        }

        /* prepare datas */
        x = GMT_memory(GMT, 0, hd.npts, double);
        y = GMT_memory(GMT, 0, hd.npts, double);
        double dt;
        if (GMT_IS_LINEAR(GMT)) dt = hd.delta;
        else if (Ctrl->m.active) dt = hd.delta/Ctrl->m.sec_per_measure;
        else {
            GMT_Report (API, GMT_MSG_NORMAL, "Error: -m option is needed in geographic plots.\n");
            Return(EXIT_FAILURE);
        }
        for (i=0; i<hd.npts; i++) {
            x[i] = i * dt;
            y[i] = data[i];
        }
        GMT_free (GMT, data);

        /* -F: data preprocess */
        for (i=0; Ctrl->F.keys[i]!='\0'; i++) {
            switch (Ctrl->F.keys[i]) {
                case 'i': integral(y, hd.delta, hd.npts); hd.npts--; break;
                case 'q':   sqr(y, hd.npts); break;
                case 'r': rmean(y, hd.npts); break;
                default: break;
            }
        }

        /* -M: determine yscale for multiple traces */
        if (Ctrl->M.active) {
            if (Ctrl->M.norm || (Ctrl->M.scaleALL && n==0)) {
                hd.depmax=-1.e20; hd.depmin=1.e20; hd.depmen=0.;
                for(i=0; i<hd.npts; i++){
                    hd.depmax = hd.depmax > y[i] ? hd.depmax : y[i];
                    hd.depmin = hd.depmin < y[i] ? hd.depmin : y[i];
                    hd.depmen += y[i];
                }
                hd.depmen = hd.depmen/hd.npts;
                yscale = Ctrl->M.size / (hd.depmax - hd.depmin);
            } else if (Ctrl->M.dist_scaling) {
                hd.depmax=-1.e20; hd.depmin=1.e20; hd.depmen=0.;
                yscale = Ctrl->M.size * pow(fabs(hd.dist), Ctrl->M.alpha);
            }
            for (i=0; i<hd.npts; i++) y[i] = y[i]*yscale;
        }
        GMT_Report (API, GMT_MSG_VERBOSE, "=> %s: yscale of trace: %lf\n", L[n].file, yscale);

        /* -v: swap x and y */
        if (Ctrl->v.active) {
            /* swap arrays */
            double *xp;
            xp = GMT_memory(GMT, 0, hd.npts, double);
            memcpy((void *)xp, (void *)y, hd.npts*sizeof(double));
            memcpy((void *)y, (void *)x, hd.npts*sizeof(double));
            memcpy((void *)x, (void *)xp, hd.npts*sizeof(double));
            GMT_free(GMT, xp);
        }

        /* Default to plot trace at station locations on geographic maps */
        if (!GMT_IS_LINEAR(GMT) && L[n].position==false) {
            L[n].position = true;
            GMT_geo_to_xy (GMT, hd.stlo, hd.stla, &L[n].x, &L[n].y);
            GMT_Report (API, GMT_MSG_VERBOSE, "=> %s: Geographic location: (%lf, %lf)\n", L[n].file, hd.stlo, hd.stla);
        }

        if (L[n].position) {   /* position (X0,Y0) on plots */
            x0 = L[n].x;
            y0 = L[n].y;
        } else {
            /* determine X0 */
            if (!Ctrl->C.active) x0 = hd.b - tref;
            else x0 = Ctrl->C.t0;

            /* determin Y0 */
            unsigned int user = 0; /* default using user0 */
            if (Ctrl->E.active) {
                switch (Ctrl->E.keys[0]) {
                    case 'a':
                        if (hd.az == SAC_FLOAT_UNDEF) GMT_Report (API, GMT_MSG_NORMAL, "Warning: Header az not defined in %s\n", L[n].file);
                        y0 = hd.az;
                        break;
                    case 'b':
                        if (hd.baz == SAC_FLOAT_UNDEF) GMT_Report (API, GMT_MSG_NORMAL, "Warning: Header baz not defined in %s\n", L[n].file);
                        y0 = hd.baz;
                        break;
                    case 'd':
                        if (hd.gcarc == SAC_FLOAT_UNDEF) GMT_Report (API, GMT_MSG_NORMAL, "Warning: Header gcarc not defined in %s", L[n].file);
                        y0 = hd.gcarc;
                        break;
                    case 'k':
                        if (hd.dist == SAC_FLOAT_UNDEF) GMT_Report (API, GMT_MSG_NORMAL, "Warning: Header dist not defined in %s\n", L[n].file);
                        y0 = hd.dist;
                        break;
                    case 'n':
                        y0 = n;
                        if (Ctrl->E.keys[1]!='\0') y0 += atof(&Ctrl->E.keys[1]);
                        break;
                    case 'u':  /* user0 to user9 */
                        if (Ctrl->E.keys[1] != '\0') user = atoi(&Ctrl->E.keys[1]);
                        y0 = *((float *) &hd + USERN + user);
                        if (y0 == SAC_FLOAT_UNDEF) GMT_Report (API, GMT_MSG_NORMAL, "Warning: Header user%d not defined in %s\n", user, L[n].file);
                        break;
                    default:
                        GMT_Report (API, GMT_MSG_NORMAL, "Error: Wrong choice of profile type (d|k|a|b|n) \n");
                        Return(EXIT_FAILURE);
                        break;
                }
            }
            if (Ctrl->v.active) {
                /* swap x0 and y0 */
                double xy;
                xy = x0;  x0 = y0;  y0 = xy;
            }
        }

        GMT_Report (API, GMT_MSG_VERBOSE, "=> %s: location of trace: (%lf, %lf)\n", L[n].file, x0, y0);
        for (i=0; i<hd.npts; i++) {
            x[i] += x0;
            y[i] += y0;
        }

        /* report xmin, xmax, ymin and ymax */
        double ymax=-1.0e20, ymin=1.0e20;
        for (i=0; i<hd.npts; i++) {
            ymax = ymax > y[i] ? ymax : y[i];
            ymin = ymin < y[i] ? ymin : y[i];
        }
        GMT_Report (API, GMT_MSG_LONG_VERBOSE, "=> %s: after scaling and shifting : xmin=%lf xmax=%lf ymin=%lf ymax=%lf\n", L[n].file, x[0], x[hd.npts-1], ymin, ymax);

        double *xp, *yp;
        int npts;
        unsigned int *plot_pen;
        if (GMT_IS_LINEAR(GMT)) {
            GMT->current.plot.n = GMT_geo_to_xy_line (GMT, x, y, hd.npts);
            xp = GMT->current.plot.x;
            yp = GMT->current.plot.y;
            npts = GMT->current.plot.n;
            plot_pen = GMT->current.plot.pen;
        } else {
            xp = x;
            yp = y;
            npts = hd.npts;
            plot_pen = GMT_memory (GMT, PSL_DRAW, npts, unsigned int);
            plot_pen[0] = PSL_MOVE;
        }

        GMT_Report (API, GMT_MSG_LONG_VERBOSE, "=> %s: after projecting (in inch): xmin=%lf xmax=%lf ymin=%lf ymax=%lf\n", L[n].file, xp[0], xp[npts-1], ymin, ymax);
        if (L[n].custom_pen) {
	        current_pen = L[n].pen;
            GMT_setpen (GMT, &L[n].pen);
        }
        GMT_plot_line (GMT, xp, yp, plot_pen, npts, current_pen.mode);
        if (L[n].custom_pen) {
	        current_pen = Ctrl->W.pen;
            GMT_setpen (GMT, &current_pen);
        }

        for (i=0; i<=1; i++) { /* 0=positive; 1=negative */
            if (Ctrl->G.active[i]) {
                double zero = 0.0;
                if (!Ctrl->v.active) zero = Ctrl->G.zero[i]*yscale + y0;
                else                 zero = Ctrl->G.zero[i]*yscale + x0;

                if (!Ctrl->G.cut[i]) {
                    if (!Ctrl->v.active) {
                        Ctrl->G.t0[i] = x[0];
                        Ctrl->G.t1[i] = x[hd.npts-1];
                    } else {
                        Ctrl->G.t0[i] = y[0];
                        Ctrl->G.t1[i] = y[hd.npts-1];
                    }
                }
                GMT_Report (API, GMT_MSG_VERBOSE, "=> %s: Painting traces: zero=%lf t0=%lf t1=%lf\n",
                        L[n].file, zero, Ctrl->G.t0[i], Ctrl->G.t1[i]);
                paint_phase(GMT, Ctrl, PSL, x, y, npts, zero, Ctrl->G.t0[i], Ctrl->G.t1[i], i);
            }
        }

        GMT_free(GMT, x);
        GMT_free(GMT, y);
    }

	if (Ctrl->D.active) PSL_setorigin (PSL, -Ctrl->D.dx, -Ctrl->D.dy, 0.0, PSL_FWD);	/* Reset shift */

	PSL_setdash (PSL, NULL, 0);
	GMT->current.map.is_world = old_is_world;

	GMT_map_basemap (GMT);
	GMT_plane_perspective (GMT, -1, 0.0);

	GMT_plotend (GMT);

	Return (GMT_OK);
}
