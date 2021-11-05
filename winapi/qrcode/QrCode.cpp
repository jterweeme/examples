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

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>
#include "qrcode.h"

using std::size_t;
using std::vector;

QrSegment::Mode::Mode(int mode, int cc0, int cc1, int cc2) : modeBits(mode)
{
    numBitsCharCount[0] = cc0;
    numBitsCharCount[1] = cc1;
    numBitsCharCount[2] = cc2;
}

/*
 * (Package-private) Returns the mode indicator bits, which is an unsigned 4-bit value (range 0 to 15).
 */
int QrSegment::Mode::getModeBits() const
{
    return modeBits;
}

/*
 * (Package-private) Returns the bit width of the character count field for a segment in
 * this mode in a QR Code at the given version number. The result is in the range [0, 16].
 */
int QrSegment::Mode::numCharCountBits(int ver) const
{
    return numBitsCharCount[(ver + 7) / 17];
}


const QrSegment::Mode QrSegment::Mode::NUMERIC     (0x1, 10, 12, 14);
const QrSegment::Mode QrSegment::Mode::ALPHANUMERIC(0x2,  9, 11, 13);
const QrSegment::Mode QrSegment::Mode::BYTE        (0x4,  8, 16, 16);
const QrSegment::Mode QrSegment::Mode::KANJI       (0x8,  8, 10, 12);
const QrSegment::Mode QrSegment::Mode::ECI         (0x7,  0,  0,  0);

/*
 * Returns a segment representing the given binary data encoded in
 * byte mode. All input byte vectors are acceptable. Any text string
 * can be converted to UTF-8 bytes and encoded as a byte mode segment.
 */
QrSegment QrSegment::makeBytes(const vector<BYTE> &data)
{
    if (data.size() > static_cast<unsigned int>(INT_MAX))
        throw std::length_error("Data too long");

    BitBuffer bb;
    std::vector<BYTE>::const_iterator it = data.begin();

    for (;it != data.end(); ++it)
        bb.appendBits(*it, 8);

    return QrSegment(Mode::BYTE, static_cast<int>(data.size()), bb);
}

/*
 * Returns a segment representing the given string of decimal digits encoded in numeric mode.
 */
QrSegment QrSegment::makeNumeric(const char *digits)
{
    BitBuffer bb;
    int accumData = 0;
    int accumCount = 0;
    int charCount = 0;

    for (; *digits != '\0'; digits++, charCount++)
    {
        char c = *digits;

        if (c < '0' || c > '9')
            throw std::domain_error("String contains non-numeric characters");

        accumData = accumData * 10 + (c - '0');
        accumCount++;

        if (accumCount == 3)
        {
            bb.appendBits(static_cast<DWORD>(accumData), 10);
            accumData = 0;
            accumCount = 0;
        }
    }

    // 1 or 2 digits remaining
    if (accumCount > 0)
        bb.appendBits(static_cast<DWORD>(accumData), accumCount * 3 + 1);

    return QrSegment(Mode::NUMERIC, charCount, bb);
}

/*
 * Returns a segment representing the given text string encoded in alphanumeric mode.
 * The characters allowed are: 0 to 9, A to Z (uppercase only), space,
 * dollar, percent, asterisk, plus, hyphen, period, slash, colon.
 */
QrSegment QrSegment::makeAlphanumeric(const char *text)
{
	BitBuffer bb;
	int accumData = 0;
	int accumCount = 0;
	int charCount = 0;

    for (; *text != '\0'; text++, charCount++)
    {
		const char *temp = std::strchr(ALPHANUMERIC_CHARSET, *text);

		if (temp == NULL)
			throw std::domain_error("String contains unencodable characters in alphanumeric mode");

		accumData = accumData * 45 + static_cast<int>(temp - ALPHANUMERIC_CHARSET);
		accumCount++;

        if (accumCount == 2)
        {
			bb.appendBits(static_cast<DWORD>(accumData), 11);
			accumData = 0;
			accumCount = 0;
		}
	}

	if (accumCount > 0)  // 1 character remaining
		bb.appendBits(static_cast<DWORD>(accumData), 6);

    return QrSegment(Mode::ALPHANUMERIC, charCount, bb);
}

/*
 * Returns a list of zero or more segments to represent the given text string. The result
 * may use various segment modes and switch modes to optimize the length of the bit stream.
 */
vector<QrSegment> QrSegment::makeSegments(const char *text)
{
	// Select the most efficient segment encoding automatically
	vector<QrSegment> result;

    // Leave result empty
    if (*text == '\0')
        return result;

    if (isNumeric(text))
    {
		result.push_back(makeNumeric(text));
    }
	else if (isAlphanumeric(text))
    {
		result.push_back(makeAlphanumeric(text));
    }
    else
    {
		vector<BYTE> bytes;

		for (; *text != '\0'; text++)
			bytes.push_back(static_cast<BYTE>(*text));

		result.push_back(makeBytes(bytes));
	}
	return result;
}

/*
 * Returns a segment representing an Extended Channel Interpretation
 * (ECI) designator with the given assignment value.
 */
