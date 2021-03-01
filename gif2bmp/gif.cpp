/*
 ***********************************************************************
 *
 *        @version  1.0
 *        @date     03/11/2014 05:31:22 PM
 *        @author   Fridolin Pokorny <fridex.devel@gmail.com>
 *
 ***********************************************************************
 */

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <cstdlib>

#include <functional>

#include "gif.h"
#include "common.h"

const size_t Gif::kHeaderSize							= (6+7);
const size_t Gif::kColorTableSize					= 3;
const size_t Gif::kImageDescriptorSize				= 9;

const uint8_t Gif::kExtensionStartByte				= 0x21;
const uint8_t Gif::kExtensionEndByte				= 0x0;
const uint8_t Gif::kStopByte							= 0x3B;
const uint8_t Gif::kExtensionGraphicControl		= 0xF9;
const uint8_t Gif::kExtensionApplication			= 0xFF;
const uint8_t Gif::kExtensionComment				= 0xFE;
const uint8_t Gif::kExtensionPlainText				= 0x01;

const uint8_t Gif::kImageDescriptor					= 0x2c;

/**
 * @brief  Constructor
 */
Gif::Gif() {  }

/**
 * @brief  Destructor
 */
Gif::~Gif() {
	for (images_t::iterator it = m_images.begin(); it != m_images.end(); ++it) {
		delete (*it);
	}

	m_images.clear();
}

/**
 * @brief  Print debug info for header
 */
void Gif::dbg_header() {
	dbg() << ">>>> HEADER INFO <<<<\n";
	dbg() << "\tHas global table?\t" << has_global_color_table() << std::endl;
	dbg() << "\tColor resolution 8bit?\t" << is_gif8bit() << std::endl;
	dbg() << "\tSorted global table?\t" << has_sorted_global_table() << std::endl;
	dbg() << "\tSize of global table?\t" << get_global_table_size() << std::endl;
	dbg() << "\tPacked:\t\t\t0x" << std::hex << (uint16_t) m_header.packed << std::endl;
	dbg() << ">>>> HEADER INFO <<<<\n";
}

/**
 * @brief  Print debug info for global color table
 */
void Gif::dbg_global_color() {
	dbg() << "==== GLOBAL COLOR TABLE ====\n";
	for (size_t i = 0; i < get_global_table_size(); ++i)
		dbg() << "\t" << i << "\t["
				<< std::hex << std::setfill('0') << std::setw(2)  << (uint16_t) global_color_table[i].data.red
				<< ", "  << std::hex << std::setfill('0') << std::setw(2)  << (uint16_t)  global_color_table[i].data.green
				<< ", "  << std::hex << std::setfill('0') << std::setw(2)  << (uint16_t)  global_color_table[i].data.blue
				<< "]\n";
	dbg() << "==== GLOBAL COLOR TABLE ====\n";
}

/**
 * @brief  Print debug info for image description
 */
void Gif::dbg_image_descriptor(struct image_descriptor_t * desc) {
	dbg() << "//// IMAGE DESCRIPTOR \\\\\\\\\n";
	dbg() << "\tLeft:\t"			<< std::hex << (uint16_t) desc->left << std::endl;
	dbg() << "\tTop:\t"			<< std::hex << (uint16_t) desc->top << std::endl;
	dbg() << "\tWidth:\t"		<< std::hex << (uint16_t) desc->width << std::endl;
	dbg() << "\tHeight:\t"		<< std::hex << (uint16_t) desc->height << std::endl;
	dbg() << "\tPacket:\t"		<< std::hex << (uint16_t) desc->packed << std::endl;
	dbg() << std::endl;
	dbg() << "\tHas local color table?\t" << has_local_color_table(desc) << std::endl;
	dbg() << "\tHas interlace?\t\t" << has_interlace(desc) << std::endl;
	dbg() << "\tIs sorted?\t\t" << is_sorted(desc) << std::endl;
	dbg() << "\tLocal table size:\t" <<  get_local_table_size(desc) << std::endl;
	dbg() << "\\\\\\\\ IMAGE DESCRIPTOR ////\n";
}

/**
 * @brief  Print debug info for image
 */
