/* ASDBGET - Database Translating AS/400 FTP Client
 * Copyright (C) 1999 Jason M. Felice
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA */

#define DTDVERSION "0.9"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_LIB5250
#include <tn5250/config.h>
#include <tn5250/utility.h>
#endif
#include "ftplib.h"

#ifdef __STDC__
#define PARMS(x) x
#else
#define PARMS(x) ()
#endif

#define nibble(buf, i) \
  ((i&1) ? \
   (buf[i>>1] & 0x0f) : \
   ((buf[i>>1] & 0xf0) >> 4))

typedef unsigned char Byte;

struct field_tag
  {
    struct field_tag *	next;
    struct field_tag *	prev;

    char		name[11];
    char		desc[51];

    Byte *		vbuffer;

    int			length;
    char		type;
    int			precision;
    int			buffer_offset;
    int			buffer_length;
  };

typedef struct field_tag field_t;


#ifdef HAVE_LIB5250
static char shortopts[] = "F:m:o:O:p:R:u:";
#else
static char shortopts[] = "F:o:O:p:R:u:";
#endif

#ifdef HAVE_GETOPT_LONG
static struct option longopts[] =
  {
#ifdef HAVE_LIB5250
    { "map",		  1,	0,    'm' },
#endif
    { "output",		  1,	0,    'o' },
    { "user",		  1,    0,    'u' },
    { NULL }
  };
#endif

#ifndef HAVE_LIB5250
/* Stole the ebcdic_to_latin1 table from tn5250, which was created by GNU
 * recode. */
static const Byte e2a [256] =
  {
   0, 1, 2, 3, 156, 9, 134, 127,	/*   0 -   7 */
   151, 141, 142, 11, 12, 13, 14, 15,	/*   8 -  15 */
   16, 17, 18, 19, 157, 133, 8, 135,	/*  16 -  23 */
   24, 25, 146, 143, 28, 29, 30, 31,	/*  24 -  31 */
   128, 129, 130, 131, 132, 10, 23, 27,		/*  32 -  39 */
   136, 137, 138, 139, 140, 5, 6, 7,	/*  40 -  47 */
   144, 145, 22, 147, 148, 149, 150, 4,		/*  48 -  55 */
   152, 153, 154, 155, 20, 21, 158, 26,		/*  56 -  63 */
   32, 160, 161, 162, 163, 164, 165, 166,	/*  64 -  71 */
   167, 168, 91, 46, 60, 40, 43, 33,	/*  72 -  79 */
   38, 169, 170, 171, 172, 173, 174, 175,	/*  80 -  87 */
   176, 177, 93, 36, 42, 41, 59, 94,	/*  88 -  95 */
   45, 47, 178, 179, 180, 181, 182, 183,	/*  96 - 103 */
   184, 185, 124, 44, 37, 95, 62, 63,	/* 104 - 111 */
   186, 187, 188, 189, 190, 191, 192, 193,	/* 112 - 119 */
   194, 96, 58, 35, 64, 39, 61, 34,	/* 120 - 127 */
   195, 97, 98, 99, 100, 101, 102, 103,		/* 128 - 135 */
   104, 105, 196, 197, 198, 199, 200, 201,	/* 136 - 143 */
   202, 106, 107, 108, 109, 110, 111, 112,	/* 144 - 151 */
   113, 114, 203, 204, 205, 206, 207, 208,	/* 152 - 159 */
   209, 126, 115, 116, 117, 118, 119, 120,	/* 160 - 167 */
   121, 122, 210, 211, 212, 213, 214, 215,	/* 168 - 175 */
   216, 217, 218, 219, 220, 221, 222, 223,	/* 176 - 183 */
   224, 225, 226, 227, 228, 229, 230, 231,	/* 184 - 191 */
   123, 65, 66, 67, 68, 69, 70, 71,	/* 192 - 199 */
   72, 73, 232, 233, 234, 235, 236, 237,	/* 200 - 207 */
   125, 74, 75, 76, 77, 78, 79, 80,	/* 208 - 215 */
   81, 82, 238, 239, 240, 241, 242, 243,	/* 216 - 223 */
   92, 159, 83, 84, 85, 86, 87, 88,	/* 224 - 231 */
   89, 90, 244, 245, 246, 247, 248, 249,	/* 232 - 239 */
   48, 49, 50, 51, 52, 53, 54, 55,	/* 240 - 247 */
   56, 57, 250, 251, 252, 253, 254, 255,	/* 248 - 255 */
  };
