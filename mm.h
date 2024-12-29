#ifndef _EXTRA_H
#define _EXTRA_H

#ifdef __cplusplus
extern "C" {
#endif

void* _mm_malloc(size_t size);
void* _mm_calloc(size_t nitems,size_t size);
BOOL _mm_RWopsReader_Eof(MREADER* reader);
BOOL _mm_RWopsReader_Read(MREADER* reader,void* ptr,size_t size);
int _mm_RWopsReader_Get(MREADER* reader);
BOOL _mm_RWopsReader_Seek(MREADER* reader,long offset,int whence);
long _mm_RWopsReader_Tell(MREADER* reader);
MREADER *_mm_new_rwops_reader(SDL_RWops * rw);
void _mm_delete_rwops_reader (MREADER* reader);

#ifdef __cplusplus
}
#endif

#endif