QrSegment QrSegment::makeEci(long assignVal)
{
	BitBuffer bb;

	if (assignVal < 0)
    {
		throw std::domain_error("ECI assignment value out of range");
    }
	else if (assignVal < (1 << 7))
    {
		bb.appendBits(static_cast<DWORD>(assignVal), 8);
    }
    else if (assignVal < (1 << 14))
    {
		bb.appendBits(2, 2);
		bb.appendBits(static_cast<DWORD>(assignVal), 14);
    }
    else if (assignVal < 1000000L)
    {
		bb.appendBits(6, 3);
		bb.appendBits(static_cast<DWORD>(assignVal), 21);
    }
    else
    {
		throw std::domain_error("ECI assignment value out of range");
    }

    return QrSegment(Mode::ECI, 0, bb);
}

/*
 * Creates a new QR Code segment with the given attributes and data.
 * The character count (numCh) must agree with the mode and the bit buffer length,
 * but the constraint isn't checked. The given bit buffer is copied and stored.
 */
QrSegment::QrSegment(Mode md, int numCh, const std::vector<bool> &dt)
    : mode(md), numChars(numCh), data(dt)
{
	if (numCh < 0)
		throw std::domain_error("Invalid value");
}

/*
 * Creates a new QR Code segment with the given parameters and data.
 * The character count (numCh) must agree with the mode and the bit buffer length,
 * but the constraint isn't checked. The given bit buffer is moved and stored.
 */
#ifdef CPP11
QrSegment::QrSegment(Mode md, int numCh, std::vector<bool> &&dt)
  :
    mode(md),
    numChars(numCh),
    data(std::move(dt))
{
	if (numCh < 0)
		throw std::domain_error("Invalid value");
}
#endif

// (Package-private) Calculates the number of bits needed to encode the given segments at
// the given version. Returns a non-negative number if successful. Otherwise returns -1 if a
// segment has too many characters to fit its length field, or the total bits exceeds INT_MAX.
int QrSegment::getTotalBits(const vector<QrSegment> &segs, int version)
{
	int result = 0;

    std::vector<QrSegment>::const_iterator it = segs.begin();

    while (it != segs.end())
    {
        int ccbits = it->mode.numCharCountBits(version);

        // The segment's length doesn't fit the field's bit width
        if (it->numChars >= (1L << ccbits))
            return -1;

        // The sum will overflow an int type
        if (4 + ccbits > INT_MAX - result)
            return -1;

        result += 4 + ccbits;

        // The sum will overflow an int type
        if (it->data.size() > static_cast<unsigned int>(INT_MAX - result))
            return -1;

        result += static_cast<int>(it->data.size());
        ++it;
    }

	return result;
}

/*
 * Tests whether the given string can be encoded as a segment in alphanumeric mode.
 * A string is encodable iff each character is in the following set: 0 to 9, A to Z
 * (uppercase only), space, dollar, percent, asterisk, plus, hyphen, period, slash, colon.
 */
bool QrSegment::isAlphanumeric(const char *text)
{
    for (; *text != '\0'; text++)
    {
		if (std::strchr(ALPHANUMERIC_CHARSET, *text) == NULL)
			return false;
	}
	return true;
}

/*
 * Tests whether the given string can be encoded as a segment in numeric mode.
 * A string is encodable iff each character is in the range 0 to 9.
 */
bool QrSegment::isNumeric(const char *text)
{
    for (; *text != '\0'; text++)
    {
		char c = *text;
		if (c < '0' || c > '9')
			return false;
	}
	return true;
}

/*
 * Returns the mode field of this segment.
 */
QrSegment::Mode QrSegment::getMode() const
{
	return mode;
}

/*
 * Returns the character count field of this segment.
 */
int QrSegment::getNumChars() const
{
	return numChars;
}

/*
 * Returns the data bits of this segment.
 */
const std::vector<bool> &QrSegment::getData() const
{
	return data;
}


const char *QrSegment::ALPHANUMERIC_CHARSET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";


// Returns a value in the range 0 to 3 (unsigned 2-bit integer).
int QrCode::getFormatBits(Ecc ecl)
{
    switch (ecl)
    {
    case Ecc::LOW:
        return 1;
    case Ecc::MEDIUM:
        return 0;
    case Ecc::QUARTILE:
        return 3;
    case Ecc::HIGH:
        return 2;
    default:
        break;
	}

    throw std::logic_error("Assertion error");
    return -1;
}

/*
 * Returns a QR Code representing the given Unicode text string at the given error correction level.
 * As a conservative upper bound, this function is guaranteed to succeed for strings that have 2953 or fewer
 * UTF-8 code units (not Unicode code points) if the low error correction level is used. The smallest possible
 * QR Code version is automatically chosen for the output. The ECC level of the result may be higher than
 * the ecl argument if it can be done without increasing the version.
 */
QrCode QrCode::encodeText(const char *text, Ecc ecl)
{
	vector<QrSegment> segs = QrSegment::makeSegments(text);
	return encodeSegments(segs, ecl);
}

