#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>

#include "twadbstream.h"
#include "twrpadbfile.h"
#include "crc32.h"
#include "md5.h"

using namespace std;

int main(int argc, char** argv)
{
	TwrpAdbFile *tbf = NULL;

	if (argc != 2)
	{
		cerr << "Expected exactly 1 argument: <input_file>" << endl;
		exit(1);
	}

	char* filename = argv[1];
	cout << "Input file: '" << filename << "'" << endl;

	try {
		tbf = new TwrpAdbFile(filename);
	} catch(...) {
		perror("Failed to open input file");
		exit(1);
	}

	printf("Opened input file:\n");
	printf("\tSize: %zu\n", tbf->GetFileSize());

	tbf->ReadNextBlock();
	if(tbf->GetCurrentBlockType() != TwrpAdbFile::TW_STREAM_HEADER) {
		fprintf(stderr, "Error - backup file doesn't start with a stream header!\n");
		exit(1);
	}
	{
		AdbBackupStreamHeader* twAdbBackupHeader = (AdbBackupStreamHeader*)tbf->GetCurrentBlock();
		printf("TW ADB Backup Header:\n");
		printf("\tType                : %-16.16s\n", twAdbBackupHeader->type);
		printf("\tVersion             : %zu\n", twAdbBackupHeader->version);
		printf("\tNumber of partitions: %zu\n", twAdbBackupHeader->partition_count);

		if(twAdbBackupHeader->version != 3) {
			cerr << "Incompatible backup version "
				<< twAdbBackupHeader->version
				<< ". This program only handles backup versions 3."
				<< endl;
			exit(1);
		}
	}

	// TODO: Toggle this via parameter
	int writeFiles = 1;

	while(tbf->ReadNextBlock())
	{
		switch(tbf->GetCurrentBlockType())
		{
		case TwrpAdbFile::TW_FILENAME:
		case TwrpAdbFile::TW_IMAGE:
			{
			twfilehdr *fh = (twfilehdr*)tbf->GetCurrentBlock();
			printf("\nFile Header:\n");
			printf("\tName: %s\n", fh->name);
			printf("\tType: %s\n", tbf->GetCurrentBlockType() == TwrpAdbFile::TW_IMAGE ? "Image" : "Data");
			printf("\tSize: %zu\n", fh->size);
			printf("\tCompressed: %s\n", fh->compressed?"Yes":"No");
			printf("\n");

			uint64_t fsize = fh->size;
			uint64_t cbReadFile = 0;
			uint64_t cbReadBlock = 0;
			MD5_CTX md5ctx = {0};
			FILE *fpOut = NULL;

			if(writeFiles)
			{
				char *f = strrchr(fh->name, '/');
				if(f) {
					f++; // Skip past '/'
				} else {
					f = fh->name;
				}
				fpOut = fopen(f, "wb");
				if(!fpOut) {
					perror("Could not open output file for writing!");
					exit(1);
				}
			}

			MD5_Init(&md5ctx);
			// Now let's read all the data in the file.
			uint64_t blockNumber = 0;
			tbf->ReadNextBlock();
			while( tbf->GetCurrentBlockType() == TwrpAdbFile::TW_DATA_BLOCK )
			{
				fh = (twfilehdr*)tbf->GetCurrentBlock();
				cbReadBlock = 0;
				blockNumber++;
				printf("Processing Data Block %zu\n", blockNumber);
				printf("\tSize: %zu\n", fh->size);

				while( tbf->ReadNextBlock() && tbf->GetCurrentBlockType() == TwrpAdbFile::TW_FILEDATA)
				{
					MD5_Update(&md5ctx, tbf->GetCurrentBlock(), MAX_ADB_READ);
					cbReadBlock += MAX_ADB_READ;
					if(fpOut) {
						fwrite(tbf->GetCurrentBlock(), 1, MAX_ADB_READ, fpOut);
					}
				}
				printf("\tRead: %zu\n", cbReadBlock); fflush(stdout);
				cbReadFile += cbReadBlock;
			}
			if(fpOut) {
				fclose(fpOut);
			}
			// The next block SHOULD be a TwrpBackupFile::TW_MD5_TRAILER block.
			if( tbf->GetCurrentBlockType() == TwrpAdbFile::TW_MD5_TRAILER )
			{
				AdbBackupFileTrailer *ft = (AdbBackupFileTrailer*)tbf->GetCurrentBlock();
				unsigned char md5sum[16];
				MD5_Final(md5sum, &md5ctx);

				// Convert md5sum to string
				char md5[40] = {0};
				for(int i = 0; i < sizeof(md5sum); i++)
				{
					sprintf(md5+(i*2), "%02x", md5sum[i]);
				}

				int md5check = 0 == memcmp(md5, ft->md5, sizeof(md5));

				printf("\nProcessed File Data:\n");
				printf("\tBytes read: %zu\n", cbReadFile);
				printf("\tMD5 check:  %s\n", md5check?"Yes":"No");
			} else {
				printf("ERROR - unexpected block type after file data: %-16.16s\n", ((char*)tbf->GetCurrentBlock())+8);
				exit(1);
			}
			}
			break;
		case TwrpAdbFile::TW_END_ADB:
			printf("Found end of stream\n");
			break;
		default:
			printf("\nUnhandled block type: %-16.16s\n\n", ((char*)tbf->GetCurrentBlock())+8);
			break;
		}
		fflush(stdout);
	}

	// We -should- be at the end of the file.
	printf("\nFinished processing file:\n");
	printf("\tBytes Processed: %zu\n", tbf->GetCurrentPosition());

	delete tbf;

	return 0;
}


