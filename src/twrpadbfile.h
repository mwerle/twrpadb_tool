/* This class abstracts a TWRP ADB Backup File.
 *
 * It is designed to allow a client program to load and parse such a file
 * primarily in order to extract the contents.
 *
 * This version only supports TWRP ADB Backup File Format v3.
*/

#ifndef TWRPBACKUPFILE_H
#define TWRPBACKUPFILE_H

class TwrpAdbFile
{
public:
	enum BlockType {
		TW_UNSET,
		TW_INVALID,
		/** The following are all block header types */
		TW_STREAM_HEADER,
		TW_FILENAME,
		TW_IMAGE,
		TW_END_OF_FILE, /**< Supposedly end-of file/image? */
		TW_MD5_TRAILER, /**< Trailer block after File/Image */
		TW_DATA_BLOCK,  /**< File/Image start of data block */
		TW_END_ADB,     /**< End of ADB stream. */
		/* The following are file data block types */
		TW_FILEDATA,
	};

	TwrpAdbFile(const char *fname);
	virtual ~TwrpAdbFile();

	size_t GetFileSize() { return fsize; }
	size_t GetCurrentPosition() { return currPos; }

	/** Reads the next block from the file */
	int ReadNextBlock();

	/** Returns the block type of the current block */
	BlockType GetCurrentBlockType();

	/** Returns the actual raw block */
	void *GetCurrentBlock();

	/** Reads data for a TW_DATA_BLOCK header; at most DATA_MAX_CHUNK_SIZE bytes are read. 
	 * \param[out]       pbData  - a buffer to hold the data
	 * \param[in,out]    cbData  - on input, the size of `pbData`, on output, the actual amount of data read
	*/
	int ReadChunk(char *pbData, size_t *cbData);

	int IsEndOfFile() { return currPos == fsize; }

private:
	char *fname;
	FILE *fp;
	size_t fsize;
	size_t currPos;

	char *currBlock;
	BlockType currBlockType;
};

#endif
