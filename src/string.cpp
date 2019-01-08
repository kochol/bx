/*
 * Copyright 2010-2018 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "bx_p.h"
#include <bx/allocator.h>
#include <bx/hash.h>
#include <bx/readerwriter.h>
#include <bx/string.h>

namespace bx
{
	inline bool isInRange(char _ch, char _from, char _to)
	{
		return unsigned(_ch - _from) <= unsigned(_to-_from);
	}

	bool isSpace(char _ch)
	{
		return ' '  == _ch
			|| '\t' == _ch
			|| '\n' == _ch
			|| '\v' == _ch
			|| '\f' == _ch
			|| '\r' == _ch
			;
	}

	bool isUpper(char _ch)
	{
		return isInRange(_ch, 'A', 'Z');
	}

	bool isLower(char _ch)
	{
		return isInRange(_ch, 'a', 'z');
	}

	bool isAlpha(char _ch)
	{
		return isLower(_ch) || isUpper(_ch);
	}

	bool isNumeric(char _ch)
	{
		return isInRange(_ch, '0', '9');
	}

	bool isAlphaNum(char _ch)
	{
		return false
			|| isAlpha(_ch)
			|| isNumeric(_ch)
			;
	}

	bool isHexNum(char _ch)
	{
		return false
			|| isInRange(toLower(_ch), 'a', 'f')
			|| isNumeric(_ch)
			;
	}

	bool isPrint(char _ch)
	{
		return isInRange(_ch, ' ', '~');
	}

	typedef bool (*CharTestFn)(char _ch);

	template<CharTestFn fn>
	inline bool isCharTest(const StringView& _str)
	{
		bool result = true;

		for (const char* ptr = _str.getPtr(), *term = _str.getTerm()
			; ptr != term && result
			; ++ptr
			)
		{
			result &= fn(*ptr);
		}

		return result;
	}

	bool isSpace(const StringView& _str)
	{
		return isCharTest<isSpace>(_str);
	}

	bool isUpper(const StringView& _str)
	{
		return isCharTest<isUpper>(_str);
	}

	bool isLower(const StringView& _str)
	{
		return isCharTest<isLower>(_str);
	}

	bool isAlpha(const StringView& _str)
	{
		return isCharTest<isAlpha>(_str);
	}

	bool isNumeric(const StringView& _str)
	{
		return isCharTest<isNumeric>(_str);
	}

	bool isAlphaNum(const StringView& _str)
	{
		return isCharTest<isAlphaNum>(_str);
	}

	bool isHexNum(const StringView& _str)
	{
		return isCharTest<isHexNum>(_str);
	}

	bool isPrint(const StringView& _str)
	{
		return isCharTest<isPrint>(_str);
	}

	char toLower(char _ch)
	{
		return _ch + (isUpper(_ch) ? 0x20 : 0);
	}

	void toLowerUnsafe(char* _inOutStr, int32_t _len)
	{
		for (int32_t ii = 0; ii < _len; ++ii)
		{
			*_inOutStr = toLower(*_inOutStr);
		}
	}

	void toLower(char* _inOutStr, int32_t _max)
	{
		const int32_t len = strLen(_inOutStr, _max);
		toLowerUnsafe(_inOutStr, len);
	}

	char toUpper(char _ch)
	{
		return _ch - (isLower(_ch) ? 0x20 : 0);
	}

	void toUpperUnsafe(char* _inOutStr, int32_t _len)
	{
		for (int32_t ii = 0; ii < _len; ++ii)
		{
			*_inOutStr = toUpper(*_inOutStr);
		}
	}

	void toUpper(char* _inOutStr, int32_t _max)
	{
		const int32_t len = strLen(_inOutStr, _max);
		toUpperUnsafe(_inOutStr, len);
	}

	typedef char (*CharFn)(char _ch);

	inline char toNoop(char _ch)
	{
		return _ch;
	}

	template<CharFn fn>
	inline int32_t strCmp(const char* _lhs, int32_t _lhsMax, const char* _rhs, int32_t _rhsMax)
	{
		int32_t max = min(_lhsMax, _rhsMax);

		for (
			; 0 < max && fn(*_lhs) == fn(*_rhs)
			; ++_lhs, ++_rhs, --max
			)
		{
			if (*_lhs == '\0'
			||  *_rhs == '\0')
			{
				break;
			}
		}

		return 0 == max && _lhsMax == _rhsMax ? 0 : fn(*_lhs) - fn(*_rhs);
	}

	int32_t strCmp(const StringView& _lhs, const StringView& _rhs, int32_t _max)
	{
		return strCmp<toNoop>(
			  _lhs.getPtr()
			, min(_lhs.getLength(), _max)
			, _rhs.getPtr()
			, min(_rhs.getLength(), _max)
			);
	}

	int32_t strCmpI(const StringView& _lhs, const StringView& _rhs, int32_t _max)
	{
		return strCmp<toLower>(
			  _lhs.getPtr()
			, min(_lhs.getLength(), _max)
			, _rhs.getPtr()
			, min(_rhs.getLength(), _max)
			);
	}

	inline int32_t strCmpV(const char* _lhs, int32_t _lhsMax, const char* _rhs, int32_t _rhsMax)
	{
		int32_t max  = min(_lhsMax, _rhsMax);
		int32_t ii   = 0;
		int32_t idx  = 0;
		bool    zero = true;

		for (
			; 0 < max && _lhs[ii] == _rhs[ii]
			; ++ii, --max
			)
		{
			const uint8_t ch = _lhs[ii];
			if ('\0' == ch
			||  '\0' == _rhs[ii])
			{
				break;
			}

			if (!isNumeric(ch) )
			{
				idx  = ii+1;
				zero = true;
			}
			else if ('0' != ch)
			{
				zero = false;
			}
		}

		if (0 == max)
		{
			return _lhsMax == _rhsMax ? 0 : _lhs[ii] - _rhs[ii];
		}

		if ('0' != _lhs[idx]
		&&  '0' != _rhs[idx])
		{
			int32_t jj = 0;
			for (jj = ii
				; 0 < max && isNumeric(_lhs[jj])
				; ++jj, --max
				)
			{
				if (!isNumeric(_rhs[jj]) )
				{
					return 1;
				}
			}

			if (isNumeric(_rhs[jj]))
			{
				return -1;
			}
		}
		else if (zero
			 &&  idx < ii
			 && (isNumeric(_lhs[ii]) || isNumeric(_rhs[ii]) ) )
		{
			return (_lhs[ii] - '0') - (_rhs[ii] - '0');
		}

		return 0 == max && _lhsMax == _rhsMax ? 0 : _lhs[ii] - _rhs[ii];
	}

	int32_t strCmpV(const StringView& _lhs, const StringView& _rhs, int32_t _max)
	{
		return strCmpV(
			  _lhs.getPtr()
			, min(_lhs.getLength(), _max)
			, _rhs.getPtr()
			, min(_rhs.getLength(), _max)
			);
	}

	int32_t strLen(const char* _str, int32_t _max)
	{
		if (NULL == _str)
		{
			return 0;
		}

		const char* ptr = _str;
		for (; 0 < _max && *ptr != '\0'; ++ptr, --_max) {};
		return int32_t(ptr - _str);
	}

	int32_t strLen(const StringView& _str, int32_t _max)
	{
		return strLen(_str.getPtr(), min(_str.getLength(), _max) );
	}

	inline int32_t strCopy(char* _dst, int32_t _dstSize, const char* _src, int32_t _num)
	{
		BX_CHECK(NULL != _dst, "_dst can't be NULL!");
		BX_CHECK(NULL != _src, "_src can't be NULL!");
		BX_CHECK(0 < _dstSize, "_dstSize can't be 0!");

		const int32_t len = strLen(_src, _num);
		const int32_t max = _dstSize-1;
		const int32_t num = (len < max ? len : max);
		memCopy(_dst, _src, num);
		_dst[num] = '\0';

		return num;
	}

	int32_t strCopy(char* _dst, int32_t _dstSize, const StringView& _str, int32_t _num)
	{
		return strCopy(_dst, _dstSize, _str.getPtr(), min(_str.getLength(), _num) );
	}

	inline int32_t strCat(char* _dst, int32_t _dstSize, const char* _src, int32_t _num)
	{
		BX_CHECK(NULL != _dst, "_dst can't be NULL!");
		BX_CHECK(NULL != _src, "_src can't be NULL!");
		BX_CHECK(0 < _dstSize, "_dstSize can't be 0!");

		const int32_t max = _dstSize;
		const int32_t len = strLen(_dst, max);
		return strCopy(&_dst[len], max-len, _src, _num);
	}

	int32_t strCat(char* _dst, int32_t _dstSize, const StringView& _str, int32_t _num)
	{
		return strCat(_dst, _dstSize, _str.getPtr(), min(_str.getLength(), _num) );
	}

	inline const char* strFindUnsafe(const char* _str, int32_t _len, char _ch)
	{
		for (int32_t ii = 0; ii < _len; ++ii)
		{
			if (_str[ii] == _ch)
			{
				return &_str[ii];
			}
		}

		return NULL;
	}

	inline const char* strFind(const char* _str, int32_t _max, char _ch)
	{
		return strFindUnsafe(_str, strLen(_str, _max), _ch);
	}

	StringView strFind(const StringView& _str, char _ch)
	{
		const char* ptr = strFindUnsafe(_str.getPtr(), _str.getLength(), _ch);
		if (NULL == ptr)
		{
			return StringView(_str.getTerm(), _str.getTerm() );
		}

		return StringView(ptr, ptr+1);
	}

	inline const char* strRFindUnsafe(const char* _str, int32_t _len, char _ch)
	{
		for (int32_t ii = _len; 0 <= ii; --ii)
		{
			if (_str[ii] == _ch)
			{
				return &_str[ii];
			}
		}

		return NULL;
	}

	StringView strRFind(const StringView& _str, char _ch)
	{
		const char* ptr = strRFindUnsafe(_str.getPtr(), _str.getLength(), _ch);
		if (NULL == ptr)
		{
			return StringView(_str.getTerm(), _str.getTerm() );
		}

		return StringView(ptr, ptr+1);
	}

	template<CharFn fn>
	inline const char* strFind(const char* _str, int32_t _strMax, const char* _find, int32_t _findMax)
	{
		const char* ptr = _str;

		int32_t       stringLen = _strMax;
		const int32_t findLen   = _findMax;

		for (; stringLen >= findLen; ++ptr, --stringLen)
		{
			// Find start of the string.
			while (fn(*ptr) != fn(*_find) )
			{
				++ptr;
				--stringLen;

				// Search pattern lenght can't be longer than the string.
				if (findLen > stringLen)
				{
					return NULL;
				}
			}

			// Set pointers.
			const char* string = ptr;
			const char* search = _find;

			// Start comparing.
			while (fn(*string++) == fn(*search++) )
			{
				// If end of the 'search' string is reached, all characters match.
				if ('\0' == *search)
				{
					return ptr;
				}
			}
		}

		return NULL;
	}

	StringView strFind(const StringView& _str, const StringView& _find, int32_t _num)
	{
		int32_t len = min(_find.getLength(), _num);

		const char* ptr = strFind<toNoop>(
			  _str.getPtr()
			, _str.getLength()
			, _find.getPtr()
			, len
			);

		if (NULL == ptr)
		{
			return StringView(_str.getTerm(), _str.getTerm() );
		}

		return StringView(ptr, len);
	}

	StringView strFindI(const StringView& _str, const StringView& _find, int32_t _num)
	{
		int32_t len = min(_find.getLength(), _num);

		const char* ptr = strFind<toLower>(
			  _str.getPtr()
			, _str.getLength()
			, _find.getPtr()
			, len
			);

		if (NULL == ptr)
		{
			return StringView(_str.getTerm(), _str.getTerm() );
		}

		return StringView(ptr, len);
	}

	StringView strLTrim(const StringView& _str, const StringView& _chars)
	{
		const char* ptr   = _str.getPtr();
		const char* chars = _chars.getPtr();
		const uint32_t charsLen = _chars.getLength();

		for (uint32_t ii = 0, len = _str.getLength(); ii < len; ++ii)
		{
			if (NULL == strFindUnsafe(chars, charsLen, ptr[ii]) )
			{
				return StringView(ptr + ii, len-ii);
			}
		}

		return _str;
	}

	StringView strLTrimSpace(const StringView& _str)
	{
		for (const char* ptr = _str.getPtr(), *term = _str.getTerm(); ptr != term; ++ptr)
		{
			if (!isSpace(*ptr) )
			{
				return StringView(ptr, term);
			}
		}

		return StringView(_str.getTerm(), _str.getTerm() );
	}

	StringView strLTrimNonSpace(const StringView& _str)
	{
		for (const char* ptr = _str.getPtr(), *term = _str.getTerm(); ptr != term; ++ptr)
		{
			if (isSpace(*ptr) )
			{
				return StringView(ptr, term);
			}
		}

		return StringView(_str.getTerm(), _str.getTerm() );
	}

	StringView strRTrim(const StringView& _str, const StringView& _chars)
	{
		if (!_str.isEmpty() )
		{
			const char* ptr = _str.getPtr();
			const char* chars = _chars.getPtr();
			const uint32_t charsLen = _chars.getLength();

			for (int32_t len = _str.getLength(), ii = len - 1; 0 <= ii; --ii)
			{
				if (NULL == strFindUnsafe(chars, charsLen, ptr[ii]))
				{
					return StringView(ptr, ii + 1);
				}
			}
		}

		return _str;
	}

	StringView strTrim(const StringView& _str, const StringView& _chars)
	{
		return strLTrim(strRTrim(_str, _chars), _chars);
	}

	constexpr uint32_t kFindStep = 1024;

	StringView strFindNl(const StringView& _str)
	{
		StringView str(_str);

		for (; str.getPtr() != _str.getTerm()
			; str = StringView(min(str.getPtr() + kFindStep, _str.getTerm() ), min(str.getPtr() + kFindStep*2, _str.getTerm() ) )
			)
		{
			StringView eol = strFind(str, "\r\n");
			if (!eol.isEmpty() )
			{
				return StringView(eol.getTerm(), _str.getTerm() );
			}

			eol = strFind(str, '\n');
			if (!eol.isEmpty() )
			{
				return StringView(eol.getTerm(), _str.getTerm() );
			}
		}

		return StringView(_str.getTerm(), _str.getTerm() );
	}

	StringView strFindEol(const StringView& _str)
	{
		StringView str(_str);

		for (; str.getPtr() != _str.getTerm()
			 ; str = StringView(min(str.getPtr() + kFindStep, _str.getTerm() ), min(str.getPtr() + kFindStep*2, _str.getTerm() ) )
			)
		{
			StringView eol = strFind(str, "\r\n");
			if (!eol.isEmpty() )
			{
				return StringView(eol.getPtr(), _str.getTerm() );
			}

			eol = strFind(str, '\n');
			if (!eol.isEmpty() )
			{
				return StringView(eol.getPtr(), _str.getTerm() );
			}
		}

		return StringView(_str.getTerm(), _str.getTerm() );
	}

	static const char* strSkipWord(const char* _str, int32_t _max)
	{
		for (char ch = *_str++; 0 < _max && (isAlphaNum(ch) || '_' == ch); ch = *_str++, --_max) {};
		return _str-1;
	}

	StringView strWord(const StringView& _str)
	{
		const char* ptr  = _str.getPtr();
		const char* term = strSkipWord(ptr, _str.getLength() );
		return StringView(ptr, term);
	}

	StringView strFindBlock(const StringView& _str, char _open, char _close)
	{
		const char* curr  = _str.getPtr();
		const char* term  = _str.getTerm();
		const char* start = NULL;

		int32_t count = 0;
		for (char ch = *curr; curr != term && count >= 0; ch = *(++curr) )
		{
			if (ch == _open)
			{
				if (0 == count)
				{
					start = curr;
				}

				++count;
			}
			else if (ch == _close)
			{
				--count;

				if (NULL == start)
				{
					break;
				}

				if (0 == count)
				{
					return StringView(start, curr+1);
				}
			}
		}

		return StringView(term, term);
	}

	StringView normalizeEolLf(char* _out, int32_t _size, const StringView& _str)
	{
		const char* start = _out;
		const char* end   = _out;

		if (0 < _size)
		{
			const char* curr = _str.getPtr();
			const char* term = _str.getTerm();
			end  = _out + _size;
			for (char ch = *curr; curr != term && _out < end; ch = *(++curr) )
			{
				if ('\r' != ch)
				{
					*_out++ = ch;
				}
			}

			end = _out;
		}

		return StringView(start, end);
	}

	StringView findIdentifierMatch(const StringView& _str, const StringView& _word)
	{
		const int32_t len = _word.getLength();
		StringView ptr = strFind(_str, _word);
		for (; !ptr.isEmpty(); ptr = strFind(StringView(ptr.getPtr() + len, _str.getTerm() ), _word) )
		{
			char ch = *(ptr.getPtr() - 1);
			if (isAlphaNum(ch) || '_' == ch)
			{
				continue;
			}

			ch = *(ptr.getPtr() + len);
			if (isAlphaNum(ch) || '_' == ch)
			{
				continue;
			}

			return ptr;
		}

		return StringView(_str.getTerm(), _str.getTerm() );
	}

	StringView findIdentifierMatch(const StringView& _str, const char** _words)
	{
		for (StringView word = *_words; !word.isEmpty(); ++_words, word = *_words)
		{
			StringView match = findIdentifierMatch(_str, word);
			if (!match.isEmpty() )
			{
				return match;
			}
		}

		return StringView(_str.getTerm(), _str.getTerm() );
	}

	namespace
	{
		struct Param
		{
			Param()
				: width(0)
				, base(10)
				, prec(INT32_MAX)
				, fill(' ')
				, bits(0)
				, left(false)
				, upper(false)
				, spec(false)
				, sign(false)
			{
			}

			int32_t width;
			int32_t base;
			int32_t prec;
			char fill;
			uint8_t bits;
			bool left;
			bool upper;
			bool spec;
			bool sign;
		};

		static int32_t write(WriterI* _writer, const char* _str, int32_t _len, const Param& _param, Error* _err)
		{
			int32_t size = 0;
			int32_t len = (int32_t)strLen(_str, _len);
			int32_t padding = _param.width > len ? _param.width - len : 0;
			bool sign = _param.sign && len > 1 && _str[0] != '-';
			padding = padding > 0 ? padding - sign : 0;

			if (!_param.left)
			{
				size += writeRep(_writer, _param.fill, padding, _err);
			}

			if (NULL == _str)
			{
				size += write(_writer, "(null)", 6, _err);
			}
			else if (_param.upper)
			{
				for (int32_t ii = 0; ii < len; ++ii)
				{
					size += write(_writer, toUpper(_str[ii]), _err);
				}
			}
			else if (sign)
			{
				size += write(_writer, '+', _err);
				size += write(_writer, _str, len, _err);
			}
			else
			{
				size += write(_writer, _str, len, _err);
			}

			if (_param.left)
			{
				size += writeRep(_writer, _param.fill, padding, _err);
			}

			return size;
		}

		static int32_t write(WriterI* _writer, char _ch, const Param& _param, Error* _err)
		{
			return write(_writer, &_ch, 1, _param, _err);
		}

		static int32_t write(WriterI* _writer, const char* _str, const Param& _param, Error* _err)
		{
			return write(_writer, _str, _param.prec, _param, _err);
		}

		static int32_t write(WriterI* _writer, int32_t _i, const Param& _param, Error* _err)
		{
			char str[33];
			int32_t len = toString(str, sizeof(str), _i, _param.base);

			if (len == 0)
			{
				return 0;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, int64_t _i, const Param& _param, Error* _err)
		{
			char str[33];
			int32_t len = toString(str, sizeof(str), _i, _param.base);

			if (len == 0)
			{
				return 0;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, uint32_t _u, const Param& _param, Error* _err)
		{
			char str[33];
			int32_t len = toString(str, sizeof(str), _u, _param.base);

			if (len == 0)
			{
				return 0;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, uint64_t _u, const Param& _param, Error* _err)
		{
			char str[33];
			int32_t len = toString(str, sizeof(str), _u, _param.base);

			if (len == 0)
			{
				return 0;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, double _d, const Param& _param, Error* _err)
		{
			char str[1024];
			int32_t len = toString(str, sizeof(str), _d);

			if (len == 0)
			{
				return 0;
			}

			if (_param.upper)
			{
				toUpperUnsafe(str, len);
			}

			const char* dot = strFind(str, INT32_MAX, '.');
			if (NULL != dot)
			{
				const int32_t prec = INT32_MAX == _param.prec ? 6 : _param.prec;
				const int32_t precLen = int32_t(
						dot
						+ uint32_min(prec + _param.spec, 1)
						+ prec
						- str
						);
				if (precLen > len)
				{
					for (int32_t ii = len; ii < precLen; ++ii)
					{
						str[ii] = '0';
					}
					str[precLen] = '\0';
				}
				len = precLen;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, const void* _ptr, const Param& _param, Error* _err)
		{
			char str[35] = "0x";
			int32_t len = toString(str + 2, sizeof(str) - 2, uint32_t(uintptr_t(_ptr) ), 16);

			if (len == 0)
			{
				return 0;
			}

			len += 2;
			return write(_writer, str, len, _param, _err);
		}
	} // anonymous namespace

	int32_t write(WriterI* _writer, const StringView& _format, va_list _argList, Error* _err)
	{
		MemoryReader reader(_format.getPtr(), _format.getLength() );

		int32_t size = 0;

		while (_err->isOk() )
		{
			char ch = '\0';

			Error err;
			if (!read(&reader, ch, &err))
				break;

			if (!_err->isOk()
			||  !err.isOk() )
			{
				break;
			}
			else if ('%' == ch)
			{
				// %[flags][width][.precision][length sub-specifier]specifier
				read(&reader, ch);

				Param param;

				// flags
				while (' ' == ch
				||     '-' == ch
				||     '+' == ch
				||     '0' == ch
				||     '#' == ch)
				{
					switch (ch)
					{
						default:
						case ' ': param.fill = ' ';  break;
						case '-': param.left = true; break;
						case '+': param.sign = true; break;
						case '0': param.fill = '0';  break;
						case '#': param.spec = true; break;
					}

					read(&reader, ch);
				}

				if (param.left)
				{
					param.fill = ' ';
				}

				// width
				if ('*' == ch)
				{
					read(&reader, ch);
					param.width = va_arg(_argList, int32_t);

					if (0 > param.width)
					{
						param.left  = true;
						param.width = -param.width;
					}

				}
				else
				{
					while (isNumeric(ch) )
					{
						param.width = param.width * 10 + ch - '0';
						read(&reader, ch);
					}
				}

				// .precision
				if ('.' == ch)
				{
					read(&reader, ch);

					if ('*' == ch)
					{
						read(&reader, ch);
						param.prec = va_arg(_argList, int32_t);
					}
					else
					{
						param.prec = 0;
						while (isNumeric(ch) )
						{
							param.prec = param.prec * 10 + ch - '0';
							read(&reader, ch);
						}
					}
				}

				// length sub-specifier
				while ('h' == ch
				||     'I' == ch
				||     'l' == ch
				||     'j' == ch
				||     't' == ch
				||     'z' == ch)
				{
					switch (ch)
					{
						default: break;

						case 'j': param.bits = sizeof(intmax_t )*8; break;
						case 't': param.bits = sizeof(size_t   )*8; break;
						case 'z': param.bits = sizeof(ptrdiff_t)*8; break;

						case 'h': case 'I': case 'l':
							switch (ch)
							{
								case 'h': param.bits = sizeof(short int)*8; break;
								case 'l': param.bits = sizeof(long int )*8; break;
								default: break;
							}

							read(&reader, ch);
							switch (ch)
							{
								case 'h': param.bits = sizeof(signed char  )*8; break;
								case 'l': param.bits = sizeof(long long int)*8; break;
								case '3':
								case '6':
									read(&reader, ch);
									switch (ch)
									{
										case '2': param.bits = sizeof(int32_t)*8; break;
										case '4': param.bits = sizeof(int64_t)*8; break;
										default: break;
									}
									break;

								default: seek(&reader, -1); break;
							}
							break;
					}

					read(&reader, ch);
				}

				// specifier
				switch (toLower(ch) )
				{
					case 'c':
						size += write(_writer, char(va_arg(_argList, int32_t) ), param, _err);
						break;

					case 's':
						size += write(_writer, va_arg(_argList, const char*), param, _err);
						break;

					case 'o':
						param.base = 8;
						switch (param.bits)
						{
						default: size += write(_writer, va_arg(_argList, int32_t), param, _err); break;
						case 64: size += write(_writer, va_arg(_argList, int64_t), param, _err); break;
						}
						break;

					case 'i':
					case 'd':
						param.base = 10;
						switch (param.bits)
						{
						default: size += write(_writer, va_arg(_argList, int32_t), param, _err); break;
						case 64: size += write(_writer, va_arg(_argList, int64_t), param, _err); break;
						};
						break;

					case 'e':
					case 'f':
					case 'g':
						param.upper = isUpper(ch);
						size += write(_writer, va_arg(_argList, double), param, _err);
						break;

					case 'p':
						size += write(_writer, va_arg(_argList, void*), param, _err);
						break;

					case 'x':
						param.base  = 16;
						param.upper = isUpper(ch);
						switch (param.bits)
						{
						default: size += write(_writer, va_arg(_argList, uint32_t), param, _err); break;
						case 64: size += write(_writer, va_arg(_argList, uint64_t), param, _err); break;
						}
						break;

					case 'u':
						param.base = 10;
						switch (param.bits)
						{
						default: size += write(_writer, va_arg(_argList, uint32_t), param, _err); break;
						case 64: size += write(_writer, va_arg(_argList, uint64_t), param, _err); break;
						}
						break;

					default:
						size += write(_writer, ch, _err);
						break;
				}
			}
			else
			{
				size += write(_writer, ch, _err);
			}
		}

		return size;
	}

	int32_t write(WriterI* _writer, Error* _err, const StringView* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		int32_t total = write(_writer, *_format, argList, _err);
		va_end(argList);
		return total;
	}

	int32_t write(WriterI* _writer, Error* _err, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		int32_t total = write(_writer, _format, argList, _err);
		va_end(argList);
		return total;
	}

	int32_t vsnprintf(char* _out, int32_t _max, const char* _format, va_list _argList)
	{
		if (1 < _max)
		{
			StaticMemoryBlockWriter writer(_out, uint32_t(_max) );

			Error err;
			va_list argListCopy;
			va_copy(argListCopy, _argList);
			int32_t size = write(&writer, _format, argListCopy, &err);
			va_end(argListCopy);

			if (err.isOk() )
			{
				size += write(&writer, '\0', &err);
				return size - 1 /* size without '\0' terminator */;
			}
			else
			{
				_out[_max-1] = '\0';
			}
		}

		Error err;
		SizerWriter sizer;
		va_list argListCopy;
		va_copy(argListCopy, _argList);
		int32_t size = write(&sizer, _format, argListCopy, &err);
		va_end(argListCopy);

		return size;
	}

	int32_t snprintf(char* _out, int32_t _max, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		int32_t total = vsnprintf(_out, _max, _format, argList);
		va_end(argList);
		return total;
	}

	static const char s_units[] = { 'B', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };

	template<uint32_t Kilo, char KiloCh0, char KiloCh1, CharFn fn>
	inline int32_t prettify(char* _out, int32_t _count, uint64_t _value)
	{
		uint8_t idx = 0;
		double value = double(_value);
		while (_value != (_value&0x7ff)
		&&     idx < BX_COUNTOF(s_units) )
		{
			_value /= Kilo;
			value  *= 1.0/double(Kilo);
			++idx;
		}

		return snprintf(_out, _count, "%0.2f %c%c%c", value
			, fn(s_units[idx])
			, idx > 0 ? KiloCh0 : '\0'
			, KiloCh1
			);
	}

	int32_t prettify(char* _out, int32_t _count, uint64_t _value, Units::Enum _units)
	{
		if (Units::Kilo == _units)
		{
			return prettify<1000, 'B', '\0', toNoop>(_out, _count, _value);
		}

		return prettify<1024, 'i', 'B', toUpper>(_out, _count, _value);
	}

} // namespace bx
