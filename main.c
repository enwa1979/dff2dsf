#include "stdio.h"
#include "string.h"
#include "malloc.h"

#define OK (0)
#define ERROR (-1)

#define FAILED(res) (res < 0)
#define SUCCEEDED(res) (res >= 0)

#define DFF_CHUNK_FUNC(x) dff_property_##x
#define CHANNEL_BLOCKSIZE 4096

#ifndef __WIN32__
#define __int8 char
#define __int16 short
#define __int32 int
#endif


int convert_endian(void *input, size_t s)
{
    size_t i;
    char *temp;

    if ((temp = (char *)calloc( s, sizeof(char))) == NULL)
	{
        return ERROR;
    }

    for (i = 0; i < s; ++i)
	{
        temp[i] = ((char *)input)[i];
    }

    for (i = 1; i <= s; ++i)
	{
        ((char *)input)[i-1] = temp[s-i];
    }

    free(temp);

    return OK;
}

unsigned __int8 reverse_bits(unsigned __int8 src)
{
	unsigned __int8 dst;

	dst = (src & 0x80) >> 7;
	dst |= (src & 0x40) >> 5;
	dst |= (src & 0x20) >> 3;
	dst |= (src & 0x10) >> 1;
	dst |= (src & 0x08) << 1;
	dst |= (src & 0x04) << 3;
	dst |= (src & 0x02) << 5;
	dst |= (src & 0x01) << 7;

	return dst;
}

typedef struct
{
	unsigned __int8 dsd_version[4];
	unsigned __int32 sampling_rate;
	unsigned __int16 channel_number;
	unsigned __int32 sound_offset;
	unsigned __int32 sound_size[2];
} dff_property_t;

int DFF_CHUNK_FUNC(FRM8)(dff_property_t *property, FILE *fp)
{
	char tmp[4];

	// size
	fseek(fp, 8, SEEK_CUR);

	// DSD
	fread(tmp, 1, 4, fp);
	if (memcmp(tmp, "DSD ", 4)) return ERROR;

	return OK;
}

int DFF_CHUNK_FUNC(FVER)(dff_property_t *property, FILE *fp)
{
	// size
	fseek(fp, 8, SEEK_CUR);

	// dsd version
	fread(property->dsd_version, 1, 4, fp);

	return OK;
}

int DFF_CHUNK_FUNC(PROP)(dff_property_t *property, FILE *fp)
{
	char tmp[4];

	// size
	fseek(fp, 8, SEEK_CUR);

	// SND
	fread(tmp, 1, 4, fp);
	if (memcmp(tmp, "SND ", 4)) return ERROR;

	return OK;
}

int DFF_CHUNK_FUNC(FS)(dff_property_t *property, FILE *fp)
{
	// size
	fseek(fp, 8, SEEK_CUR);

	// sampling rate
	fread(&property->sampling_rate, 4, 1, fp);
	convert_endian(&property->sampling_rate, 4);

	return OK;
}

int DFF_CHUNK_FUNC(CHNL)(dff_property_t *property, FILE *fp)
{
	char tmp[4];

	// size
	fseek(fp, 8, SEEK_CUR);

	// channel number
	fread(&property->channel_number, 2, 1, fp);
	convert_endian(&property->channel_number, 2);

	// SLFT
	fread(tmp, 1, 4, fp);
	if (memcmp(tmp, "SLFT", 4)) return ERROR;

	// SRGT
	fread(tmp, 1, 4, fp);
	if (memcmp(tmp, "SRGT", 4)) return ERROR;

	return OK;
}

int DFF_CHUNK_FUNC(CMPR)(dff_property_t *property, FILE *fp)
{
	char tmp[4];
	unsigned __int8 namesize;
	int i;

	// size
	fseek(fp, 8, SEEK_CUR);

	// DSD
	fread(tmp, 1, 4, fp);
	if (memcmp(tmp, "DSD ", 4)) return ERROR;

	// compression name size
	fread(&namesize, 1, 1, fp);

	// compression name
	fseek(fp, namesize, SEEK_CUR);

	// padding(4byte boundary)
	i = (1 + namesize) % 4;
	if (i) fseek(fp, 4 - i, SEEK_CUR);

	return OK;
}

