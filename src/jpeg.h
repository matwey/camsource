#ifndef _JPEG_H_
#define _JPEG_H_

struct image;

struct jpegbuf
{
	char *buf;
	unsigned int bufsize;
};

/* Compresses an image and returns a buffer to the jpeg data. Caller must
 * free(jpegbuf->buf). Third arg is jpeg quality (1-100). If given as 0, a
 * default value is used.
 */
void jpeg_compress(struct jpegbuf *, const struct image *, int);

#endif