void Gif::dbg_imgs() {
	dbg() << "****************** IMAGE DATA ******************\n";
	for (size_t i = 0; i < m_images.size(); ++i) {
		dbg() << "***\n";
		dbg() << "* " << i << std::endl;
		dbg() << "***\n";
		dbg() << "\t--- LOCAL COLOR TABLE ---\n";
		for (size_t j = 0; j < m_images[i]->local_color_table.size(); ++j)
			dbg() << "\t\t" << j << "\t["
					<< std::hex << std::setfill('0') << std::setw(2)  << (uint16_t)   m_images[i]->local_color_table[j].data.red
					<< ", "  << std::hex << std::setfill('0') << std::setw(2)  << (uint16_t)  m_images[i]->local_color_table[j].data.green
					<< ", "  << std::hex << std::setfill('0') << std::setw(2)  << (uint16_t)  m_images[i]->local_color_table[j].data.blue
					<< "]\n";
		dbg() << "\t--- LOCAL COLOR TABLE ---\n";
		dbg() << "\t--- DATA ---\n";
		for (size_t j = 0; j < m_images[i]->compressed.size(); ++j)
			dbg() << "\t" << std::dec << j << ":\t" << std::hex << std::setfill('0') << std::setw(2)  << (uint16_t) m_images[i]->compressed[j] << std::endl;

		dbg() << "\t--- DATA ---\n";

		dbg() << "****************** IMAGE DATA ******************\n";
	}
}

/**
 * @brief  Parse Graphic Control Extension
 *
 * @param f file to read extension from
 *
 * @return  true on success
 */
bool Gif::parse_graphic_control_extension(FILE * f) {
	int c;
	int size;

	size = c = fgetc(f);
	for (int i = 0; i < size && c != EOF; ++i) {
		c = fgetc(f);
	}

	c = fgetc(f);
	if (c == EOF) {
		err() << "Premature end of graphic control extension!\n";
		return false;
	} else if (c != kExtensionEndByte) {
		err() << "End of graphic control extension byte expected, got '0x"
				<< std::hex << c << "'!\n";
		return false;
	} else
		return true;
};

/**
 * @brief  Parse Application Extension
 *
 * @param f file to read extension from
 *
 * @return  true on success
 */
bool Gif::parse_application_extension(FILE * f) {
	int c = 0;
	size_t size;

	std::ostream & out = (info() << "Application extension: ");
	size = fgetc(f);
	for (size_t i = 0; i < size && (c = fgetc(f)) != EOF; ++i) {
		out << (char) c;
	}
	out << std::endl;

	/*
	 * There can be multiple sub-blocks
	 */
	do {
		size = fgetc(f);
		for (size_t i = 0; i < size && c != EOF; ++i) {
			c = fgetc(f);
		}
	} while (size != 0);

	if (c == EOF) {
		err() << "Premature end of application extension!\n";
		return false;
	} else if (c != kExtensionEndByte) {
		err() << "End of application extension byte expected, got '0x"
				<< std::hex << c << "'!\n";
		return false;
	} else
		return true;
};

/**
 * @brief  Parse Comment Extension
 *
 * @param f file to read extension from
 *
 * @return  true on success
 */
bool Gif::parse_comment_extension(FILE * f) {
	int c;
	size_t size;

	std::ostream & out = (info() << "Comment extension: ");
	do {
		size = c = fgetc(f);
		for (size_t i = 0; i < size && c != EOF; ++i)
			out << (char) (c = fgetc(f));
	} while (c != kExtensionEndByte && c != EOF);
	out << std::endl;

	if (c == EOF) {
		err() << "Premature end of comment extension!\n";
		return false;
	} else if (c != kExtensionEndByte) {
		err() << "End of comment extension byte expected, got '0x"
				<< std::hex << c << "'!\n";
		return false;
	} else
		return true;
};

/**
 * @brief  Parse Plain Text Extension
 *
 * @param f file to read extension from
 *
 * @return  true on success
 */
bool Gif::parse_plain_text_extension(FILE * f) {
	UNUSED(f);
	int c;
	int size;

	std::ostream & out = (info() << "Plain text extension: ");

	// skip block
	c = size = fgetc(f);
	for (int i = 0; c != EOF && i < size; ++i)
		c = fgetc(f);

	do {
		c = size = fgetc(f);
		for (int i = 0; c != EOF && i < size; ++i)
			out << (char) (c = fgetc(f));
	} while (c != kExtensionEndByte && c != EOF);
	out << std::endl;

	if (c == EOF) {
		err() << "Premature end of comment extension!\n";
		return false;
	} else if (c != kExtensionEndByte) {
		err() << "End of plain text extension byte expected, got '0x"
				<< std::hex << c << "'!\n";
		return false;
	} else
		return true;
};

