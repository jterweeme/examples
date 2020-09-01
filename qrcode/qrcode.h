/* 
 * QR Code generator library (C++)
 * 
 * Copyright (c) Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/qr-code-generator-library
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#ifndef QRCODE_H
#define QRCODE_H

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class History
{
private:
    int _buf[7];
public:
    int get(int i) const;
    void clear();
    void push_front(int val);
};

/* 
 * A segment of character/binary/control data in a QR Code symbol.
 * Instances of this class are immutable.
 * The mid-level way to create a segment is to take the payload data
 * and call a static factory function such as QrSegment::makeNumeric().
 * The low-level way to create a segment is to custom-make the bit buffer
 * and call the QrSegment() constructor with appropriate values.
 * This segment class imposes no length restrictions, but QR Codes have restrictions.
 * Even in the most favorable conditions, a QR Code can only hold 7089 characters of data.
 * Any segment longer than this is meaningless for the purpose of generating QR Codes.
 */
class QrSegment final
{
public:
    class Mode final
    {
    private:
        // The mode indicator bits, which is a uint4 value (range 0 to 15).
        int modeBits;
		
		// Number of character count bits for three different version ranges.
        int numBitsCharCount[3];
        Mode(int mode, int cc0, int cc1, int cc2);
    public:
        int getModeBits() const;
        int numCharCountBits(int ver) const;
        static const Mode NUMERIC;
        static const Mode ALPHANUMERIC;
        static const Mode BYTE;
        static const Mode KANJI;
        static const Mode ECI;
	};
public:
    static QrSegment makeBytes(const std::vector<std::uint8_t> &data);
    static QrSegment makeNumeric(const char *digits);
    static QrSegment makeAlphanumeric(const char *text);
    static std::vector<QrSegment> makeSegments(const char *text);
    static QrSegment makeEci(long assignVal);
    static bool isAlphanumeric(const char *text);
    static bool isNumeric(const char *text);
	
	/*---- Instance fields ----*/
	
	/* The mode indicator of this segment. Accessed through getMode(). */
	private: Mode mode;
	
	/* The length of this segment's unencoded data. Measured in characters for
	 * numeric/alphanumeric/kanji mode, bytes for byte mode, and 0 for ECI mode.
	 * Always zero or positive. Not the same as the data's bit length.
	 * Accessed through getNumChars(). */
	private: int numChars;
private:
    /* The data bits of this segment. Accessed through getData(). */
    std::vector<bool> data;
public:
    QrSegment(Mode md, int numCh, const std::vector<bool> &dt);
	
	
	/* 
	 * Creates a new QR Code segment with the given parameters and data.
	 * The character count (numCh) must agree with the mode and the bit buffer length,
	 * but the constraint isn't checked. The given bit buffer is moved and stored.
	 */
	public: QrSegment(Mode md, int numCh, std::vector<bool> &&dt);
	
	
	/*---- Methods ----*/
	
	/* 
	 * Returns the mode field of this segment.
	 */
	public: Mode getMode() const;
	
	
	/* 
	 * Returns the character count field of this segment.
	 */
	public: int getNumChars() const;
	
	
	/* 
	 * Returns the data bits of this segment.
	 */
	public: const std::vector<bool> &getData() const;
	
	
	// (Package-private) Calculates the number of bits needed to encode the given segments at
	// the given version. Returns a non-negative number if successful. Otherwise returns -1 if a
	// segment has too many characters to fit its length field, or the total bits exceeds INT_MAX.
	public: static int getTotalBits(const std::vector<QrSegment> &segs, int version);
	
	
	/*---- Private constant ----*/
	
	/* The set of all legal characters in alphanumeric mode, where
	 * each character value maps to the index in the string. */
	private: static const char *ALPHANUMERIC_CHARSET;
	
};



/* 
 * A QR Code symbol, which is a type of two-dimension barcode.
 * Invented by Denso Wave and described in the ISO/IEC 18004 standard.
 * Instances of this class represent an immutable square grid of black and white cells.
 * The class provides static factory functions to create a QR Code from text or binary data.
 * The class covers the QR Code Model 2 specification, supporting all versions (sizes)
 * from 1 to 40, all 4 error correction levels, and 4 character encoding modes.
 * 
 * Ways to create a QR Code object:
 * - High level: Take the payload data and call QrCode::encodeText() or QrCode::encodeBinary().
 * - Mid level: Custom-make the list of segments and call QrCode::encodeSegments().
 * - Low level: Custom-make the array of data codeword bytes (including
 *   segment headers and final padding, excluding error correction codewords),
 *   supply the appropriate version number, and call the QrCode() constructor.
 * (Note that all ways require supplying the desired error correction level.)
 */