#else
static Tn5250CharMap *map = NULL;
#endif

  /* Command-line options */
static char *opt_output = NULL;
static char *opt_user =	NULL;
static char *opt_password = NULL;
static char *opt_host = NULL;
static char *opt_file = NULL;
#ifdef HAVE_LIB5250
static char *opt_map = NULL;
#endif

static field_t *fields = NULL;	  /* List of fields. */
static FILE *outf = NULL;	  /* Output file. */
static int file_lrl = 0;	  /* File logical record length. */
static netbuf *nControl = NULL;   /* FTP control stream. */

static void	syntax		PARMS ((void));
static void	get_file	PARMS ((const char *name,
				      int lrl,
				      void (* cb) (Byte *buf)));
static void	get_ffd_data	PARMS ((void));
static void	get_ffd_data_cb PARMS ((Byte *buf));
static void     get_fd_data     PARMS ((void));
static void     get_fd_data_cb  PARMS ((Byte *buf));
static int      signed2int      PARMS ((Byte *buf, int len));
static void     file_data_cb    PARMS ((Byte *buf));

static void	XML_putc	PARMS ((Byte c));
static void	XML_header	PARMS ((void));
static void	XML_footer	PARMS ((void));
static void	XML_record	PARMS ((void));

static void
syntax ()
{
#ifdef HAVE_LIB5250
  Tn5250CharMap *m = tn5250_transmaps;
  int i = 0;
#endif
#ifdef HAVE_GETOPT_LONG
  printf ("asdbget - Get and Translate AS/400 Database Files via FTP.\n\
Syntax:\n\
  asdbget [options] HOST FILE\n\
\n\
Options:\n"
#ifdef HAVE_LIB5250
"  --map, -m MAP                 Use MAP as translation map (default: 37):");
  while (m->name != NULL)
    {
      if (i % 8 == 0)
	printf ("\n                                  ");
      printf ("%s, ", m->name);
      m++; i++;
    }
  printf ("\n"
#endif
"  --output, -o OFILE            Specify output file (default: stdout).\n\
  --user, -u UNAME              Specify the login user (default: %s).\n\
\n", "en", opt_user);
#else
  printf ("asdbget - Get and Translate AS/400 Database Files via FTP.\n\
Syntax:\n\
  asdbget [options] HOST FILE\n\
\n\
Options:\n"
#ifdef HAVE_LIB5250
"  -m MAP                        Use MAP as translation map (default: 37):\n");
  while (m->name != NULL)
    {
      if (i % 8 == 0)
	printf ("\n                                  ");
      printf ("%s, ", m->name);
      m++; i++;
    }
  printf ("\n"
#endif
"  -o OFILE                      Specify output file (default: stdout).\n\
  -u UNAME                      Specify the login user (default: %s).\n\
\n", opt_user);
#endif
  exit (1);
}

/* Retreive a binary image of a file and feed `lrl'-sized blocks through a
 * callback. */
static void
get_file (name, lrl, cb)
	const char *name;
	int lrl;
	void (* cb) ();
{
  netbuf *nData = NULL;
  Byte *buf;
  int i, r;

  if ((buf = (Byte *)malloc (lrl)) == NULL)
    {
      perror ("asdbget");
      exit (1);
    }
  if (!FtpAccess (name, FTPLIB_FILE_READ, FTPLIB_IMAGE, FTPLIB_RECORD, nControl, &nData))
    {
      fprintf (stderr, "asdbget: while trying to retreive '%s':\n%s\n",
	  name, FtpLastResponse (nControl));
      exit (1);
    }

  while ((r = FtpReadRecord (buf, lrl, nData)) >= 0)
    (* cb) (buf);

error_out:
  free (buf);
  FtpClose (nData);

  if (!FtpReadResponse ('2',nControl))
    {
      fprintf (stderr, "%s\n", FtpLastResponse (nControl));
      exit (1);
    }
}

