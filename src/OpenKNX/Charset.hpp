// Deliberately no "#pragma once": the content depends on the state of
// OPENKNX_CHARSET *at the time of each individual #include*. A classic
// include guard (or #pragma once) would keep a file that was once read
// while empty -- e.g. before the #define -- empty for the rest of the
// translation unit, even if a later #include (after the #define) should
// actually activate it. The guard below is therefore only set once the
// content has actually been emitted, not just because the file was read.
#if defined(OPENKNX_CHARSET) && !defined(OPENKNX_CHARSET_HPP_DEFINED)
    #define OPENKNX_CHARSET_HPP_DEFINED
    #include <cstdint>
    #include <string>

namespace OpenKNX
{
    namespace Charset
    {
        // Firmware encoding (ISO-8859-15) -> UTF-8. Every byte is a valid
        // Unicode code point -> can never fail, hence no bool return value
        // (unlike decodeUtf8, which can be lossy).
        inline void encodeUtf8(std::string& out, const char* s, size_t len)
        {
            for (size_t i = 0; i < len; i++)
            {
                uint8_t c = (uint8_t)s[i];
                if (c < 0x80)
                {
                    out += (char)c;
                    continue;
                }
                uint16_t cp = c;
                switch (c)
                {
                    case 0xA4: cp = 0x20AC; break; // € -- only Latin-15 special case > U+07FF
                    case 0xA6: cp = 0x0160; break; // Š
                    case 0xA8: cp = 0x0161; break; // š
                    case 0xB4: cp = 0x017D; break; // Ž
                    case 0xB8: cp = 0x017E; break; // ž
                    case 0xBC: cp = 0x0152; break; // Œ
                    case 0xBD: cp = 0x0153; break; // œ
                    case 0xBE: cp = 0x0178; break; // Ÿ
                    default: break;                // ISO-8859-15 code point == Unicode code point
                }
                if (cp <= 0x7FF)
                {
                    out += (char)(0xC0 | (cp >> 6));
                    out += (char)(0x80 | (cp & 0x3F));
                }
                else
                {
                    // only relevant for €/U+20AC -- 3-byte UTF-8
                    out += (char)(0xE0 | (cp >> 12));
                    out += (char)(0x80 | ((cp >> 6) & 0x3F));
                    out += (char)(0x80 | (cp & 0x3F));
                }
            }
        }

        // UTF-8 -> firmware encoding (ISO-8859-15). Not every Unicode character
        // exists in Latin-15 -> fallback '?'. Returns false as soon as at
        // least one character could not be converted losslessly (the caller
        // can then optionally react differently, e.g. reject the request
        // instead of accepting the `?` fallback).
        inline bool decodeUtf8(std::string& out, const char* s, size_t len)
        {
            bool lossless = true;
            size_t i = 0;
            while (i < len)
            {
                uint8_t c = (uint8_t)s[i];
                uint32_t cp;
                size_t n;
                if (c < 0x80)
                {
                    cp = c;
                    n = 1;
                }
                else if ((c & 0xE0) == 0xC0)
                {
                    cp = c & 0x1F;
                    n = 2;
                }
                else if ((c & 0xF0) == 0xE0)
                {
                    cp = c & 0x0F;
                    n = 3;
                }
                else if ((c & 0xF8) == 0xF0)
                {
                    cp = c & 0x07;
                    n = 4;
                }
                else
                {
                    out += '?';
                    lossless = false;
                    i++;
                    continue;
                }

                bool valid = (i + n <= len);
                for (size_t k = 1; valid && k < n; k++)
                {
                    uint8_t cc = (uint8_t)s[i + k];
                    if ((cc & 0xC0) != 0x80)
                    {
                        valid = false;
                        break;
                    }
                    cp = (cp << 6) | (cc & 0x3F);
                }
                if (!valid)
                {
                    out += '?';
                    lossless = false;
                    i++;
                    continue;
                }

                char b = '?';
                bool mapped = true;
                if (cp < 0x80)
                    b = (char)cp;
                else
                    switch (cp)
                    {
                        case 0x20AC: b = (char)0xA4; break;
                        case 0x0160: b = (char)0xA6; break;
                        case 0x0161: b = (char)0xA8; break;
                        case 0x017D: b = (char)0xB4; break;
                        case 0x017E: b = (char)0xB8; break;
                        case 0x0152: b = (char)0xBC; break;
                        case 0x0153: b = (char)0xBD; break;
                        case 0x0178: b = (char)0xBE; break;
                        default:
                            if (cp <= 0xFF)
                                b = (char)cp;
                            else
                                mapped = false;
                            break;
                    }
                if (!mapped) lossless = false;
                out += b;
                i += n;
            }
            return lossless;
        }
    } // namespace Charset
} // namespace OpenKNX
#endif // OPENKNX_CHARSET && !OPENKNX_CHARSET_HPP_DEFINED
