/*
 ***********************************************************************
 *
 *        @version  1.0
 *        @date     03/11/2014 04:38:09 PM
 *        @author   Fridolin Pokorny <fridex.devel@gmail.com>
 *
 ***********************************************************************
 */

#ifndef GIF_H_
#define GIF_H_

#include <inttypes.h>
#include <cstdio>
#include <cstring>

#include <utility>
#include <vector>
#include <list>

/*
 * Thanks to:
 * http://www.fileformat.info/format/gif/egff.htm
 * http://www.onicos.com/staff/iz/formats/gif.html#header
 * http://www.stringology.org/DataCompression/lzw-d/index_cs.html
 */

/**
 * @brief  GIF image structure
 */
class Gif {
public:
	/**
	 * @brief  GIF header
	 */
	struct header_t
	{
	  // Header
		uint8_t signature[3];		///< Header Signature (always "GIF")
		uint8_t version[3];			///< GIF format version("87a" or "89a")
		// Logical Screen Descriptor
		uint16_t screen_width;		///< Width of Display Screen in Pixels
		uint16_t screen_height;		///< Height of Display Screen in Pixels
		uint8_t packed;				///< Screen and Color Map Information
		uint8_t background_color;	///< Background Color Index
		uint8_t aspect_ratio;		///< Pixel Aspect Ratio
	};

	/**
	 * @brief  GIF color table
	 */
	struct color_item_t
	{
		struct {
		uint8_t red;					///< Red Color Element
		uint8_t green;					///< Green Color Element
		uint8_t blue;					///< Blue Color Element
		} data;
		bool cc;
		bool eoi;
	};

	/**
	 * @brief  GIF local image descriptor
	 */
	struct image_descriptor_t
	{
		uint16_t left;					///< X position of image on the display
		uint16_t top;					///< Y position of image on the display
		uint16_t width;				///< Width of the image in pixels
		uint16_t height;				///< Height of the image in pixels
		uint8_t packed;				///< Image and Color Table Data Information
	};

	typedef std::vector<class GifImgData *> images_t;

	images_t m_images;

	size_t num_imgs() {
		return m_images.size();
	}

	class GifImgData * get_image(size_t i = 0) {
		return m_images[i];
	}

	bool parse(FILE * f);

	bool has_global_color_table() { return getbit(m_header.packed, 7); }
	bool is_gif8bit() { return ((m_header.packed >> 4) & 0x7) == 0x7; }
	bool has_sorted_global_table() { return getbit(m_header.packed, 3); }
	size_t get_global_table_size() { return (1 << ((m_header.packed & 0x7) + 1)); }

	struct header_t m_header;
	std::vector<color_item_t> global_color_table;

	Gif();
	~Gif();
private:

	static bool getbit(uint64_t x, size_t n) { return (x >> (n)) & 1; }

	bool is_gif() {
		// check start bytes
		return m_header.signature[0] == 'G'
				&& m_header.signature[1] == 'I'
				&& m_header.signature[2] == 'F';
	}

	bool is_gif89a() {
		return m_header.version[0] == '8'
				&& m_header.version[1] == '9'
				&& m_header.version[2] == 'a';
	}

	bool has_local_color_table(struct image_descriptor_t * desc) { return getbit(desc->packed, 7); }
	bool has_interlace(struct image_descriptor_t * desc) { return getbit(desc->packed, 6); }
	bool is_sorted(struct image_descriptor_t * desc) { return getbit(desc->packed, 5); }
	size_t get_local_table_size(struct image_descriptor_t * desc) { return 1 << ((desc->packed & 0x7) + 1); }

	void dbg_header();
	void dbg_global_color();
	void dbg_imgs();
	void dbg_image_descriptor(struct image_descriptor_t * desc);

	// http://www.onicos.com/staff/iz/formats/gif.html
	bool parse_graphic_control_extension(FILE * f);
	bool parse_application_extension(FILE * f);
	bool parse_comment_extension(FILE * f);
	bool parse_plain_text_extension(FILE * f);

	bool parse_image(FILE * f);

	static const size_t kHeaderSize;
	static const size_t kColorTableSize;
	static const size_t kImageDescriptorSize;

	static const uint8_t kExtensionStartByte;
	static const uint8_t kExtensionEndByte;
	static const uint8_t kStopByte;
	static const uint8_t kExtensionGraphicControl;
	static const uint8_t kExtensionApplication;
	static const uint8_t kExtensionComment;
	static const uint8_t kExtensionPlainText;

	static const uint8_t kImageDescriptor;

	friend class GifImgData;
}; // class Gif

/**
 * @brief  GIF image data structure
 */
class GifImgData {
public:
	struct Gif::image_descriptor_t image_desc;
	std::vector<struct Gif::color_item_t> local_color_table;
	std::vector<uint8_t> compressed;
	std::vector<uint8_t> decompressed;

	bool has_local_color_table() { return Gif::getbit(image_desc.packed, 7); }
	bool has_interlace() { return Gif::getbit(image_desc.packed, 6); }
	bool is_sorted() { return Gif::getbit(image_desc.packed, 5); }
	size_t get_local_table_size() { return 1 << ((image_desc.packed & 0x7) + 1); }
};

#endif // GIF_H_

