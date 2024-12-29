/*	MikMod sound library
	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: mmio.c 1819 2006-01-29 06:52:40Z slouken $

  Portable file I/O routines

==============================================================================*/

/*

	The way this module works:

	- _mm_fopen will call the errorhandler [see mmerror.c] in addition to
	  setting _mm_errno on exit.
	- _mm_iobase is for internal use.  It is used by Player_LoadFP to
	  ensure that it works properly with wad files.
	- _mm_read_I_* and _mm_read_M_* differ : the first is for reading data
	  written by a little endian (intel) machine, and the second is for reading
	  big endian (Mac, RISC, Alpha) machine data.
	- _mm_write functions work the same as the _mm_read functions.
	- _mm_read_string is for reading binary strings.  It is basically the same
	  as an fread of bytes.

*/

/* FIXME
	the _mm_iobase variable ought to be MREADER-specific. It will eventually
	become a private field of the MREADER structure, but this will require a
	soname version bump.

	In the meantime, the drawback is that if you use the xxx_LoadFP functions,
	you can't have several MREADER objects with different iobase values.
*/

#include <SDL2/SDL.h>

#include <stdio.h>
#include <string.h>

#include "mikmod_internals.h"

#define COPY_BUFSIZE  512

static long _mm_iobase=0;

/* Same as malloc, but sets error variable _mm_error when fails */
void* _mm_malloc(size_t size)
{
	void *d;

	if(!(d=calloc(1,size))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return d;
}

/* Same as calloc, but sets error variable _mm_error when fails */
void* _mm_calloc(size_t nitems,size_t size)
{
	void *d;

	if(!(d=calloc(nitems,size))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return d;
}

/*

  This section is added to use SDL_rwops for IO

  -Matt Campbell (matt@campbellhome.dhs.org) April 2000

*/

typedef struct MRWOPSREADER {
	MREADER core;
	SDL_RWops* rw;
	int end;
} MRWOPSREADER;

static BOOL _mm_RWopsReader_Eof(MREADER* reader)
{
	if ( ((MRWOPSREADER*)reader)->end <
			SDL_RWtell(((MRWOPSREADER*)reader)->rw) ) return 1;
	else return 0;
}

static BOOL _mm_RWopsReader_Read(MREADER* reader,void* ptr,size_t size)
{
	return SDL_RWread(((MRWOPSREADER*)reader)->rw, ptr, size, 1);
}

static int _mm_RWopsReader_Get(MREADER* reader)
{
	char buf;
	if ( SDL_RWread(((MRWOPSREADER*)reader)->rw, &buf, 1, 1) != 1 ) return EOF;
	else return (int)buf;
}

static BOOL _mm_RWopsReader_Seek(MREADER* reader,long offset,int whence)
{
	return SDL_RWseek(((MRWOPSREADER*)reader)->rw,
			(whence==SEEK_SET)?offset+_mm_iobase:offset,whence);
}

static long _mm_RWopsReader_Tell(MREADER* reader)
{
	return SDL_RWtell(((MRWOPSREADER*)reader)->rw) - _mm_iobase;
}

MREADER *_mm_new_rwops_reader(SDL_RWops * rw)
{
	int here;
	MRWOPSREADER* reader=(MRWOPSREADER*)_mm_malloc(sizeof(MRWOPSREADER));
	if (reader) {
		reader->core.Eof =&_mm_RWopsReader_Eof;
		reader->core.Read=&_mm_RWopsReader_Read;
		reader->core.Get =&_mm_RWopsReader_Get;
		reader->core.Seek=&_mm_RWopsReader_Seek;
		reader->core.Tell=&_mm_RWopsReader_Tell;
		reader->rw=rw;

		/* RWops does not explicitly support an eof check, so we shall find
		   the end manually - this requires seek support for the RWop */
		here = SDL_RWtell(rw);
		reader->end = SDL_RWseek(rw, 0, SEEK_END);
		SDL_RWseek(rw, here, SEEK_SET);   /* Move back */
	}
	return (MREADER*)reader;
}

void _mm_delete_rwops_reader (MREADER* reader)
{
	if(reader) free(reader);
}

/*

  End SDL_rwops section

*/


/* ex:set ts=4: */
