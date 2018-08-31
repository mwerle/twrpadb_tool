#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>

#include "twadbstream.h"
#include "twrpadbfile.h"

using namespace std;

#if defined(_MSC_VER)
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

TwrpAdbFile::TwrpAdbFile(const char *filename)
{
	// Try to open file
	fp = fopen(filename, "rb");
	if (!fp) {
		cerr << "Could not open output file '" << filename << "'" << endl;
		throw;
	}

	// Grab the file size
	fseek(fp, 0L, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	currPos = 0;

	// Keep a copy of the filename and allocate memory
	fname = STRDUP(filename);
	if (!fname) {
		cerr << "Could not allocate filename memory" << endl;
		throw;
	}

	currBlock = (char*)malloc(MAX_ADB_READ);
	if (!currBlock) {
		cerr << "Could not allocate buffer memory" << endl;
		throw;
	}
}

TwrpAdbFile::~TwrpAdbFile()
{
	free(currBlock);
	free(fname);
	fclose(fp);
}

int TwrpAdbFile::ReadNextBlock()
{
	size_t bytesRead;
	
	currBlockType = TW_UNSET;
	bytesRead = fread(currBlock, 1, MAX_ADB_READ, fp);
	currPos += bytesRead;
	if(currPos > fsize) {
		cerr << "ERROR: read past end of file!" << endl;
		throw;
	}
	if(bytesRead != MAX_ADB_READ) {
		cerr << "ERROR: could not read enough bytes for block" << endl;
		return 0;
	}

	return 1;
}

int TwrpAdbFile::ReadChunk(char *pbData,size_t *cbData)
{
	size_t numRead = 0;

	if (!pbData || !cbData) {
		return -1;
	}
	if(GetCurrentBlockType() != TW_DATA_BLOCK) {
		*cbData = 0;
		return -1;
	}

	*cbData = 0;
	while(*cbData < (DATA_MAX_CHUNK_SIZE - MAX_ADB_READ)
		&& ReadNextBlock()
		&& GetCurrentBlockType() == TW_FILEDATA)
	{
		memcpy(pbData + *cbData, currBlock, MAX_ADB_READ);
		*cbData += MAX_ADB_READ;
	}

	// If *cbData == DATA_MAX_CHUNK_SIZE, then according to spec, 
	// the next block MUST be another header. But we'll leave that
	// for upstream logic to figure out.
	return 1;
}

TwrpAdbFile::BlockType TwrpAdbFile::GetCurrentBlockType()
{
	// First 8 bytes of block should be the magic number
	// Next 16 bytes of block should be the block type
	twfilehdr *fh = (twfilehdr*)currBlock;

	//
	// TODO: CRC verification!
	//

	// Optimisation; prevent recalculating CRC
	if(currBlockType != TW_UNSET) {
		goto done;
	}

	if(strncmp(fh->start_of_header, TWRP, sizeof(TWRP)-1) == 0) {
		// Assume it's a header block, so verify the CRC32
		//uint32_t crc;
		//crc32(currBlock, MAX_ADB_READ, &crc);

		/* Unfortunately we can't check this here since the crc is at
		 * different offsets for the different block types. Who designed
		 * this?! */
		/*
		if(crc != ct->crc) {
			// CRC doesn't match, so let's assume it's a data block
			currBlockType = TW_FILEDATA;
			goto done;
		}
		*/

		if(strncmp(fh->type,TWSTREAMHDR,sizeof(TWSTREAMHDR)-1) == 0) {
			//AdbBackupStreamHeader *sh = (AdbBackupStreamHeader*)currBlock;
			//currBlockType = crc == sh->crc ? TW_STREAM_HEADER : TW_DATA_BLOCK;
			currBlockType = TW_STREAM_HEADER;
			goto done;
		}
		if(strncmp(fh->type,TWFN,sizeof(TWFN)-1) == 0) {
			currBlockType = TW_FILENAME;
			goto done;
		}
		if(strncmp(fh->type,TWIMG,sizeof(TWIMG)-1) == 0) {
			currBlockType = TW_IMAGE;
			goto done;
		}
		if(strncmp(fh->type,TWEOF,sizeof(TWEOF)-1) == 0) {
			currBlockType = TW_END_OF_FILE;
			goto done;
		}
		if(strncmp(fh->type,MD5TRAILER,sizeof(MD5TRAILER)-1) == 0) {
			currBlockType = TW_MD5_TRAILER;
			goto done;
		}
		if(strncmp(fh->type,TWDATA,sizeof(TWDATA)-1) == 0) {
			currBlockType = TW_DATA_BLOCK;
			goto done;
		}
	}

	// Otherwise let's assume it's data
	currBlockType = TW_FILEDATA;
done:
	return currBlockType;
}

void *TwrpAdbFile::GetCurrentBlock()
{
	return currBlock;
}