static void
get_ffd_data ()
{
  char rcmdbuf[128];
  sprintf (rcmdbuf, "DSPFFD FILE(%s) OUTPUT(*OUTFILE) \
OUTFILE(QTEMP/FIELDS)", opt_file);
  if (!FtpRcmd (rcmdbuf, nControl))
    {
      fprintf (stderr, "asdbget: While processing RCMD GETFFD:\n%s\n",
	  FtpLastResponse (nControl));
      exit (1);
    }
  get_file ("QTEMP/FIELDS", 605, get_ffd_data_cb);
  FtpDelete ("QTEMP/FIELDS", nControl);
}

static void
get_fd_data ()
{
  char rcmdbuf[128];
  sprintf (rcmdbuf, "DSPFD FILE(%s) TYPE(*ACCPTH) OUTPUT(*OUTFILE) \
OUTFILE(QTEMP/KEYS)", opt_file);
  if (!FtpRcmd (rcmdbuf, nControl))
    {
      fprintf (stderr, "asdbget: While processing RCMD GETFD:\n%s\n",
	  FtpLastResponse (nControl));
      exit (1);
    }
  get_file ("QTEMP/KEYS", 605, get_fd_data_cb);
  FtpDelete ("QTEMP/KEYS", nControl);
}

/* Parse a DSPFFD record and create a new field structure. */
static void
get_ffd_data_cb (buf)
	Byte *buf;
{
  field_t *field;
  int i;

  if ((field = (field_t*)malloc (sizeof (field_t))) == NULL)
    {
      perror ("asdbget");
      exit (1);
    }

  /* WHFLDI */
  for (i = 0; i < 10; i++)
#ifdef HAVE_LIB5250 
    field->name[i] = tn5250_char_map_to_local(map, buf[129+i]);
#else
    field->name[i] = e2a [buf[129+i]];
#endif /* HAVE_LIB5250 */

  field->name[10] = 0;
  for (i = 9; i > 0 && field->name[i] == ' '; i--)
    field->name[i] = 0;

  /* WHFTXT */
  for (i = 0; i < 50; i++)
#ifdef HAVE_LIB5250 
    field->desc[i] = tn5250_char_map_to_local(map, buf[168+i]);
#else
    field->desc[i] = e2a [buf[168+i]];
#endif /* HAVE_LIB5250 */

  field->desc[50] = 0;
  for (i = 49; i > 0 && field->desc[i] == ' '; i--)
    field->desc[i] = 0;

  /* WHFLDT */
#ifdef HAVE_LIB5250 
  field->type = tn5250_char_map_to_local(map, buf[321]);
#else
  field->type = e2a [buf[321]];
#endif /* HAVE_LIB5250 */
  field->buffer_length = signed2int (buf + 159, 5);
  field->precision = signed2int (buf + 166, 2);
  field->buffer_offset = signed2int (buf + 154, 5) - 1;

  field->length = field->buffer_length;
  if (field->type == 'P')
    field->length = field->length * 2 - 1;

  if (field->buffer_offset + field->buffer_length > file_lrl)
    file_lrl = field->buffer_offset + field->buffer_length;

  if ((field->vbuffer = (Byte *)malloc (field->length + 3)) == NULL)
    {
      perror ("asdbget");
      exit (1);
    }

  /* Append field_t structure to end of list of fields. */
  if (fields == NULL)
    fields = field->next = field->prev = field;
  else
    {
      field->next = fields;
      field->prev = fields->prev;
      field->prev->next = field;
      field->next->prev = field;
    }
}

static void
get_fd_data_cb (buf)
	Byte *buf;
{
}

/* Convert a signed decimal field to a C `int' */
static int
signed2int (buf, len)
	Byte *buf;
	int len;
{
  int v = 0;
  while (len)
    {
      v *= 10;
      v += buf[0] & 0x0f;
      len--;
      buf++;
    }
  if ((buf[0] & 0xf0) == 0xd0)
    v = -v;
  return v;
}

