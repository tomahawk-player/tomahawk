#ifndef _MD5_H_
#define _MD5_H_


#ifdef _MSC_VER
#include <msinttypes/stdint.h>
#else
#include <stdint.h>
#endif


typedef struct {
	uint32_t total[2];
	uint32_t state[4];
	uint8_t buffer[64];
} md5_context;
 
void md5_starts(md5_context *ctx);
void md5_update(md5_context *ctx, const uint8_t *input, uint32_t length);
void md5_finish(md5_context *ctx, uint8_t digest[16]);

#endif /* _MD5_H_ */