/**
 * @brief  Parse GIF image data from file
 *
 * @param f file to parse image from
 *
 * @return true on success
 */
bool Gif::parse_image(FILE * f) {
	struct image_descriptor_t image_desc;

	class GifImgData * img = new class GifImgData;
	m_images.push_back(img);

	if (fread(&image_desc, kImageDescriptorSize, 1, f) != 1) {
		err() << "Failed to read image descriptor!\n";
		return false;
	}
	//dbg_image_descriptor(&image_desc);

	img->image_desc = image_desc;

	if (has_local_color_table(&image_desc)) {
		struct color_item_t item;
		for (size_t i = 0; i < get_local_table_size(&image_desc); ++i) {
			if (fread(&item.data, kColorTableSize, 1, f) != 1) {
				err() << "Failed to read local color table!\n";
				return false;
			}
			item.cc = item.eoi = false;
			img->local_color_table.push_back(item);
		}
	}

	int size;
	int c;
	img->compressed.push_back(fgetc(f)); // lzw mincode size
	do {
		size = c = fgetc(f);
		for (int i = 0; i < size && c != EOF; ++i) {
			c = fgetc(f);
			img->compressed.push_back(((unsigned) c) & 0xFF);
		}
		//img->compressed.push_back(c);
	} while (size != 0 && c != EOF);


	if (c == EOF) {
		err() << "Premature end of image data!\n";
		return false;
	} else if (c != 0)
		warn() << "Expected zero byte after image data, got '0x" << std::hex << c << "'!\n";

	return true;
}

/**
 * @brief Parse gif from a file
 *
 * @param f file to parse from
 *
 * @return true on success
 */
bool Gif::parse(FILE * f) {
	/*
	 * Read header
	 */
	if (fread(&m_header, kHeaderSize, 1, f) != 1) {
		err() << "Failed to read header!\n";
		return false;
	}

	/*
	 * Check correct GIF img
	 */
	if (! is_gif()) { err() << "Input file is not a GIF file!\n"; return false; }
	if (! is_gif89a()) { err() << "Unsupported GIF version!\n"; return false; }
	//if (! is_gif8bit()) { err() << "Not GIF 8bit!\n"; return false; }

	/*
	 * Read color table
	 */
	if (has_global_color_table()) {
		struct color_item_t item;
		for (size_t i = 0; i < get_global_table_size(); ++i) {
			if (fread(&item.data, kColorTableSize, 1, f) != 1) {
				err() << "Failed to read global table!\n";
				return false;
			}
			item.cc = item.eoi = false;
			global_color_table.push_back(item);
		}
		//dbg_global_color();
	}

	/*
	 * Check start byte. Note than *_extension() functions access data without
	 * entry byte!
	 */
	int c;
	while ((c = fgetc(f)) != EOF) {
		switch (c) {
			case kExtensionStartByte:
					/* FALLTRHU*/
			case kImageDescriptor:
				if (c != kImageDescriptor) c = fgetc(f);
				switch((c)) {
					case kExtensionComment:
						if (! parse_comment_extension(f))
							return false;
						break;
					case kExtensionApplication:
						if (! parse_application_extension(f))
							return false;
						break;
					case kExtensionGraphicControl:
						if (! parse_graphic_control_extension(f))
							return false;
						break;
					case kImageDescriptor:
						/* FALLTRHU*/
					case kExtensionPlainText:
						if (c == kImageDescriptor) {
							if (! parse_image(f))
								return false;
						} else if (c == kExtensionPlainText) {
							if (! parse_plain_text_extension(f))
								return false;
						}
						break;
					default:
						err() << "Unknown extension type '"
								<< std::hex << (unsigned) c << "'!\n";
						UNREACHABLE();
						return false;
				}
				break;
			case kStopByte:
				if (fgetc(f) != EOF) {
					err() << "Stop byte is not last byte!\n";
					return false;
				}
				break;
			default:
				err() << "Unknown start byte '"
					<< std::hex << (unsigned) c
					<< "', extension or EOF expected!\n";
				return false;
		}
	}

	//dbg_imgs();

	return true;
}

