#include "stdafx.h"
#include "TH14Unpacker.h"


TH14Unpacker::TH14Unpacker(FILE* _f)
: THUnpackerBase(_f)
{
	dirName = "th14";
}

void TH14Unpacker::readHeader()
{
	DWORD header[4];
	header[0] = 0xB4B35A13;
	fread(&header[1], 1, 12, f);
	// decrypt
	thDecrypt((BYTE*)header, 16, 27, 55, 16, 16);
	count = header[3] - 135792468;
	indexAddress = fileSize - (header[2] - 987654321);
	originalIndexSize = header[1] - 123456789;
}

void TH14Unpacker::readIndex()
{
	fseek(f, indexAddress, SEEK_SET);
	DWORD indexSize = fileSize - indexAddress;
	BYTE* indexBuffer = new BYTE[indexSize];
	fread(indexBuffer, 1, indexSize, f);
	// decrypt
	thDecrypt(indexBuffer, indexSize, 62, -101, 128, indexSize);
	// uncompress
	BYTE* result = thUncompress(indexBuffer, indexSize, NULL, originalIndexSize);
	delete indexBuffer;
	indexBuffer = result;
	indexSize = originalIndexSize;

	// format index
	formatIndex(index, indexBuffer, count, indexAddress);
	delete indexBuffer;
}

void TH14Unpacker::formatIndex(vector<Index>& index, const BYTE* indexBuffer, int fileCount, DWORD indexAddress)
{
	index.resize(fileCount);
	int i;
	for (i = 0; i < fileCount; i++)
	{
		index[i].name = (char*)indexBuffer;
		DWORD offset = strlen((char*)indexBuffer) + 1;
		if (offset % 4 != 0)
			offset += 4 - offset % 4;
		indexBuffer += offset;
		index[i].address = ((DWORD*)indexBuffer)[0];
		index[i].originalLength = ((DWORD*)indexBuffer)[1];
		indexBuffer += 12;

		printf("%30s  %10d  %10d\n", index[i].name.c_str(), index[i].address, index[i].originalLength);
	}
	for (i = 0; i < fileCount - 1; i++)
		index[i].length = index[i + 1].address - index[i].address;
	index[i].length = indexAddress - index[i].address;
}

// see th14.exe.004647F1
bool TH14Unpacker::onUncompress(const Index& index, BYTE*& buffer, DWORD& size)
{
	// see th14.exe.004D1DA0
	static const BYTE decParam[] = {
		0x1B, 0x73, 0xAA, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x12, 0x43, 0xFF, 0x00,
		0x00, 0x02, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x35, 0x79, 0x11, 0x00, 0x00, 0x04, 0x00, 0x00,
		0x00, 0x3C, 0x00, 0x00, 0x03, 0x91, 0xDD, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
		0xAB, 0xDC, 0xEE, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x51, 0x9E, 0xBB, 0x00,
		0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xC1, 0x15, 0xCC, 0x00, 0x00, 0x04, 0x00, 0x00,
		0x00, 0x2C, 0x00, 0x00, 0x99, 0x7D, 0x77, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00
	};

	BYTE asciiSum = 0;
	for (const char c : index.name)
		asciiSum += (BYTE)c;
	BYTE paramIndex = (asciiSum & 0x07) * 3;

	thDecrypt(
		buffer,
		size,
		(decParam + 0x0)[4 * paramIndex],
		(decParam + 0x1)[4 * paramIndex],
		*(int*)&(decParam + 0x4)[4 * paramIndex],
		*(int*)&(decParam + 0x8)[4 * paramIndex]);

	return size != index.originalLength;
}
