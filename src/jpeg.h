#ifndef _JPEG_H_
#define _JPEG_H_

struct image;

struct jpegbuf
{
	char *buf;
	unsigned int bufsize;
};

void jpeg_compress(struct jpegbuf *, const struct image *, int);

#endif