/*
 * Returns a QR Code representing the given binary data at the given error correction level.
 * This function always encodes using the binary segment mode, not any text mode. The maximum number of
 * bytes allowed is 2953. The smallest possible QR Code version is automatically chosen for the output.
 * The ECC level of the result may be higher than the ecl argument if it can be done without increasing the version.
 */
QrCode QrCode::encodeBinary(const vector<BYTE> &data, Ecc ecl)
{
#if 0
    vector<QrSegment> segs{QrSegment::makeBytes(data)};
#else
    vector<QrSegment> segs;
    segs.push_back(QrSegment::makeBytes(data));
#endif
    return encodeSegments(segs, ecl);
}

/*
 * Returns a QR Code representing the given segments with the given encoding parameters.
 * The smallest possible QR Code version within the given range is automatically
 * chosen for the output. Iff boostEcl is true, then the ECC level of the result
 * may be higher than the ecl argument if it can be done without increasing the
 * version. The mask number is either between 0 to 7 (inclusive) to force that
 * mask, or -1 to automatically choose an appropriate mask (which may be slow).
 * This function allows the user to create a custom sequence of segments that switches
 * between modes (such as alphanumeric and byte) to encode text in less space.
 * This is a mid-level API; the high-level API is encodeText() and encodeBinary().
 */
QrCode QrCode::encodeSegments(const vector<QrSegment> &segs, Ecc ecl,
        int minVersion, int maxVersion, int mask, bool boostEcl)
{
	if (!(MIN_VERSION <= minVersion && minVersion <= maxVersion && maxVersion <= MAX_VERSION) || mask < -1 || mask > 7)
		throw std::invalid_argument("Invalid value");
	
	// Find the minimal version number to use
	int version, dataUsedBits;
    for (version = minVersion; ; version++)
    {
        // Number of data bits available
        int dataCapacityBits = getNumDataCodewords(version, ecl) * 8;

		dataUsedBits = QrSegment::getTotalBits(segs, version);

        // This version number is found to be suitable
		if (dataUsedBits != -1 && dataUsedBits <= dataCapacityBits)
            break;

        // All versions in the range could not fit the given data
        if (version >= maxVersion)
        {
			std::ostringstream sb;
			if (dataUsedBits == -1)
            {
				sb << "Segment too long";
            }
            else
            {
				sb << "Data length = " << dataUsedBits << " bits, ";
				sb << "Max capacity = " << dataCapacityBits << " bits";
			}
			throw data_too_long(sb.str());
		}
	}

	if (dataUsedBits == -1)
		throw std::logic_error("Assertion error");
	
	// Increase the error correction level while the data still fits in the current version number
#if 0
    for (Ecc newEcl : vector<Ecc>{Ecc::MEDIUM, Ecc::QUARTILE, Ecc::HIGH})
    {
        // From low to high
		if (boostEcl && dataUsedBits <= getNumDataCodewords(version, newEcl) * 8)
			ecl = newEcl;
    }
#else
    if (boostEcl && dataUsedBits <= getNumDataCodewords(version, Ecc::MEDIUM) * 8)
        ecl = Ecc::MEDIUM;

    if (boostEcl && dataUsedBits <= getNumDataCodewords(version, Ecc::QUARTILE) * 8)
        ecl = Ecc::QUARTILE;

    if (boostEcl && dataUsedBits <= getNumDataCodewords(version, Ecc::HIGH) * 8)
        ecl = Ecc::HIGH;
#endif
	
    // Concatenate all segments to create the data bit string
    BitBuffer bb;

    std::vector<QrSegment>::const_iterator it = segs.begin();
    while (it != segs.end())
    {
        bb.appendBits(static_cast<DWORD>(it->getMode().getModeBits()), 4);
        bb.appendBits(static_cast<DWORD>(it->getNumChars()), it->getMode().numCharCountBits(version));
        bb.insert(bb.end(), it->getData().begin(), it->getData().end());
        ++it;
    }

    if (bb.size() != static_cast<unsigned int>(dataUsedBits))
        throw std::logic_error("Assertion error");
	
    // Add terminator and pad up to a byte if applicable
    size_t dataCapacityBits = static_cast<size_t>(getNumDataCodewords(version, ecl)) * 8;

    if (bb.size() > dataCapacityBits)
        throw std::logic_error("Assertion error");

    bb.appendBits(0, mynMin(4, static_cast<int>(dataCapacityBits - bb.size())));
    bb.appendBits(0, (8 - static_cast<int>(bb.size() % 8)) % 8);

    if (bb.size() % 8 != 0)
        throw std::logic_error("Assertion error");
	
    // Pad with alternating bytes until data capacity is reached
    for (BYTE padByte = 0xEC; bb.size() < dataCapacityBits; padByte ^= 0xEC ^ 0x11)
        bb.appendBits(padByte, 8);
	
    // Pack bits into bytes in big endian
    vector<BYTE> dataCodewords(bb.size() / 8);
    for (size_t i = 0; i < bb.size(); i++)
        dataCodewords[i >> 3] |= (bb.at(i) ? 1 : 0) << (7 - (i & 7));
	
    // Create the QR Code object
    return QrCode(version, ecl, dataCodewords, mask);
}

