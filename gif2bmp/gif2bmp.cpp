/*
 ***********************************************************************
 *
 *        @version  1.0
 *        @date     03/11/2014 02:09:16 PM
 *        @author   Fridolin Pokorny <fridex.devel@gmail.com>
 *
 ***********************************************************************
 */

#include <cstdio>
#include <cstring>
#include <stdexcept>

#include "gif2bmp.h"
#include "common.h"
#include "gif.h"

const int kControlCode			= -1;
const int kEndOfImage			= -2;
const int kBMPHeaderSize		= 14;
const int kBMPDIPHeaderSize	= 40;

const int kMaxFileNameSize		= 512;

#define MASK(X)			((1 << (X)) - 1)
#define IS_CC(X)			(X[0] == kControlCode)
#define IS_EOI(X)			(X[0] == kEndOfImage)


/**
 * @brief  Generate BMP image
 *
 * @param sizeo output size of BMP image
 * @param gif Gif from which BMP should be generated
 * @param indexes Decoded indexes to color table
 * @param color_table used color table
 * @param out_file output file to write to
 *
 * @return  true on success
 */
static inline
bool generate_bmp(size_t & sizeo, const Gif * gif, const std::vector<uint8_t> * indexes, const std::vector<Gif::color_item_t> * color_table, FILE * out_file) {
	/*
	 * Header
	 */
	fwrite("BM", 1, 2, out_file);
	uint32_t size = kBMPHeaderSize + kBMPDIPHeaderSize + indexes->size() * 3 + 4;
	sizeo = size;
	fwrite(&size, 4, 1, out_file);
	uint16_t tmp16 = 0;
	uint32_t tmp32 = 0;
	fwrite(&tmp32, 4, 1, out_file);
	tmp32 = kBMPHeaderSize + kBMPDIPHeaderSize;
	fwrite(&tmp32, 4, 1, out_file);
	/*
	 * DIP Header
	 */
	tmp32 = kBMPDIPHeaderSize;
	fwrite(&tmp32, 4, 1, out_file);
	tmp32 = gif->m_header.screen_width;
	fwrite(&tmp32, 4, 1, out_file);
	tmp32 = gif->m_header.screen_height;
	fwrite(&tmp32, 4, 1, out_file);
	tmp16 = 1; // plane
	fwrite(&tmp16, 2, 1, out_file);
	tmp16 = 24;
	fwrite(&tmp16, 2, 1, out_file);
	tmp32 = 0;
	fwrite(&tmp32, 4, 1, out_file);
	tmp16 = (indexes->size() * 3); // raw size
	{
		// padding to 4bytes
		size_t w = (3 * gif->m_header.screen_width) & 0x2;
		for (size_t i = 0; i < 4 - w; ++i) {
			tmp16++;
			//uint8_t p = 0;
			//fwrite(&p, 1, 1, out_file);
		}

	}
	fwrite(&tmp16, 4, 1, out_file);
	tmp32 = 2835; // print resolution
	fwrite(&tmp32, 4, 1, out_file);
	tmp32 = 2835; // print resolution
	fwrite(&tmp32, 4, 1, out_file);
	tmp32 = 0; // number of colors in palette
	fwrite(&tmp32, 4, 1, out_file);
	tmp32 = 0; // number of colors in palette
	fwrite(&tmp32, 4, 1, out_file);

	for (size_t i = 1; i <= gif->m_header.screen_height; ++i) {
		for (size_t j = 0; j < gif->m_header.screen_width; ++j) {
			try {
			fwrite(&color_table->at(indexes->at((gif->m_header.screen_height - i)
								* gif->m_header.screen_width + j)).data.blue, 1, 1, out_file);
			fwrite(&color_table->at(indexes->at((gif->m_header.screen_height - i)
								* gif->m_header.screen_width + j)).data.green, 1, 1, out_file);
			fwrite(&color_table->at(indexes->at((gif->m_header.screen_height - i)
								* gif->m_header.screen_width + j)).data.red, 1, 1, out_file);
			} catch (std::out_of_range e) {
				UNUSED(e);
				warn() << "Wrong index to color table, using black color!\n";
				fwrite("\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 9, out_file);
			}
		}

		size_t w = (3 * gif->m_header.screen_width) & 0x3;
		for (size_t j = 0; j < 4 - w && w != 0; ++j) {
			uint8_t p = 0x00;
			fwrite(&p, 1, 1, out_file);
		}
	}

	return true;
}

/**
 * @brief  Get nearest log2 from index
 *
 * @param index num to log2 from
 *
 * @return   log2 of num
 */
static inline
int log2up(int index) {
	int log = 0;
	int idx_bac = index;
	while (index >>= 1) ++log;
	if ((1 << log) <= idx_bac) ++log;
	return log;
}

/**
 * @brief  Get index from byte stream
 *
 * @param idx output index
 * @param start start index
 * @param idx_size size of the index in (num of bits)
 * @param byte1 first byte use
 * @param byte2 second byte to use
 * @param byte3 third byte to use
 *
 * @return number of bytes that can be discarded (1 => byte1, 2=> byte2)
 */
static inline
int getIdx(size_t & idx, size_t & start, size_t idx_size,
        uint8_t byte1, uint8_t byte2, uint8_t byte3 = 0)
{
	assert(start <= 7);

    if (start + idx_size > 8 && start + idx_size > 15)
    {
		idx = ((byte1 >> start) & MASK(8 - start));				// lower part from byte1
		idx = idx | (byte2  << (8 - start));
		idx = idx | ((byte3 & MASK(idx_size - 8 - (8 - start))) << (8 + (8 - start)));
		start = start + idx_size;
		start = start & 0x7;
		return 2;
    }
    else if (start + idx_size > 8 && start + idx_size <= 16)
    {
		idx = ((byte1 >> start) & MASK(8 - start));				// lower part from byte1
		if (idx_size - (8 - start) > 0) {
			// upper part from byte2
			idx = idx | ((byte2 & MASK(idx_size - (8 - start))) << (8 - start));
		}
		start = start + idx_size;
		if (start > 15) {
			start = start & 0x7;
			return 2;
		} else {
			start = start & 0x7;
			return 1;
		}
		return 1;	// bytes were swapped, new byte 2 expected
	} else if (start + idx_size <= 8) {
		idx = ((byte1 >> start) & MASK(idx_size));
		start += idx_size;
		if (start > 7) {
			start = start & 0x7;
			return 1;	// bytes were swapped, new byte 2 expected
		} else
			return 0;
	} else {
		assert(0);
		return 1;
	}
}

/**
 * @brief  Add indexes to dictionary
 *
 * @param dic dictionary to be used
 * @param add vector to be added to dic
 * @param size number of base dic size
 */
static inline
void add_to_dic(std::vector< std::vector<int> > & dic, const std::vector<int> & add, size_t size) {
	for (size_t i = 0; i < size; i++) {
		if (dic[i].size() == add.size()) {
			for (size_t j = 0; j < dic[i].size(); ++j) {
				if (dic[i][j] == add[j] && j == dic[i].size() - 1) {
					return;
				} else if (dic[i][j] != add[j])
					break;
			}
		}
	}
	dic.push_back(add);
}

/**
 * @brief  Decode LZW compression
 *
 * @param size size of output image
 * @param img image data to be decompressed
 * @param gif image which data are decompressed
 * @param out_file output file
 *
 * @return   true on success
 */
static inline
bool decode_lzw(size_t & size, GifImgData * img, Gif * gif, FILE * out_file) {
	/*
	 * set up color table to use
	 */
	std::vector<Gif::color_item_t> * color_table;
	if (img->has_local_color_table()) {
		color_table = &img->local_color_table;
	} else if (gif->has_global_color_table()) {
		color_table = &gif->global_color_table;
	} else {
		err() << "No global nor local color table!\n";
		return false;
	}

	/*
	 * init dic
	 */
	std::vector< std::vector<int> > dic;
	for (size_t i = 0; i < color_table->size(); ++i) {
		dic.push_back(std::vector<int>());
		dic[i].push_back(i);
	}

	/*
	 * add Control code and End of image to dic
	 */
	dic.push_back(std::vector<int>());
	dic[dic.size() - 1].push_back(kControlCode);
	dic.push_back(std::vector<int>());
	dic[dic.size() - 1].push_back(kEndOfImage);

	/*
	 * run lzw decoder
	 */
	std::vector<int> phrase;
	std::vector<uint8_t> indexes;
	std::vector<int> tmp;
	size_t idx = 0;
	size_t j = 1; // skip lzw min code size
	size_t start = 0;
	uint8_t byte1 = img->compressed[j++];
	uint8_t byte2 = img->compressed[j++];
	uint8_t byte3 = img->compressed[j++];
	unsigned idx_size = img->compressed[0]; // lzw min code size
	size_t original_size = dic.size();
	idx_size++; // Clear Code and End Of Image has to be indexed too
	int res;
	for(;;) {
		idx_size = log2up(dic.size());
		if (idx_size == 13)
			idx_size = 12;

		if ((res = getIdx(idx, start, idx_size, byte1, byte2, byte3))) {
			if (res == 1) {
				byte1 = byte2;
				byte2 = byte3;
				if (j < img->compressed.size())
					byte3 = img->compressed[j++];
				else
					byte3 = 0;
			} else if (res == 2) {
				byte1 = byte3;
				if (j < img->compressed.size())
					byte2 = img->compressed[j++];
				else
					byte2 = 0;
				if (j < img->compressed.size())
					byte3 = img->compressed[j++];
				else
					byte3 = 0;
			}
		}

		//dbg() << "Index:\t" << std::dec << (unsigned) idx << std::endl;
		//dbg() << "Start:\t" << std::dec << (unsigned)start << std::endl;
		//dbg() << "idx_size:\t" << std::dec << (unsigned)idx_size << std::endl;
		//dbg() << "byte1:\t0x" << std::hex << (unsigned)byte1 << std::endl;
		//dbg() << "byte2:\t0x" << std::hex << (unsigned)byte2 << std::endl;
		//dbg() << "byte3:\t0x" << std::hex << (unsigned)byte3 << std::endl;
		//dbg() << "shift count:\t" << std::dec << (unsigned)res << std::endl;
		//dbg() << "dic size:\t" << std::dec << dic.size() << std::endl;
		//dbg() << "j\t\t\t" << std::dec << j << std::endl;

		if (idx < dic.size()) {
			if (IS_CC(dic[idx])) { dic.resize(original_size); tmp.clear(); continue; }
			if (IS_EOI(dic[idx])) { dbg() << "EOI: " << j << " =?= " << img->compressed.size() << std::endl; break; }

			phrase = dic[idx];

			for (size_t i = 0; i < phrase.size(); ++i)
				indexes.push_back(phrase[i]);

			{ // add to dic
				std::vector<int> v = tmp;
				v.push_back(phrase[0]);
				add_to_dic(dic, v, original_size);
			}
		} else {
			if (idx != dic.size()) {
				warn() << "Bad index byte to dictionary. Image could be demaged!\n";
			}

			phrase = tmp;
			phrase.push_back(tmp[0]);

			for (size_t i = 0; i < phrase.size(); ++i)
				indexes.push_back(phrase[i]);

			add_to_dic(dic, phrase, original_size);
		}

		tmp = phrase;
	}

	return generate_bmp(size, gif, &indexes, color_table, out_file);
}

/**
 * @brief  Convert GIF to BMP
 *
 * @param status output status (compressed / decompressed size)
 * @param in_file input file (GIF)
 * @param out_file output file (BMP), when NULL creates image for every image in
 * GIF
 *
 * @return  0 on success
 */
int gif2bmp(struct gif2bmp_t * status, FILE * in_file, FILE * out_file)
{
	Gif gif;
	size_t size_bmp;

    if (! gif.parse(in_file))
    {
		err() << "Parse FAILED due to fatal errors!\n";
		return 1;
	}

	if (out_file != NULL) {
		if (! decode_lzw(size_bmp, gif.get_image(0), &gif, out_file))
			return false;
	} else {
		char * filename = new char[kMaxFileNameSize];
		FILE * f = NULL;
		size_t size_tmp = 0;


		for (unsigned i = 0; i < gif.num_imgs(); ++i) {
			sprintf(filename, "%04u.bmp", i+1);
			f = fopen(filename, "wb");
			if (f) {
				if (! decode_lzw(size_tmp, gif.get_image(i), &gif, f)) {
					break;
				}
				size_tmp += size_bmp;
			} else {
				err() << "Failed to create file '" << filename << "'\n";
				delete [] filename;
				return 1;
			}
		}

		delete [] filename;
	}

	if (status) {
		/*
		 * get GIF size
		 */
		fseek(in_file, 0L, SEEK_END);
		status->gif_size = ftell(in_file);
		fseek(in_file, 0L, SEEK_SET);

		/*
		 * get BMP size
		 */
		status->bmp_size = size_bmp;
	}

	return 0;
}