/* Unpack decimal to signed numeric (zone-shifted last digit, digits are
 * in ebcdic. */
static void
unpack_decimal (dst,src,len)
	Byte *dst;
	Byte *src;
	int len;
{
  int i;
  for (i = 0; i < len; i++)
    dst[i] = nibble (src, i) | 0xf0;
  if ((nibble (src,len) & 0x0f) == 0x0d)
    dst[len-1] = (dst[len-1] & 0x0f) | 0xd0;
}

/* Translate the record buffer into the fields' value buffers. */
static void
file_data_cb (buf)
	Byte *buf;
{
  field_t *iter;
  Byte *ptr, *data;
  Byte pbuf[32];
  int i, l;

  if ((iter = fields) != NULL)
    {
      do
	{
	  data = buf + iter->buffer_offset;
	  l = iter->buffer_length;
	  ptr = iter->vbuffer;
	  switch (iter->type)
	    {
	    case 'P': /* Packed decimal fields */
	      unpack_decimal (pbuf,data,iter->length); 
	      data = pbuf;
	      l = iter->length;

	      /* FALL THROUGH */

	    case 'S': /* Signed numeric fields */
	      if ((data[l-1] & 0xf0) == 0xd0)
		*ptr++ = '-';
	      i = 0;
	      while (i < l - iter->precision && data[i] == 0xf0)
		i++;
	      if (i == l - iter->precision)
		*ptr++ = '0';
	      while (i < l)
		{
		  if (i == l - iter->precision)
		    *ptr++ = '.';
		  *ptr++ = ((0x0f & data[i]) | 0x30);
		  i++;
		}
	      break;

	    case 'A': /* Alpha-numeric fields */
	      while (data[l-1] == 0x40 && l > 0)
		l--;
	      for (i = 0; i < l; i++)
#ifdef HAVE_LIB5250 
		*ptr++ = tn5250_char_map_to_local (map, data[i]);
#else
		*ptr++ = e2a[data[i]];
#endif /* HAVE_LIB5250 */
	      break;

	    case 'Z': /* Timestamp fields */
	      if (iter->length != 26) {
		fprintf(stderr, "invalid TimeStamp len %d/26", iter->length);
		assert (0);
	      }

	      for (i = 0; i < iter->length; i++)
#ifdef HAVE_LIB5250
		*ptr++ = tn5250_char_map_to_local (map, data[i]);
#else
		*ptr++ = e2a[data[i]];
#endif /* HAVE_LIB5250 */
	    
	      break;
 
	    default:
	      fprintf(stderr, "unknown type %c", (char)iter->type);
	      assert (0);
	    }
	  *ptr = 0;
	  iter = iter->next;
	}
      while (iter != fields);
    }

  XML_record ();
}