/*
 * Creates a new QR Code with the given version number,
 * error correction level, data codeword bytes, and mask number.
 * This is a low-level API that most users should not use directly.
 * A mid-level API is the encodeSegments() function.
 */
QrCode::QrCode(int ver, Ecc ecl, const vector<BYTE> &dataCodewords, int msk)
    : version(ver), errorCorrectionLevel(ecl)
{
	if (ver < MIN_VERSION || ver > MAX_VERSION)
		throw std::domain_error("Version value out of range");

	if (msk < -1 || msk > 7)
		throw std::domain_error("Mask value out of range");

	size = ver * 4 + 17;
	size_t sz = static_cast<size_t>(size);

    // Initially all white
    modules = vector<vector<bool> >(sz, vector<bool>(sz));

	isFunction = vector<vector<bool> >(sz, vector<bool>(sz));
	
	// Compute ECC, draw modules
	drawFunctionPatterns();
        const vector<BYTE> allCodewords = addEccAndInterleave(dataCodewords);
	drawCodewords(allCodewords);
	
	// Do masking
    // Automatically choose best mask
    if (msk == -1)
    {
        long minPenalty = LONG_MAX;
        for (int i = 0; i < 8; i++)
        {
			applyMask(i);
			drawFormatBits(i);
			long penalty = getPenaltyScore();

            if (penalty < minPenalty)
            {
				msk = i;
				minPenalty = penalty;
			}

            // Undoes the mask due to XOR
            applyMask(i);
		}
	}
	if (msk < 0 || msk > 7)
		throw std::logic_error("Assertion error");

	this->mask = msk;
	applyMask(msk);  // Apply the final choice of mask
	drawFormatBits(msk);  // Overwrite old format bits
	
	isFunction.clear();
#ifdef CPP11
	isFunction.shrink_to_fit();
#endif
}

/*
 * Returns this QR Code's version, in the range [1, 40].
 */
int QrCode::getVersion() const
{
    return version;
}

/*
 * Returns this QR Code's size, in the range [21, 177].
 */
int QrCode::getSize() const
{
    return size;
}

/*
 * Returns this QR Code's error correction level.
 */
QrCode::Ecc QrCode::getErrorCorrectionLevel() const
{
    return errorCorrectionLevel;
}

/*
 * Returns this QR Code's mask, in the range [0, 7].
 */
int QrCode::getMask() const
{
	return mask;
}

/*
 * Returns the color of the module (pixel) at the given coordinates, which is false
 * for white or true for black. The top left corner has the coordinates (x=0, y=0).
 * If the given coordinates are out of bounds, then false (white) is returned.
 */
bool QrCode::getModule(int x, int y) const
{
	return 0 <= x && x < size && 0 <= y && y < size && module(x, y);
}

/*
 * Returns a string of SVG code for an image depicting this QR Code, with the given number
 * of border modules. The string always uses Unix newlines (\n), regardless of the platform.
 */
std::string QrCode::toSvgString(int border) const
{
	if (border < 0)
		throw std::domain_error("Border must be non-negative");

	if (border > INT_MAX / 2 || border * 2 > INT_MAX - size)
		throw std::overflow_error("Border too large");
	
	std::ostringstream sb;
	sb << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	sb << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
	sb << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
	sb << (size + border * 2) << " " << (size + border * 2) << "\" stroke=\"none\">\n";
	sb << "\t<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>\n";
	sb << "\t<path d=\"";

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            if (getModule(x, y))
            {
				if (x != 0 || y != 0)
					sb << " ";

				sb << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
			}
		}
	}
	sb << "\" fill=\"#000000\"/>\n";
	sb << "</svg>\n";
	return sb.str();
}

// Reads this object's version field, and draws and marks all function modules.
void QrCode::drawFunctionPatterns()
{
	// Draw horizontal and vertical timing patterns
    for (int i = 0; i < size; i++)
    {
		setFunctionModule(6, i, i % 2 == 0);
		setFunctionModule(i, 6, i % 2 == 0);
	}
	
	// Draw 3 finder patterns (all corners except bottom right; overwrites some timing modules)
	drawFinderPattern(3, 3);
	drawFinderPattern(size - 4, 3);
	drawFinderPattern(3, size - 4);
	
	// Draw numerous alignment patterns
	const vector<int> alignPatPos = getAlignmentPatternPositions();
	size_t numAlign = alignPatPos.size();

    for (size_t i = 0; i < numAlign; i++)
    {
        for (size_t j = 0; j < numAlign; j++)
        {
			// Don't draw on the three finder corners
			if (!((i == 0 && j == 0) || (i == 0 && j == numAlign - 1) || (i == numAlign - 1 && j == 0)))
				drawAlignmentPattern(alignPatPos.at(i), alignPatPos.at(j));
		}
	}
	
	// Draw configuration data
	drawFormatBits(0);  // Dummy mask value; overwritten later in the constructor
	drawVersion();
}

