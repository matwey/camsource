#ifndef _IMAGE_H_
#define _IMAGE_H_

struct image
{
	unsigned int x, y;
	unsigned int bufsize;
	unsigned char *buf;
};

/* Inits image struct and allocates buf */
void image_new(struct image *, unsigned int, unsigned int);

/* Copies image with contents (= image_dup & memcpy(buf, buf)) */
void image_copy(struct image *, const struct image *);

/* Only duplicates the structure and buffer, not the contents of the buffer */
void image_dup(struct image *, const struct image *);

/* Moves the buffer and structure over to another var, and frees the old var and buf */
void image_move(struct image *, const struct image *);

/* Frees buffer */
void image_destroy(struct image *);

#endif

