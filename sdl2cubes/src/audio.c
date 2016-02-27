#include <vorbis/vorbisfile.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"

typedef struct AudioReader {
    OggVorbis_File vorbisFile;
    FILE *file;
} AudioReader;

/*typedef struct {
  size_t (*read_func)  (void *ptr, size_t size, size_t nmemb, void
*datasource);
  int    (*seek_func)  (void *datasource, ogg_int64_t offset, int whence);
  int    (*close_func) (void *datasource);
  long   (*tell_func)  (void *datasource);
} ov_callbacks;*/

size_t vbRead(void *dest, size_t size, size_t count, void *data) {
    AudioReader *reader = (AudioReader *)data;
    return fread(dest, size, count, reader->file);
}

int vbSeek(void *data, ogg_int64_t offset, int whence) {
    AudioReader *reader = (AudioReader *)data;
    return fseek(reader->file, offset, whence);
}

long vbTell(void *data) {
    AudioReader *reader = (AudioReader *)data;
    return ftell(reader->file);
}

struct AudioReader *arInit(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if(!file) {
        return NULL;
    }
    AudioReader *reader = (AudioReader *)malloc(sizeof(AudioReader));
    memset(reader, 0, sizeof(AudioReader));
    reader->file = file;
    // close can be omitted if we handle that
    ov_callbacks cb = {vbRead, vbSeek, NULL, vbTell};
    int res = ov_open_callbacks(reader, &reader->vorbisFile, NULL, 0, cb);
    if(res) {
        fprintf(stderr, "error: can't open Vorbis file %s: %i\n", filename,
                res);
        free(reader);
        return NULL;
    }
    int numStreams = ov_streams(&reader->vorbisFile);
    if(numStreams != 1) {
        printf("warning: multiple streams in file %s\n", filename);
    }
    printf("opened Vorbis file %s\n", filename);
    return reader;
}

void arClose(AudioReader *ar) {
    if(ar) {
        if(ar->file) {
            fclose(ar->file);
            ar->file = NULL;
        }
        ov_clear(&ar->vorbisFile);
        free(ar);
    }
}

int arGetRate(AudioReader *ar) {
    vorbis_info *info = ov_info(&ar->vorbisFile, -1);
    return info->rate;
}

int arGetChannels(AudioReader *ar) {
    vorbis_info *info = ov_info(&ar->vorbisFile, -1);
    return info->channels;
}

int arGetSampleBits(AudioReader *ar) {
    (void)ar; // pretend we do something with ar
    return 2;
}

int arRead(AudioReader *ar, unsigned char *dest, int len) {
    int ofs       = 0;
    int bitstream = 0;
    long result   = 0;
    do {
        char *ptr = (char *)dest + ofs;
        result = ov_read(&ar->vorbisFile, ptr, len - ofs, 0, 2, 1, &bitstream);
        if(result == OV_HOLE) {
            fprintf(stderr, "warning: Vorbis bitstream changed.");
            break;
        }
        if(result == OV_EBADLINK || result == OV_EINVAL) {
            fprintf(stderr, "error: Vorbis file is corrupted.");
            return 0;
        }
        ofs += result;
    } while(result && (len - ofs));
    return ofs;
}