// Draws two copies of the format bits (with its own error correction code)
// based on the given mask and this object's error correction level field.
void QrCode::drawFormatBits(int msk)
{
    // Calculate error correction code and pack bits
    // errCorrLvl is uint2, msk is uint3
    int data = getFormatBits(errorCorrectionLevel) << 3 | msk;
    int rem = data;

    for (int i = 0; i < 10; i++)
        rem = (rem << 1) ^ ((rem >> 9) * 0x537);

    // uint15
    int bits = (data << 10 | rem) ^ 0x5412;

    if (bits >> 15 != 0)
        throw std::logic_error("Assertion error");
	
    // Draw first copy
    for (int i = 0; i <= 5; i++)
        setFunctionModule(8, i, getBit(bits, i));

    setFunctionModule(8, 7, getBit(bits, 6));
    setFunctionModule(8, 8, getBit(bits, 7));
    setFunctionModule(7, 8, getBit(bits, 8));

    for (int i = 9; i < 15; i++)
        setFunctionModule(14 - i, 8, getBit(bits, i));
	
    // Draw second copy
    for (int i = 0; i < 8; i++)
        setFunctionModule(size - 1 - i, 8, getBit(bits, i));

    for (int i = 8; i < 15; i++)
        setFunctionModule(8, size - 15 + i, getBit(bits, i));

    // Always black
    setFunctionModule(8, size - 8, true);
}

// Draws two copies of the version bits (with its own error correction code),
// based on this object's version field, iff 7 <= version <= 40.
void QrCode::drawVersion()
{
    if (version < 7)
        return;
	
    // Calculate error correction code and pack bits
    int rem = version;  // version is uint6, in the range [7, 40]

    for (int i = 0; i < 12; i++)
        rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);

    long bits = static_cast<long>(version) << 12 | rem;  // uint18

    if (bits >> 18 != 0)
        throw std::logic_error("Assertion error");
	
    // Draw two copies
    for (int i = 0; i < 18; i++)
    {
        bool bit = getBit(bits, i);
        int a = size - 11 + i % 3;
        int b = i / 3;
        setFunctionModule(a, b, bit);
        setFunctionModule(b, a, bit);
    }
}

// Draws a 9*9 finder pattern including the border separator,
// with the center module at (x, y). Modules can be out of bounds.
void QrCode::drawFinderPattern(int x, int y)
{
    for (int dy = -4; dy <= 4; dy++)
    {
        for (int dx = -4; dx <= 4; dx++)
        {
            // Chebyshev/infinity norm
            int dist = mynMax(std::abs(dx), std::abs(dy));
			int xx = x + dx, yy = y + dy;

			if (0 <= xx && xx < size && 0 <= yy && yy < size)
				setFunctionModule(xx, yy, dist != 2 && dist != 4);
		}
	}
}

// Draws a 5*5 alignment pattern, with the center module
// at (x, y). All modules must be in bounds.
void QrCode::drawAlignmentPattern(int x, int y)
{
    for (int dy = -2; dy <= 2; dy++)
    {
        for (int dx = -2; dx <= 2; dx++)
            setFunctionModule(x + dx, y + dy, mynMax(std::abs(dx), std::abs(dy)) != 1);
    }
}

// Sets the color of a module and marks it as a function module.
// Only used by the constructor. Coordinates must be in bounds.
void QrCode::setFunctionModule(int x, int y, bool isBlack)
{
    size_t ux = static_cast<size_t>(x);
    size_t uy = static_cast<size_t>(y);
    modules.at(uy).at(ux) = isBlack;
    isFunction.at(uy).at(ux) = true;
}

// Returns the color of the module at the given coordinates, which must be in range.
bool QrCode::module(int x, int y) const
{
    return modules.at(static_cast<size_t>(y)).at(static_cast<size_t>(x));
}

// Returns a new byte string representing the given data with the appropriate error correction
// codewords appended to it, based on this object's version and error correction level.
vector<BYTE> QrCode::addEccAndInterleave(const vector<BYTE> &data) const
{
    if (data.size() != static_cast<unsigned int>(getNumDataCodewords(version, errorCorrectionLevel)))
        throw std::invalid_argument("Invalid argument");
	
    // Calculate parameter numbers
    int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[static_cast<int>(errorCorrectionLevel)][version];
    int blockEccLen = ECC_CODEWORDS_PER_BLOCK  [static_cast<int>(errorCorrectionLevel)][version];
    int rawCodewords = getNumRawDataModules(version) / 8;
    int numShortBlocks = numBlocks - rawCodewords % numBlocks;
    int shortBlockLen = rawCodewords / numBlocks;
	
    // Split data into blocks and append ECC to each block
    vector<vector<BYTE> > blocks;
    const vector<BYTE> rsDiv = reedSolomonComputeDivisor(blockEccLen);

    for (int i = 0, k = 0; i < numBlocks; i++)
    {
        vector<BYTE> dat(data.begin() + k, data.begin() + (k + shortBlockLen - blockEccLen + (i < numShortBlocks ? 0 : 1)));
        k += static_cast<int>(dat.size());
        const vector<BYTE> ecc = reedSolomonComputeRemainder(dat, rsDiv);

        if (i < numShortBlocks)
            dat.push_back(0);

        dat.insert(dat.end(), ecc.begin(), ecc.end());
        blocks.push_back(dat);
    }
	
    // Interleave (not concatenate) the bytes from every block into a single sequence
    vector<BYTE> result;
    for (size_t i = 0; i < blocks.at(0).size(); i++)
    {
        for (size_t j = 0; j < blocks.size(); j++)
        {
            // Skip the padding byte in short blocks
            if (i != static_cast<unsigned int>(shortBlockLen - blockEccLen) || j >= static_cast<unsigned int>(numShortBlocks))
                result.push_back(blocks.at(j).at(i));
        }
    }

    if (result.size() != static_cast<unsigned int>(rawCodewords))
        throw std::logic_error("Assertion error");

    return result;
}