int
main (argc, argv)
	int argc;
	char **argv;
{
  int o, loptind;

  outf = stdout;
  opt_user = getenv ("USER");
  opt_password = getenv ("ASDBGET_PASSWORD");
#ifdef HAVE_GETOPT_LONG
  while ((o = getopt_long (argc, argv, shortopts, longopts, &loptind)) != -1)
#else
  while ((o = getopt (argc, argv, shortopts)) != -1)
#endif
    {
      switch (o)
	{
	case 'o':
	  opt_output = optarg;
	  break;

	case 'u':
	  opt_user = optarg;
	  break;

#ifdef HAVE_LIB5250
	case 'm':
	  opt_map = optarg;
	  break;
#endif /* HAVE_LIB5250 */

	default:
	  syntax ();
	}
    }
#ifdef HAVE_LIB5250 
  if (opt_map == NULL)
    opt_map = "37";
  if ((map = tn5250_char_map_new (opt_map)) == NULL)
    syntax ();
#endif /* HAVE_LIB5250 */

  if (optind >= argc)
    syntax ();
  opt_host = argv[optind++];
  if (optind >= argc)
    syntax ();
  opt_file = argv[optind++];
  if (optind != argc)
    syntax ();

  if (opt_output != NULL)
    {
      if ((outf = fopen (opt_output, "w")) == NULL)
	{
	  perror (opt_output);
	  exit (1);
	}
    }

  if (!FtpConnect (opt_host, &nControl))
    {
      fprintf (stderr, "%s\n", FtpLastResponse (nControl));
      exit (1);
    }

  /* Prompt for password if not provided. */
  if (opt_password == NULL)
    {
      struct termios tios_new, tios_old;
      static char pass[78];
      tcgetattr (0, &tios_old);
      memcpy (&tios_new, &tios_old, sizeof (struct termios));
      tios_new.c_lflag &= ~ECHO;
      tios_new.c_lflag |= ICANON;
      fprintf (stderr, "Password for %s@%s: ", opt_user, opt_host);
      fflush (stderr);
      tcsetattr (0, TCSAFLUSH, &tios_new);
      fgets (pass, sizeof(pass)-1, stdin);
      if (strchr (pass, '\n'))
	*strchr (pass, '\n') = '\0';
      tcsetattr (0, TCSAFLUSH, &tios_old);
      fprintf (stderr, "\n");
      fflush (stderr);
      opt_password = pass;
    }

  if (!FtpLogin (opt_user, opt_password, nControl))
    {
      fprintf (stderr, "%s\n", FtpLastResponse (nControl));
      exit (1);
    }

  get_ffd_data ();
  get_fd_data ();

  XML_header ();
  get_file (opt_file, file_lrl, file_data_cb);
  XML_footer ();

  FtpQuit (nControl);
  FtpClose (nControl);
#ifdef HAVE_LIB5250
  tn5250_char_map_destroy (map);
#endif

  if (outf != stdout)
    fclose (outf);
  return 0;
}

/* Output callback for the XML header. */
static void
XML_header ()
{
  field_t *iter;

  fprintf (outf, "<?xml version=\"1.0\"?>\n\
<?xml-stylesheet type=\"text/xsl\" href=\"file:%s/%s/ASDB.xsl\"?>\n\
<!DOCTYPE ASDB SYSTEM \"%s/%s/ASDB.dtd\">\n", DATADIR, PACKAGE, DATADIR, PACKAGE);

  fprintf (outf, "<ASDB VERSION=\"%s\">\n\
  <LAYOUT RECLEN=\"%d\">\n", DTDVERSION, file_lrl);

  if ((iter = fields) != NULL)
    {
      do
	{
	  fprintf (outf, "    <FIELD NAME=\"%s\" TYPE=\"%c\" OFFSET=\"%d\" WIDTH=\"%d\"",
	      iter->name, iter->type, iter->buffer_offset,
	      iter->buffer_length);
	  if (iter->type == 'P' || iter->type == 'S')
	    fprintf (outf, " PRECISION=\"%d\"", iter->precision);
	  fprintf (outf, ">\n      %s\n    </FIELD>\n", iter->desc);
	  iter = iter->next;
	}
      while (iter != fields);
    }
  fprintf (outf, "  </LAYOUT>\n");
}

static void
XML_footer ()
{
  fprintf (outf, "</ASDB>\n");
}

/* Output callback for XML records. */
static void
XML_record ()
{
  field_t *iter;
  int i;

  fprintf (outf,"  <RECORD>\n");
  if ((iter = fields) != NULL)
    {
      do
	{
	  fprintf (outf, "    <DATUM NAME=\"%s\">", iter->name);
	  for (i = 0; iter->vbuffer[i] != 0; i++)
	    XML_putc (iter->vbuffer[i]);
	  
	  fprintf (outf, "</DATUM>\n");
	  iter = iter->next;
	}
      while (iter != fields);
    }
  fprintf (outf, "  </RECORD>\n");
}

/* Write an XML character or produce the appropriate entity as required. */
static void
XML_putc (c)
        Byte c;
{
  switch (c)
    {
    case '&': fputs ("&amp;", outf); break;
    case '<': fputs ("&lt;", outf); break;
    case '>': fputs ("&gt;", outf); break;
    case '"': fputs ("&quot;", outf); break;
    case '\'': fputs ("&apos;", outf); break;
    default:
      fputc (c, outf);
    }
}

/* vi:set ts=8 sts=2 sw=2 cindent cinoptions=^-2,p8,{.75s,f0,>4,n-2,:0: */
