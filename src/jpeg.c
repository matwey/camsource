#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#undef HAVE_STDLIB_H

#include "config.h"

#include "jpeg.h"
#include "image.h"
#define MODULE_GENERIC
#include "module.h"

struct jpeg_ctx
{
	struct jpeg_destination_mgr jdm;
	char *buf;
	int bufsize;
};

char *name = "jpeg_comp";
char *deps[] = { NULL };
pthread_t tid;

void
j_id(struct jpeg_compress_struct *cinfo)
{
}

boolean
j_eob(struct jpeg_compress_struct *cinfo)
{
	struct jpeg_ctx *jctx;
	int filled;
	
	jctx = (struct jpeg_ctx *) cinfo;
	filled = jctx->bufsize - jctx->jdm.free_in_buffer;
	jctx->buf = realloc(jctx->buf, jctx->bufsize * 2);
	jctx->jdm.next_output_byte = jctx->buf + filled;
	jctx->jdm.free_in_buffer += jctx->bufsize;
	jctx->bufsize *= 2;
	
	return TRUE;
}

void
j_td(struct jpeg_compress_struct *cinfo)
{
}

void
jpeg_compress(struct jpegbuf *dst, const struct image *src)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row;
	int rownum;
	struct jpeg_ctx jctx;
	
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	
	jctx.bufsize = src->bufsize;
	jctx.buf = malloc(jctx.bufsize);
	jctx.jdm.next_output_byte = jctx.buf;
	jctx.jdm.free_in_buffer = jctx.bufsize;
	jctx.jdm.init_destination = j_id;
	jctx.jdm.empty_output_buffer = j_eob;
	jctx.jdm.term_destination = j_td;
	
	cinfo.dest = &jctx.jdm;
	
	cinfo.image_width = src->x;
	cinfo.image_height =	src->y;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);

	jpeg_start_compress(&cinfo, TRUE);
	
	for (rownum = 0; rownum < src->y; rownum++)
	{
		row = src->buf + (rownum * src->x * 3);
		jpeg_write_scanlines(&cinfo, &row, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	
	dst->bufsize = jctx.bufsize - jctx.jdm.free_in_buffer;
	dst->buf = malloc(dst->bufsize);
	memcpy(dst->buf, jctx.buf, dst->bufsize);
	
	free(jctx.buf);
}