// Draws the given sequence of 8-bit codewords (data and error correction) onto the entire
// data area of this QR Code. Function modules need to be marked off before this is called.
void QrCode::drawCodewords(const vector<BYTE> &data)
{
    if (data.size() != static_cast<unsigned int>(getNumRawDataModules(version) / 8))
        throw std::invalid_argument("Invalid argument");
	
    // Bit index into the data
    size_t i = 0;

    // Do the funny zigzag scan
    // Index of right column in each column pair
    for (int right = size - 1; right >= 1; right -= 2)
    {
        if (right == 6)
            right = 5;

        // Vertical counter
        for (int vert = 0; vert < size; vert++)
        {
            for (int j = 0; j < 2; j++)
            {
                // Actual x coordinate
                size_t x = static_cast<size_t>(right - j);

                bool upward = ((right + 1) & 2) == 0;

                // Actual y coordinate
                size_t y = static_cast<size_t>(upward ? size - 1 - vert : vert);

                if (!isFunction.at(y).at(x) && i < data.size() * 8)
                {
					modules.at(y).at(x) = getBit(data.at(i >> 3), 7 - static_cast<int>(i & 7));
					i++;
				}
				// If this QR Code has any remainder bits (0 to 7), they were assigned as
				// 0/false/white by the constructor and are left unchanged by this method
			}
		}
	}
	if (i != data.size() * 8)
		throw std::logic_error("Assertion error");
}

// XORs the codeword modules in this QR Code with the given mask pattern.
// The function modules must be marked and the codeword bits must be drawn
// before masking. Due to the arithmetic of XOR, calling applyMask() with
// the same mask value a second time will undo the mask. A final well-formed
// QR Code needs exactly one (not zero, two, etc.) mask applied.
void QrCode::applyMask(int msk)
{
	if (msk < 0 || msk > 7)
		throw std::domain_error("Mask value out of range");

	size_t sz = static_cast<size_t>(size);

    for (size_t y = 0; y < sz; y++)
    {
        for (size_t x = 0; x < sz; x++)
        {
			bool invert;

            switch (msk)
            {
            case 0:
                invert = (x + y) % 2 == 0;
                break;
            case 1:
                invert = y % 2 == 0;
                break;
            case 2:
                invert = x % 3 == 0;
                break;
            case 3:
                invert = (x + y) % 3 == 0;
                break;
            case 4:
                invert = (x / 3 + y / 2) % 2 == 0;
                break;
            case 5:
                invert = x * y % 2 + x * y % 3 == 0;
                break;
            case 6:
                invert = (x * y % 2 + x * y % 3) % 2 == 0;
                break;
            case 7:
                invert = ((x + y) % 2 + x * y % 3) % 2 == 0;
                break;
            default:
                throw std::logic_error("Assertion error");
			}
			modules.at(y).at(x) = modules.at(y).at(x) ^ (invert & !isFunction.at(y).at(x));
		}
	}
}