int DFF_CHUNK_FUNC(ABSS)(dff_property_t *property, FILE *fp)
{
	// size
	fseek(fp, 8, SEEK_CUR);

	fseek(fp, 8, SEEK_CUR);

	return OK;
}

int DFF_CHUNK_FUNC(DSD)(dff_property_t *property, FILE *fp)
{
	// size
	fread(property->sound_size, 4, 2, fp);
	convert_endian(&property->sound_size, 8);

	property->sound_offset = ftell(fp);
	fseek(fp, property->sound_size[0], SEEK_CUR);

	return OK;
}

int DFF_CHUNK_FUNC(COMT)(dff_property_t *property, FILE *fp)
{
	unsigned __int32 size[2];

	// size
	fread(size, 4, 2, fp);
	convert_endian(size, 8);

	fseek(fp, size[0], SEEK_CUR);

	/*
	// num
	fseek(fp, 2, SEEK_CUR);

	// date
	fseek(fp, 6, SEEK_CUR);

	// text size
	fread(&size, 4, 1, fp);

	// text
	property->comment = (char*)malloc(size[0] + 1);
	fread(property->comment, 1, size[0], fp);
	property->comment[size[0]] = '\0';
	*/

	return OK;
}

int DFF_CHUNK_FUNC(DIIN)(dff_property_t *property, FILE *fp)
{
	unsigned __int32 size[2];

	// size
	fread(size, 4, 2, fp);
	convert_endian(size, 8);

	fseek(fp, size[0], SEEK_CUR);

	return OK;
}

struct
{
  char *name;
  int (*func)(dff_property_t *property, FILE *fp);
} dff_chunk_list[] =
{
	"FRM8", DFF_CHUNK_FUNC(FRM8),
	"FVER", DFF_CHUNK_FUNC(FVER),
	"PROP", DFF_CHUNK_FUNC(PROP),
	"FS  ", DFF_CHUNK_FUNC(FS),
	"CHNL", DFF_CHUNK_FUNC(CHNL),
	"CMPR", DFF_CHUNK_FUNC(CMPR),
	"ABSS", DFF_CHUNK_FUNC(ABSS),
	"DSD ", DFF_CHUNK_FUNC(DSD),
	"COMT", DFF_CHUNK_FUNC(COMT),
	"DIIN", DFF_CHUNK_FUNC(DIIN)
};

