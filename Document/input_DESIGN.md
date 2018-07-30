效果：可读取不同编码格式的字符流。

BOM: byte-order-mark

-----------------------------------------------------------------------------------------
UTF-8: variable-length, compatible with ASCII (another benefit is easy separate non-ASCII
content from special meaning ASCII chars), good structure, easy error detection.

BOM: 0x EF BB BF
code point          UTF-8                               Effective Bits      Occupied Bits
U+0000 - U+007F     0xxxxxxx                            7                   8
U+0080 - U+07FF     110xxxxx 10xxxxxx                   11                  16
U+0800 - U+FFFF     1110xxxx 10xxxxxx 10xxxxxx          16                  24
U+10000 - U+10FFFF  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 21                  32

-----------------------------------------------------------------------------------------
UTF-16: from fixed-length UCS-2, variable-length (almost fixed-length in practice.)

BOM(BE): 0x FE FF
code point          UTF-16                              Effective Bits      Occupied Bits
U+0000 - U+D7FF     xxxxxxxx xxxxxxxx                   16                  16
U+D800 - U+DFFF     (reserved)
U+E000 - U+FFFF     xxxxxxxx xxxxxxxx                   16                  16
U+10000 - U+10FFFF  110110xx xxxxxxxx 110111xx xxxxxxxx 20                  32

BOM(LE): 0x FF FE
code point          UTF-16                              Effective Bits      Occupied Bits
U+0000 - U+D7FF     xxxxxxxx xxxxxxxx                   16                  16
U+D800 - U+DFFF     (reserved)
U+E000 - U+FFFF     xxxxxxxx xxxxxxxx                   16                  16
U+10000 - U+10FFFF  xxxxxxxx 110110xx xxxxxxxx 110111xx 20                  32

-----------------------------------------------------------------------------------------
UTF-32: fixed-length, equal to code point.

BOM(BE): 00 00 FE FF
BOM(LE): FF FE 00 00

-----------------------------------------------------------------------------------------
谈谈 U+D800 - U+DFFF
还有编码叫 WTF-8 ?!
谈谈 wchar_t