// Calculates and returns the penalty score based on state of this QR Code's current modules.
// This is used by the automatic mask choice algorithm to find the mask pattern that yields the lowest score.
long QrCode::getPenaltyScore() const
{
    long result = 0;

    // Adjacent modules in row having same color, and finder-like patterns
    for (int y = 0; y < size; y++)
    {
        bool runColor = false;
        int runX = 0;
        History hist;
        hist.clear();
        //std::array<int,7> runHistory = {};

        for (int x = 0; x < size; x++)
        {
            if (module(x, y) == runColor)
            {
                runX++;
                if (runX == 5)
                    result += PENALTY_N1;
                else if (runX > 5)
                    result++;
            }
            else
            {
                finderPenaltyAddHistory(runX, hist);

                if (!runColor)
                    result += finderPenaltyCountPatterns(hist) * PENALTY_N3;

                runColor = module(x, y);
                runX = 1;
            }
        }
        result += finderPenaltyTerminateAndCount(runColor, runX, hist) * PENALTY_N3;
    }

    // Adjacent modules in column having same color, and finder-like patterns
    for (int x = 0; x < size; x++)
    {
        bool runColor = false;
        int runY = 0;

        History hist;
        hist.clear();

        //std::array<int,7> runHistory = {};

        for (int y = 0; y < size; y++)
        {
            if (module(x, y) == runColor)
            {
                runY++;
                if (runY == 5)
                    result += PENALTY_N1;
                else if (runY > 5)
                    result++;
            }
            else
            {
                finderPenaltyAddHistory(runY, hist);

                if (!runColor)
                    result += finderPenaltyCountPatterns(hist) * PENALTY_N3;

                runColor = module(x, y);
                runY = 1;
            }
        }
        result += finderPenaltyTerminateAndCount(runColor, runY, hist) * PENALTY_N3;
    }

    // 2*2 blocks of modules having same color
    for (int y = 0; y < size - 1; y++)
    {
        for (int x = 0; x < size - 1; x++)
        {
            bool color = module(x, y);

            if (color == module(x + 1, y) && color == module(x, y + 1) && color == module(x + 1, y + 1))
                result += PENALTY_N2;
        }
    }

    // Balance of black and white modules
    int black = 0;
#if 0
    for (const vector<bool> &row : modules)
    {
        for (bool color : row)
        {
            if (color)
                black++;
        }
    }
#else
    std::vector<std::vector<bool> >::const_iterator row = modules.begin();

    while (row != modules.end())
    {
        std::vector<bool>::const_iterator color = row->begin();
        for (;color != row->end(); ++color)
            if (*color)
                black++;

        ++row;
    }
#endif

    // Note that size is odd, so black/total != 1/2
    int total = size * size;

    // Compute the smallest integer k >= 0 such that (45-5k)% <= black/total <= (55+5k)%
    int k = static_cast<int>((std::abs(black * 20L - total * 10L) + total - 1) / total) - 1;

    result += k * PENALTY_N4;
    return result;
}




// Returns an ascending list of positions of alignment patterns for this version number.
// Each position is in the range [0,177), and are used on both the x and y axes.
// This could be implemented as lookup table of 40 variable-length lists of unsigned bytes.
vector<int> QrCode::getAlignmentPatternPositions() const
{
    if (version == 1)
        return vector<int>();

    int numAlign = version / 7 + 2;
    int step = (version == 32) ? 26 : (version*4 + numAlign*2 + 1) / (numAlign*2 - 2) * 2;

    vector<int> result;
    for (int i = 0, pos = size - 7; i < numAlign - 1; i++, pos -= step)
        result.insert(result.begin(), pos);

    result.insert(result.begin(), 6);
    return result;

}

// Returns the number of data bits that can be stored in a QR Code of the given version number, after
// all function modules are excluded. This includes remainder bits, so it might not be a multiple of 8.
// The result is in the range [208, 29648]. This could be implemented as a 40-entry lookup table.
int QrCode::getNumRawDataModules(int ver)
{
	if (ver < MIN_VERSION || ver > MAX_VERSION)
		throw std::domain_error("Version number out of range");

	int result = (16 * ver + 128) * ver + 64;

    if (ver >= 2)
    {
		int numAlign = ver / 7 + 2;
		result -= (25 * numAlign - 10) * numAlign - 55;
		if (ver >= 7)
			result -= 36;
	}

	if (!(208 <= result && result <= 29648))
		throw std::logic_error("Assertion error");

	return result;
}

// Returns the number of 8-bit data (i.e. not error correction) codewords contained in any
// QR Code of the given version number and error correction level, with remainder bits discarded.
// This stateless pure function could be implemented as a (40*4)-cell lookup table.
int QrCode::getNumDataCodewords(int ver, Ecc ecl)
{
	return getNumRawDataModules(ver) / 8
		- ECC_CODEWORDS_PER_BLOCK    [static_cast<int>(ecl)][ver]
		* NUM_ERROR_CORRECTION_BLOCKS[static_cast<int>(ecl)][ver];
}

// Returns a Reed-Solomon ECC generator polynomial for the given degree. This could be
// implemented as a lookup table over all possible parameter values, instead of as an algorithm.
vector<BYTE> QrCode::reedSolomonComputeDivisor(int degree)
{
    if (degree < 1 || degree > 255)
        throw std::domain_error("Degree out of range");

    // Polynomial coefficients are stored from highest to lowest power, excluding the leading term which is always 1.
    // For example the polynomial x^3 + 255x^2 + 8x + 93 is stored as the uint8 array {255, 8, 93}.
    vector<BYTE> result(static_cast<size_t>(degree));
    result.at(result.size() - 1) = 1;  // Start off with the monomial x^0
	
    // Compute the product polynomial (x - r^0) * (x - r^1) * (x - r^2) * ... * (x - r^{degree-1}),
    // and drop the highest monomial term which is always 1x^degree.
    // Note that r = 0x02, which is a generator element of this field GF(2^8/0x11D).
    BYTE root = 1;
    for (int i = 0; i < degree; i++)
    {
        // Multiply the current product by (x - r^i)
        for (size_t j = 0; j < result.size(); j++)
        {
            result.at(j) = reedSolomonMultiply(result.at(j), root);

            if (j + 1 < result.size())
                result.at(j) ^= result.at(j + 1);
        }
        root = reedSolomonMultiply(root, 0x02);
    }
    return result;
}