int dff2dsf(const char *in, const char *out)
{
	unsigned __int32 buf_size;
	unsigned __int8 *src_buf;
	unsigned __int8 *dst_buf;
	unsigned __int32 tmp;
	FILE *in_fp, *out_fp;
	dff_property_t dff_property;

	// input and output files are the same
	if (!strcmp(in, out)) return ERROR;

	/*
	dff_property.comment = NULL;
	*/

	if ((in_fp = fopen(in, "rb")) == NULL)
	{
		printf("file open error!! => '%s'\n", in);
		return ERROR;
	};

	// parse chunks
	while (1)
	{
		int i = sizeof(dff_chunk_list) / sizeof(dff_chunk_list[0]) - 1;
		char type[4];

		if (4 > fread(type, 1, 4, in_fp)) break;

		for (; 0 <= i; --i)
		{
			if (memcmp(dff_chunk_list[i].name, type, 4))
			{
				if (0 == i)
				{
					fclose(in_fp);
					return ERROR;
				}
				continue;
			}
			if (FAILED(dff_chunk_list[i].func(&dff_property, in_fp)))
			{
				fclose(in_fp);
				return ERROR;
			}
			break;
		}
	}

	// output
	if ((out_fp = fopen(out, "wb")) == NULL)
	{
		printf("file open error!! => '%s'\n", out);
		fclose(in_fp);
		return ERROR;
	};

	/* DSD chunk */
	fwrite("DSD ", 1, 4, out_fp);
	// chunk size
	tmp = 0x0000001C;
	fwrite(&tmp, 4, 1, out_fp);
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);
	// file size
	tmp = dff_property.sound_size[0] + 92;
	fwrite(&tmp, 4, 1, out_fp);
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);
	// meta data offset
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);

	/* fmt chunk */
	fwrite("fmt ", 1, 4, out_fp);
	// chunk size
	tmp = 0x00000034;
	fwrite(&tmp, 4, 1, out_fp);
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);
	// format version
	tmp = 0x00000001;
	fwrite(&tmp, 4, 1, out_fp);
	// format ID
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);
	// channel type
	tmp = 2;
	fwrite(&tmp, 4, 1, out_fp);
	// channel number
	tmp = dff_property.channel_number;
	fwrite(&tmp, 4, 1, out_fp);
	// sampling rate
	tmp = dff_property.sampling_rate;
	fwrite(&tmp, 4, 1, out_fp);
	// sample bit
	tmp = 0x00000001;
	fwrite(&tmp, 4, 1, out_fp);
	// sample number
	tmp = dff_property.sound_size[0] * (8 / dff_property.channel_number);
	fwrite(&tmp, 4, 1, out_fp);
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);
	// channel block size
	tmp = CHANNEL_BLOCKSIZE;
	fwrite(&tmp, 4, 1, out_fp);
	// reverse
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);

	/* data chunk */
	fwrite("data", 1, 4, out_fp);
	// chunk size
	tmp = dff_property.sound_size[0] + 12;
	fwrite(&tmp, 4, 1, out_fp);
	tmp = 0x00000000;
	fwrite(&tmp, 4, 1, out_fp);
	// sound data
	buf_size = CHANNEL_BLOCKSIZE * dff_property.channel_number;
	src_buf = (unsigned __int8*)malloc(buf_size);
	dst_buf = (unsigned __int8*)malloc(buf_size);

	fseek(in_fp, dff_property.sound_offset, SEEK_SET);
	while (1)
	{
		unsigned __int8 *src_buf_offset = src_buf;
		unsigned __int32 channel = 0, dst_offset = 0;

		unsigned __int32 size = fread(src_buf, 1, buf_size, in_fp);
		
		if (!size) break;

		// zero out(at last)
		if (size < buf_size)
		{
			memset(dst_buf, 0, buf_size);
		}

		size /= dff_property.channel_number;

		while (size > dst_offset)
		{
			dst_buf[dst_offset + CHANNEL_BLOCKSIZE * channel] = reverse_bits(*src_buf_offset);
			++channel;

			++src_buf_offset;
			if (channel >= dff_property.channel_number)
			{
				++dst_offset;
				channel = 0;
			}
		}
		fwrite(dst_buf, 1, buf_size, out_fp);
	}
	
	/*
	if (NULL != dff_property.comment) free(dff_property.comment);
	*/
	free(src_buf);
	free(dst_buf);
	fclose(in_fp);
	fclose(out_fp);

	return OK;
}

int main(int argc, char *argv[])
{
	for (--argc; 0 < argc; --argc)
	{
		char *in = argv[argc], *out;
		size_t len = strlen(in), extpos = len - 1;

		// output file
		while (0 < extpos && '/' != in[extpos] && '.' != in[extpos]) extpos--;
		if ('.' != in[extpos]) // extension not found
		{
			extpos = len;
		}
		out = (char*)malloc(extpos + 8);
		memcpy(out, in, extpos);
		strcpy(out + extpos, ".dsf");

		// convert
		dff2dsf(in, out);

		free(out);
	}

	return 0;
}