class QrCode final
{
public:
    /*
     * The error correction level in a QR Code symbol.
     */
    enum class Ecc {
		LOW = 0 ,  // The QR Code can tolerate about  7% erroneous codewords
		MEDIUM  ,  // The QR Code can tolerate about 15% erroneous codewords
		QUARTILE,  // The QR Code can tolerate about 25% erroneous codewords
		HIGH    ,  // The QR Code can tolerate about 30% erroneous codewords
	};
private:
    static int getFormatBits(Ecc ecl);
public:
    static QrCode encodeText(const char *text, Ecc ecl);
    static QrCode encodeBinary(const std::vector<std::uint8_t> &data, Ecc ecl);

    static QrCode encodeSegments(const std::vector<QrSegment> &segs, Ecc ecl,
        int minVersion=1, int maxVersion=40, int mask=-1, bool boostEcl=true);
	

	
	// Immutable scalar parameters:
	
	/* The version number of this QR Code, which is between 1 and 40 (inclusive).
	 * This determines the size of this barcode. */
private:
    int version;
	
	/* The width and height of this QR Code, measured in modules, between
	 * 21 and 177 (inclusive). This is equal to version * 4 + 17. */
    int size;
	
	/* The error correction level used in this QR Code. */
    Ecc errorCorrectionLevel;
	
	/* The index of the mask pattern used in this QR Code, which is between 0 and 7 (inclusive).
	 * Even if a QR Code is created with automatic masking requested (mask = -1),
	 * the resulting object still has a mask value between 0 and 7. */
    int mask;
	
	// Private grids of modules/pixels, with dimensions of size*size:
	
	// The modules of this QR Code (false = white, true = black).
	// Immutable after constructor finishes. Accessed through getModule().
    std::vector<std::vector<bool> > modules;
	
	// Indicates function modules that are not subjected to masking. Discarded when constructor finishes.
    std::vector<std::vector<bool> > isFunction;
public:
    QrCode(int ver, Ecc ecl, const std::vector<std::uint8_t> &dataCodewords, int msk);
    int getVersion() const;
    int getSize() const;
    Ecc getErrorCorrectionLevel() const;
    int getMask() const;
    bool getModule(int x, int y) const;
    std::string toSvgString(int border) const;
	
	/*---- Private helper methods for constructor: Drawing function modules ----*/
private:
    void drawFunctionPatterns();
    void drawFormatBits(int msk);
    void drawVersion();
    void drawFinderPattern(int x, int y);
    void drawAlignmentPattern(int x, int y);
    void setFunctionModule(int x, int y, bool isBlack);
    bool module(int x, int y) const;
    std::vector<std::uint8_t> addEccAndInterleave(const std::vector<std::uint8_t> &data) const;
    void drawCodewords(const std::vector<std::uint8_t> &data);
    void applyMask(int msk);

    std::vector<int> getAlignmentPatternPositions() const;
    static int getNumRawDataModules(int ver);
    static int getNumDataCodewords(int ver, Ecc ecl);
    static std::vector<std::uint8_t> reedSolomonComputeDivisor(int degree);

    static std::vector<std::uint8_t> reedSolomonComputeRemainder(
            const std::vector<std::uint8_t> &data, const std::vector<std::uint8_t> &divisor);
	
    static std::uint8_t reedSolomonMultiply(std::uint8_t x, std::uint8_t y);
#if 0
    long getPenaltyScore() const;
    int finderPenaltyCountPatterns(std::array<int,7> runHistory) const;
    int finderPenaltyTerminateAndCount(bool currentRunColor, int currentRunLength, std::array<int,7> &runHistory) const;
    void finderPenaltyAddHistory(int currentRunLength, std::array<int,7> &runHistory) const;
#endif
    long getPenaltyScore() const;
    int finderPenaltyCountPatterns(const History &hist) const;
    int finderPenaltyTerminateAndCount(bool currentRunColor, int currentRunLength, History &hist) const;
    void finderPenaltyAddHistory(int currentRunLength, History &hist) const;

    static bool getBit(long x, int i);
public:
    // The minimum version number supported in the QR Code Model 2 standard.
    static constexpr int MIN_VERSION =  1;
	
	// The maximum version number supported in the QR Code Model 2 standard.
    static constexpr int MAX_VERSION = 40;
	
	
	// For use in getPenaltyScore(), when evaluating which mask is best.
private:
    static const int PENALTY_N1;
    static const int PENALTY_N2;
    static const int PENALTY_N3;
    static const int PENALTY_N4;
    static const std::int8_t ECC_CODEWORDS_PER_BLOCK[4][41];
    static const std::int8_t NUM_ERROR_CORRECTION_BLOCKS[4][41];
};

/*---- Public exception class ----*/
class data_too_long : public std::length_error
{
public:
    explicit data_too_long(const std::string &msg);
};


/* 
 * An appendable sequence of bits (0s and 1s). Mainly used by QrSegment.
 */
class BitBuffer final : public std::vector<bool>
{
public:
    BitBuffer();
    void appendBits(std::uint32_t val, int len);
};
#endif