// Returns the Reed-Solomon error correction codeword for the given data and divisor polynomials.
vector<BYTE> QrCode::reedSolomonComputeRemainder(
    const vector<BYTE> &data, const vector<BYTE> &divisor)
{
    vector<BYTE> result(divisor.size());

    // Polynomial division
    std::vector<BYTE>::const_iterator it = data.begin();

    while (it != data.end())
    {
        BYTE factor = *it ^ result.at(0);
        result.erase(result.begin());
        result.push_back(0);

        for (size_t i = 0; i < result.size(); i++)
            result.at(i) ^= reedSolomonMultiply(divisor.at(i), factor);

        ++it;
    }

    return result;
}

// Returns the product of the two given field elements modulo GF(2^8/0x11D).
// All inputs are valid. This could be implemented as a 256*256 lookup table.
BYTE QrCode::reedSolomonMultiply(BYTE x, BYTE y)
{
    // Russian peasant multiplication
    int z = 0;
    for (int i = 7; i >= 0; i--)
    {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }

    if (z >> 8 != 0)
        throw std::logic_error("Assertion error");

    return static_cast<BYTE>(z);
}

int History::get(int i) const
{
    return _buf[i];
}

void History::clear()
{
    for (int i = 0; i < 7; ++i)
        _buf[i] = 0;
}

void History::push_front(int val)
{
    for (int i = 6; i > 0; --i)
        _buf[i] = _buf[i - 1];

    _buf[0] = val;
}

// Can only be called immediately after a white run is added, and
// returns either 0, 1, or 2. A helper function for getPenaltyScore().
int QrCode::finderPenaltyCountPatterns(const History &hist) const
{
    const int n = hist.get(1);

    if (n > size * 3)
        throw std::logic_error("Assertion error");

    const bool core = n > 0 && hist.get(2) == n && hist.get(3) == n * 3 && hist.get(4) == n && hist.get(5) == n;
    const int a = core && hist.get(0) >= n * 4 && hist.get(6) >= n ? 1 : 0;
    const int b = core && hist.get(6) >= n * 4 && hist.get(0) >= n ? 1 : 0;
    return a + b;
}

// Must be called at the end of a line (row or column) of modules.
//A helper function for getPenaltyScore().
int QrCode::finderPenaltyTerminateAndCount(
    bool currentRunColor, int currentRunLength, History &hist) const
{
    if (currentRunColor)
    {
        finderPenaltyAddHistory(currentRunLength, hist);
        currentRunLength = 0;
    }

    currentRunLength += size;
    finderPenaltyAddHistory(currentRunLength, hist);
    return finderPenaltyCountPatterns(hist);
}

// Pushes the given value to the front and drops the last value.
//A helper function for getPenaltyScore().
void QrCode::finderPenaltyAddHistory(int currentRunLength, History &hist) const
{
    if (hist.get(0) == 0)
        currentRunLength += size;

    hist.push_front(currentRunLength);
}

// Returns true iff the i'th bit of x is set to 1.
bool QrCode::getBit(long x, int i)
{
	return ((x >> i) & 1) != 0;
}

const char QrCode::ECC_CODEWORDS_PER_BLOCK[4][41] = {
	// Version: (note that index 0 is for padding, and is set to an illegal value)
	//0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40    Error correction level
	{-1,  7, 10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28, 28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},  // Low
	{-1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28},  // Medium
	{-1, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30, 28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},  // Quartile
	{-1, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28, 30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},  // High
};

const char QrCode::NUM_ERROR_CORRECTION_BLOCKS[4][41] = {
	// Version: (note that index 0 is for padding, and is set to an illegal value)
	//0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40    Error correction level
	{-1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4,  4,  4,  4,  4,  6,  6,  6,  6,  7,  8,  8,  9,  9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},  // Low
	{-1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5,  5,  8,  9,  9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},  // Medium
	{-1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8,  8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},  // Quartile
	{-1, 1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81},  // High
};

/*
 * Thrown when the supplied data does not fit any QR Code version. Ways to handle this exception include:
 * - Decrease the error correction level if it was greater than Ecc::LOW.
 * - If the encodeSegments() function was called with a maxVersion argument, then increase
 *   it if it was less than QrCode::MAX_VERSION. (This advice does not apply to the other
 *   factory functions because they search all versions up to QrCode::MAX_VERSION.)
 * - Split the text data into better or optimal segments in order to reduce the number of bits required.
 * - Change the text or binary data to be shorter.
 * - Change the text to fit the character set of a particular segment mode (e.g. alphanumeric).
 * - Propagate the error upward to the caller/user.
 */
data_too_long::data_too_long(const std::string &msg) : std::length_error(msg)
{

}

// Creates an empty bit buffer (length 0).
BitBuffer::BitBuffer() : std::vector<bool>()
{

}

// Appends the given number of low-order bits of the given value
// to this buffer. Requires 0 <= len <= 31 and val < 2^len.
void BitBuffer::appendBits(DWORD val, int len)
{
    if (len < 0 || len > 31 || val >> len != 0)
        throw std::domain_error("Value out of range");

    // Append bit by bit
    for (int i = len - 1; i >= 0; i--)
        this->push_back(((val >> i) & 1) != 0);
}

